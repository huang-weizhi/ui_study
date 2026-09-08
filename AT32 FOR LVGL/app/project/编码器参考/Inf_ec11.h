#ifndef __INF_EC11_H
#define __INF_EC11_H

#include "at32f403a_407.h"
#include <stdint.h>

/* ================== Pin definitions (adjust according to actual wiring) ================== */
/* Encoder 1 */
#define EC11_1_A_PORT GPIOE
#define EC11_1_A_PIN GPIO_PINS_6
#define EC11_1_A_EXTI_LINE EXINT_LINE_6
#define EC11_1_A_IRQN EXINT9_5_IRQn

#define EC11_1_B_PORT GPIOE
#define EC11_1_B_PIN GPIO_PINS_5
#define EC11_1_B_EXTI_LINE EXINT_LINE_5
#define EC11_1_B_IRQN EXINT9_5_IRQn

#define EC11_1_KEY_PORT GPIOE
#define EC11_1_KEY_PIN GPIO_PINS_4

/* Encoder 2 */
#define EC11_2_A_PORT GPIOC
#define EC11_2_A_PIN GPIO_PINS_15
#define EC11_2_A_EXTI_LINE EXINT_LINE_15
#define EC11_2_A_IRQN EXINT15_10_IRQn

#define EC11_2_B_PORT GPIOC
#define EC11_2_B_PIN GPIO_PINS_14
#define EC11_2_B_EXTI_LINE EXINT_LINE_14
#define EC11_2_B_IRQN EXINT15_10_IRQn

#define EC11_2_KEY_PORT GPIOC
#define EC11_2_KEY_PIN GPIO_PINS_13

/* ================== Configuration structure ================== */
typedef struct
{
    gpio_type *gpio_a;
    uint16_t pin_a;
    gpio_type *gpio_b;
    uint16_t pin_b;
    gpio_type *gpio_key;
    uint16_t pin_key;

    uint32_t exti_line_a;
    uint32_t exti_line_b;
    uint8_t irqn_a;
    uint8_t irqn_b;

    uint8_t pulses_per_detent; /* Number of steps per detent (normally 4) */
    uint16_t fast_rotation_ms; /* Fast rotation threshold: interval < this value -> fast */
    uint16_t debounce_ms;      /* Button debounce time (ms) */
    uint16_t long_press_ms;    /* Long press first trigger time (ms) */
    uint16_t long_repeat_ms;   /* Long press repeat interval (ms) */
    uint16_t double_click_ms;  /* Maximum double click interval (ms) */
} EC11_Config;

/* ================== Event types ================== */
typedef enum
{
    EC11_EVENT_NONE = 0,
    EC11_EVENT_CW,                /* Forward one detent */
    EC11_EVENT_CCW,               /* Reverse one detent */
    EC11_EVENT_FAST_CW,           /* Fast forward rotation */
    EC11_EVENT_FAST_CCW,          /* Fast reverse rotation */
    EC11_EVENT_SHORT_CLICK,       /* Short click */
    EC11_EVENT_LONG_PRESS_ONCE,   /* Long press first trigger */
    EC11_EVENT_LONG_PRESS_REPEAT, /* Long press repeat */
    EC11_EVENT_DOUBLE_CLICK       /* Double click */
} EC11_Event;

/* ================== Encoder instance ================== */
typedef struct
{
    EC11_Config cfg;

    /* Rotation (interrupt‑driven) */
    volatile int32_t count;   /* Absolute step counter */
    volatile uint8_t last_ab; /* Last stable A/B level state (0~3) */
    int32_t last_event_count; /* Counter value at last event output */
    uint32_t last_event_tick; /* System tick at last event output */

    /* Button (polling) */
    uint8_t key_stable;
    uint8_t key_prev_raw;
    uint32_t key_debounce_tick;
    uint32_t key_press_tick;
    uint8_t long_active;
    uint8_t click_pending;
    uint32_t click_tick;
    uint8_t double_lock;
    uint32_t last_long_repeat_tick;

    /* Event cache */
    volatile EC11_Event cached_event;
} EC11_Encoder;

/* ================== Public API ================== */
void EC11_TickInc(void); /* Call in 1ms timer */
uint32_t EC11_GetTick(void);

void EC11_Init(EC11_Encoder *enc, const EC11_Config *cfg);
void EC11_ISR_A(EC11_Encoder *enc);
void EC11_ISR_B(EC11_Encoder *enc);
void EC11_KeyScan(EC11_Encoder *enc); /* Call periodically (1ms) */
EC11_Event EC11_GetEvent(EC11_Encoder *enc);

int32_t EC11_GetCount(EC11_Encoder *enc);
void EC11_ResetCount(EC11_Encoder *enc);

#endif