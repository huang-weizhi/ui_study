/**
 ******************************************************************************
 * File Name : Inf_encoder.h
 * Description: Rotary encoder driver (incremental A/B phases + middle key),
 *              for LVGL encoder / application logic
 *
 * Pin definitions come from at32f403a_407_wk_config.h:
 *   A phase PB7 (IN_ENCODER_A_HL_PIN, EXINT line 7)
 *   B phase PB5 (IN_ENCODER_B_HL_PIN, EXINT line 5)
 *   Middle key PB6 (IN_ENCODER_KEY_L_PIN, active low)
 *
 * Quadrature decoding runs in the EXINT9_5 interrupt (both A/B edges trigger
 * the ISR), so no periodic sampling is required:
 *   key_init();          // holds the KEY_ENCODER debounce state
 *   encoder_init();      // configures EXINT lines 5/7 + NVIC, starts ISR
 *   // periodically (e.g. every 10 ms) just read the rotation/key state:
 *   int16_t clicks = encoder_get_diff();
 *   key_scan();          // still needed for the middle key debounce
 *
 * Use inside the LVGL encoder read callback (rotation + middle key state):
 *   data->enc_diff = encoder_get_diff();  // net "clicks" since last read
 *   data->state    = encoder_key_is_down() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
 *
 * If the rotation direction is opposite to what the menu expects: set
 * ENCODER_DIR_INVERT to 1 in Inf_encoder.c. If the edges per click differ
 * (e.g. some encoders change phase only twice per click): change
 * ENC_EDGES_PER_CLICK.
 ******************************************************************************
 */
#ifndef __INF_ENCODER_H
#define __INF_ENCODER_H

#include <stdint.h>

/* -------------------- Functions -------------------- */

/**
 * @brief  Configure the A/B EXINT inputs (pull-ups) + EXINT lines 5/7 with
 *         both-edge triggers, enable the EXINT9_5 NVIC channel and reset the
 *         counters. Call once at startup before rotating the knob.
 * @note   Idempotent and safe to call repeatedly.
 */
void encoder_init(void);

/**
 * @brief  Net "clicks" rotated since the last call (positive = CW /
 *         negative = CCW; counter is cleared on read).
 * @note   One click = ENC_EDGES_PER_CLICK (default 4) phase transitions;
 *         the remainder below one click is carried over, never lost.
 *         Decoding is done in the EXINT ISR; this only reads the result.
 */
int16_t encoder_get_diff(void);

/**
 * @brief  Level state of the encoder middle key (KEY_ENCODER), 1 = pressed
 *         (e.g. acts as ENTER in LVGL).
 */
uint8_t encoder_key_is_down(void);

/**
 * @brief  Whether the encoder middle key was just pressed (edge event,
 *         cleared on read).
 */
uint8_t encoder_key_pressed(void);

/**
 * @brief  Whether the encoder middle key was just released (edge event,
 *         cleared on read).
 */
uint8_t encoder_key_released(void);

/**
 * @brief  Hardware-validation helper: net detents (clicks) + direction since
 *         the last call.
 * @note   Works standalone in a bare-metal loop (ISR decodes, this only
 *         reads). If the reported direction is reversed on the real
 *         hardware, set ENCODER_DIR_INVERT to 1 in Inf_encoder.c.
 * @retval > 0 : rotated CW, value = number of detents
 * @retval < 0 : rotated CCW, value = number of detents
 * @retval 0   : not rotated since the last call
 *
 * Example (bare-metal loop):
 *   int16_t enc = encoder_test_read(); // -> +1/-1 per detent, 0 = idle
 *
 * If the value stays 0 while turning: check the wiring first (A/B to
 * PB7/PB5, common C to GND) and watch the raw phases with
 * encoder_test_read_ab().
 */
int16_t encoder_test_read(void);

/**
 * @brief  Hardware test helper: raw A/B phase levels (bit1 = A, bit0 = B).
 * @note   Slowly rotate the knob and poll this value: a working encoder
 *         cycles 00->01->11->10 (Gray). If it never changes, the phases are
 *         not reaching the MCU (wiring, common C not to GND, or a dead
 *         pull-up/pull-down on A/B).
 * @retval 0..3 : current phase combination
 */
uint8_t encoder_test_read_ab(void);

#endif /* __INF_ENCODER_H */
