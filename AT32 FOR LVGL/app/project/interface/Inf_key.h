/**
 ******************************************************************************
 * File Name : Inf_key.h
 * Description: Driver for the 4 onboard buttons + encoder middle key (active low),
 *              for LVGL keypad / application logic
 *
 * Button pin definitions come from at32f403a_407_wk_config.h:
 *   KEY1 PB12 / KEY2 PB13 / KEY3 PB14 / KEY4 PB15
 *   KEY_ENCODER = rotary encoder middle key PB6
 *
 * Typical integration (e.g. an RTOS input task at 10ms period):
 *   key_init();            // call once
 *   key_scan();            // periodic scan (debounced internally, 10~20ms)
 *
 * Use the level state key_is_down() inside the LVGL keypad read callback:
 *   if (key_is_down(KEY_1))   data->key = LV_KEY_UP;
 *   else if (key_is_down(KEY_2)) data->key = LV_KEY_DOWN;
 *   ... map as the menu requires ...
 *   data->state = (any key held) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
 *   (when several of KEY1..KEY4 are pressed at once, the upper layer
 *    decides the priority)
 ******************************************************************************
 */
#ifndef __INF_KEY_H
#define __INF_KEY_H

#include <stdint.h>

/* Key indexes (KEY_1..KEY_4 match IN_KEYx_L_*, KEY_ENCODER is the encoder
 * middle key IN_ENCODER_KEY_L_*) */
typedef enum
{
  KEY_1 = 0,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_ENCODER, /* encoder middle key (PB6, active low), used by Inf_encoder */
  KEY_MAX
} key_id_t;

/* -------------------- Functions -------------------- */

/**
 * @brief  Key GPIO init: PB12~PB15 pull-up inputs (also configured in
 *         wk_gpio_config(); this function is idempotent and safe to re-run).
 */
void key_init(void);

/**
 * @brief  Periodically scan the keys (debounce and update state/edge events).
 * @note   Recommended call period 10~20ms; internal debounce ~2 periods.
 */
void key_scan(void);

/**
 * @brief  Query the stable level state of a key.
 * @param  id key index (KEY_1..KEY_4)
 * @retval 1 = currently pressed (level state, used by LVGL keypad), 0 = released
 */
uint8_t key_is_down(key_id_t id);

/**
 * @brief  Query whether a "press" edge just occurred (auto-cleared on read).
 * @retval 1 = one press event detected
 */
uint8_t key_pressed(key_id_t id);

/**
 * @brief  Query whether a "release" edge just occurred (auto-cleared on read).
 * @retval 1 = one release event detected
 */
uint8_t key_released(key_id_t id);

/**
 * @brief  Hardware-validation helper: get the key that is currently held.
 * @note   Scans internally, so it also works standalone in a bare-metal loop
 *         (calling it together with a periodic key_scan() is safe).
 * @retval 1..5 : KEY_1..KEY_4 / encoder middle key currently held
 * @retval 0    : no key held
 *
 * Example (bare-metal loop):
 *   int8_t key = key_test_read();  // -> 1..5 when a key is held, else 0
 */
int8_t key_test_read(void);

#endif /* __INF_KEY_H */
