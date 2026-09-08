/**
 * @file lv_tick_freertos.h
 * @brief  LVGL custom tick source — uses a plain SysTick counter (no FreeRTOS dependency).
 *
 * lvgl_systick_ms is incremented in SysTick_Handler (at32f403a_407_int.c).
 * SysTick is configured for 1 ms period, so 1 tick = 1 ms.
 */
#ifndef LV_TICK_FREERTOS_H
#define LV_TICK_FREERTOS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t lvgl_systick_ms;

static inline uint32_t lv_tick_ms_get(void)
{
    return lvgl_systick_ms;
}

#ifdef __cplusplus
}
#endif

#endif /* LV_TICK_FREERTOS_H */
