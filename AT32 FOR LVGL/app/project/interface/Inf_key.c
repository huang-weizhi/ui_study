/**
 ******************************************************************************
 * File Name : Inf_key.c
 * Description: Onboard physical buttons driver (active low)
 *
 *   KEY1 PB12 / KEY2 PB13 / KEY3 PB14 / KEY4 PB15
 *   Idle high (internal pull-up), reads 0 when pressed.
 *
 * Implementation: software debounce - the stable level only toggles after
 *   KEY_DEBOUNCE_SAMPLES consecutive scans agree, and one press/release edge
 *   event is produced. Edge events are cleared automatically when read.
 ******************************************************************************
 */

#include "Inf_key.h"
#include "at32f403a_407.h"
#include "at32f403a_407_wk_config.h"

/* Debounce sample count: key_scan() is called every 10ms,
 * 2 samples debounce about 20ms */
#define KEY_DEBOUNCE_SAMPLES 2

/* Per-key software debounce data */
typedef struct
{
  uint8_t raw_last;    /* last sampled level (1 = pressed) */
  uint8_t dbnc_cnt;    /* consecutive equal-sample counter */
  uint8_t state;       /* current stable level (1 = pressed) */
  uint8_t press_evt;   /* press edge (unread) */
  uint8_t release_evt; /* release edge (unread) */
} key_sw_t;

static key_sw_t s_key[KEY_MAX];

/* Key map (order matches key_id_t) */
static gpio_type *const s_key_port[KEY_MAX] = {
    IN_KEY1_L_GPIO_PORT, IN_KEY2_L_GPIO_PORT,
    IN_KEY3_L_GPIO_PORT, IN_KEY4_L_GPIO_PORT,
    IN_ENCODER_KEY_L_GPIO_PORT};
static const uint32_t s_key_pin[KEY_MAX] = {
    IN_KEY1_L_PIN, IN_KEY2_L_PIN, IN_KEY3_L_PIN, IN_KEY4_L_PIN,
    IN_ENCODER_KEY_L_PIN};

/**
 * @brief  Read the raw level of a single key (normalized: 1 = pressed).
 */
static uint8_t key_read_raw(uint8_t id)
{
  /* Active low: port reads 0 means pressed */
  if (gpio_input_data_bit_read(s_key_port[id], s_key_pin[id]) == RESET)
    return 1;
  return 0;
}

/* -------------------- API -------------------- */

void key_init(void)
{
  uint8_t i;
  gpio_init_type gpio_init_struct;

  /* Enable clock (safe to enable twice, also done in wk_gpio_config) */
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

  /* PB12~PB15 + encoder middle key PB6 as pull-up inputs */
  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init_struct.gpio_pins = IN_KEY1_L_PIN | IN_KEY2_L_PIN |
                               IN_KEY3_L_PIN | IN_KEY4_L_PIN |
                               IN_ENCODER_KEY_L_PIN;
  gpio_init(IN_KEY1_L_GPIO_PORT, &gpio_init_struct);

  /* Initialize state from current levels to avoid false edges at power-up */
  for (i = 0; i < KEY_MAX; i++)
  {
    s_key[i].raw_last = key_read_raw(i);
    s_key[i].dbnc_cnt = 0;
    s_key[i].state = s_key[i].raw_last;
    s_key[i].press_evt = 0;
    s_key[i].release_evt = 0;
  }
}

void key_scan(void)
{
  uint8_t i;
  uint8_t raw;

  for (i = 0; i < KEY_MAX; i++)
  {
    raw = key_read_raw(i);

    if (raw == s_key[i].raw_last)
    {
      /* Same as last sample: count up, toggle stable state once threshold
       * reached and state not yet synced */
      if (s_key[i].dbnc_cnt < KEY_DEBOUNCE_SAMPLES)
        s_key[i].dbnc_cnt++;

      if (s_key[i].dbnc_cnt >= KEY_DEBOUNCE_SAMPLES &&
          s_key[i].state != raw)
      {
        s_key[i].state = raw;
        if (raw)
          s_key[i].press_evt = 1;
        else
          s_key[i].release_evt = 1;
      }
    }
    else
    {
      /* Sample changed: restart counting, do not change state during debounce */
      s_key[i].raw_last = raw;
      s_key[i].dbnc_cnt = 0;
    }
  }
}

uint8_t key_is_down(key_id_t id)
{
  if (id >= KEY_MAX)
    return 0;
  return s_key[id].state;
}

uint8_t key_pressed(key_id_t id)
{
  uint8_t evt;
  if (id >= KEY_MAX)
    return 0;
  evt = s_key[id].press_evt;
  s_key[id].press_evt = 0; /* clear after read */
  return evt;
}

uint8_t key_released(key_id_t id)
{
  uint8_t evt;
  if (id >= KEY_MAX)
    return 0;
  evt = s_key[id].release_evt;
  s_key[id].release_evt = 0; /* clear after read */
  return evt;
}

/**
 * @brief  Hardware test helper: key currently held, for manual checkout.
 * @note   Usage (poll every ~10 ms):
 *           int8_t key = key_test_read();
 *           -> 0    : no key held
 *              1..4 : KEY1..KEY4
 *              5    : encoder middle key
 *         Scans internally, so a standalone key_scan() is not required;
 *         calling it together with the periodic key_scan() in the RTOS task
 *         is safe too. When several keys are held, the lowest key number
 *         wins.
 * @retval 0..5 : held key number (see above), 0 = none
 */
int8_t key_test_read(void)
{
  key_id_t i;

  key_scan(); /* refresh the debounce state first */

  for (i = KEY_1; i < KEY_MAX; i++)
  {
    if (key_is_down(i))
      return (int8_t)(i + 1); /* KEY_1 -> 1 ... KEY_ENCODER -> 5 */
  }
  return 0;
}
