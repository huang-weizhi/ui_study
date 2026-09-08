#include "inf_ec11.h"
#include <stddef.h> /* NULL */

/* ================== System tick (1ms) ================== */
static volatile uint32_t _tick = 0;

void EC11_TickInc(void)
{
    _tick++;
}

uint32_t EC11_GetTick(void)
{
    return _tick;
}

/* ================== Quadrature decoding state machine ================== */
static const int8_t step_table[16] = {
    0,  // 00 -> 00
    1,  // 00 -> 01  forward
    -1, // 00 -> 10  reverse
    0,  // 00 -> 11  invalid
    -1, // 01 -> 00  reverse
    0,  // 01 -> 01
    0,  // 01 -> 10  invalid
    1,  // 01 -> 11  forward
    1,  // 10 -> 00  forward
    0,  // 10 -> 01  invalid
    0,  // 10 -> 10
    -1, // 10 -> 11  reverse
    0,  // 11 -> 00  invalid
    -1, // 11 -> 01  reverse
    1,  // 11 -> 10  forward
    0   // 11 -> 11
};

/* ================== Read pin level ================== */
static inline uint8_t read_pin(gpio_type *port, uint16_t pin)
{
    return gpio_input_data_bit_read(port, pin) ? 1 : 0;
}

/* ================== Initialization ================== */
void EC11_Init(EC11_Encoder *enc, const EC11_Config *cfg)
{
    enc->cfg = *cfg;

    /* Rotation state */
    uint8_t a = read_pin(enc->cfg.gpio_a, enc->cfg.pin_a);
    uint8_t b = read_pin(enc->cfg.gpio_b, enc->cfg.pin_b);
    enc->last_ab = (a << 1) | b; // Initialize last AB state
    enc->count = 0;
    enc->last_event_count = 0;
    enc->last_event_tick = _tick;

    /* Button state */
    enc->key_stable = 1;
    enc->key_prev_raw = 1;
    enc->key_debounce_tick = 0;
    enc->key_press_tick = 0;
    enc->long_active = 0;
    enc->click_pending = 0;
    enc->click_tick = 0;
    enc->double_lock = 0;
    enc->last_long_repeat_tick = 0;

    enc->cached_event = EC11_EVENT_NONE;
}

/* ================== EXTI interrupt handler ================== */
void EC11_ISR_A(EC11_Encoder *enc)
{
    uint8_t a = read_pin(enc->cfg.gpio_a, enc->cfg.pin_a);
    uint8_t b = read_pin(enc->cfg.gpio_b, enc->cfg.pin_b);
    uint8_t cur = (a << 1) | b;

    int8_t step = step_table[(enc->last_ab << 2) | cur];
    if (step != 0)
    {
        enc->count += step;
        enc->last_ab = cur;
    }
}

void EC11_ISR_B(EC11_Encoder *enc)
{
    uint8_t a = read_pin(enc->cfg.gpio_a, enc->cfg.pin_a);
    uint8_t b = read_pin(enc->cfg.gpio_b, enc->cfg.pin_b);
    uint8_t cur = (a << 1) | b;

    int8_t step = step_table[(enc->last_ab << 2) | cur];
    if (step != 0)
    {
        enc->count += step;
        enc->last_ab = cur;
    }
}

/* ================== Button scanning (polling) ================== */
void EC11_KeyScan(EC11_Encoder *enc)
{
    if (enc->cfg.gpio_key == NULL)
        return;

    uint32_t now = _tick;
    uint8_t raw = read_pin(enc->cfg.gpio_key, enc->cfg.pin_key);

    if (raw != enc->key_prev_raw)
    {
        enc->key_debounce_tick = now;
    }
    enc->key_prev_raw = raw;

    if ((now - enc->key_debounce_tick) >= enc->cfg.debounce_ms)
    {
        uint8_t prev = enc->key_stable;
        enc->key_stable = raw;

        if (prev == 1 && raw == 0)
        { // Pressed
            enc->key_press_tick = now;
            enc->long_active = 0;
            enc->last_long_repeat_tick = 0;

            if (enc->click_pending && !enc->double_lock)
            {
                if ((now - enc->click_tick) <= enc->cfg.double_click_ms)
                {
                    enc->cached_event = EC11_EVENT_DOUBLE_CLICK;
                    enc->click_pending = 0;
                    enc->double_lock = 1;
                }
                else
                {
                    enc->cached_event = EC11_EVENT_SHORT_CLICK;
                    enc->click_pending = 0;
                }
            }
        }
        else if (prev == 0 && raw == 1)
        { // Released
            if (!enc->double_lock)
            {
                uint32_t duration = now - enc->key_press_tick;
                if (duration < enc->cfg.long_press_ms)
                {
                    if (!enc->click_pending)
                    {
                        enc->click_pending = 1;
                        enc->click_tick = now;
                    }
                }
            }
            enc->double_lock = 0;
        }
    }

    /* Long press detection */
    if (enc->key_stable == 0 && !enc->double_lock)
    {
        uint32_t duration = now - enc->key_press_tick;
        if (duration >= enc->cfg.long_press_ms)
        {
            if (!enc->long_active)
            {
                enc->cached_event = EC11_EVENT_LONG_PRESS_ONCE;
                enc->long_active = 1;
                enc->last_long_repeat_tick = now;
                enc->click_pending = 0;
            }
            else if ((now - enc->last_long_repeat_tick) >= enc->cfg.long_repeat_ms)
            {
                enc->cached_event = EC11_EVENT_LONG_PRESS_REPEAT;
                enc->last_long_repeat_tick = now;
            }
        }
    }

    /* Short click timeout (double click window expired) */
    if (enc->click_pending)
    {
        if ((now - enc->click_tick) > enc->cfg.double_click_ms)
        {
            enc->cached_event = EC11_EVENT_SHORT_CLICK;
            enc->click_pending = 0;
        }
    }
}

/* ================== Get event (detent‑based, supports fast/slow rotation) ================== */
EC11_Event EC11_GetEvent(EC11_Encoder *enc)
{
    /* Return cached button/double‑click events first */
    if (enc->cached_event != EC11_EVENT_NONE)
    {
        EC11_Event ev = enc->cached_event;
        enc->cached_event = EC11_EVENT_NONE;
        return ev;
    }

    /* Calculate accumulated steps since last event */
    int32_t delta = enc->count - enc->last_event_count;
    int32_t detent = (int32_t)enc->cfg.pulses_per_detent;

    if (delta >= detent)
    { // Forward by one detent
        enc->last_event_count += detent;
        uint32_t now = _tick;
        uint32_t interval = now - enc->last_event_tick;
        enc->last_event_tick = now;
        return (interval < enc->cfg.fast_rotation_ms) ? EC11_EVENT_FAST_CW : EC11_EVENT_CW;
    }
    else if (delta <= -detent)
    { // Reverse by one detent
        enc->last_event_count -= detent;
        uint32_t now = _tick;
        uint32_t interval = now - enc->last_event_tick;
        enc->last_event_tick = now;
        return (interval < enc->cfg.fast_rotation_ms) ? EC11_EVENT_FAST_CCW : EC11_EVENT_CCW;
    }

    return EC11_EVENT_NONE;
}

/* ================== Helper functions ================== */
int32_t EC11_GetCount(EC11_Encoder *enc)
{
    return enc->count;
}

void EC11_ResetCount(EC11_Encoder *enc)
{
    enc->count = 0;
    enc->last_event_count = 0;
}