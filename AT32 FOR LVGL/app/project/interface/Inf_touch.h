/*=============================================================================
 * Inf_touch.h - FT6336U touch driver interface (official FT6336U frame
 *               protocol, adapted for LVGL)
 *
 *   ft6336_init()       : reset + wait TP ready (call once at startup)
 *   ft6336_read_touch() : read touch frame (reg 0x01), return whether a
 *                         finger is down and output screen coordinates
 *
 * Bus: AT32 hardware I2C3 (SCL = PA8, SDA = PC9).  Peripheral, GPIOs and
 *      clock are configured by WorkBench (wk_i2c3_init() in src/wk_i2c.c,
 *      called from main.c before the RTOS starts).  This driver talks to
 *      the I2C3 peripheral directly through the AT32 register-level driver
 *      (at32f403a_407_i2c.h) - no middleware library is required.
 *
 *   Coordinate parse follows the Focaltech FT6336U kernel driver:
 *     read 15 bytes from register 0x01:
 *       buf[0]            = reg 0x01 (header byte, unused)
 *       buf[1]            = reg 0x02, high nibble = event type
 *                           (0x0 TOUCH_DEFAULT / 0x2 TOUCH_EVENT_NUM),
 *                           low nibble = number of touch points
 *       buf[2 + 6*i ...]  = point i, 6 bytes:
 *                           [0] XH (bit7:6 event flag 0=down 1=up 2=contact,
 *                                    bit3:0 X high)
 *                           [1] XL
 *                           [2] ID/YH (bit7:4 point ID, bit3:0 Y high)
 *                           [3] YL
 *                           [4] pressure   [5] area
 * ============================================================================*/
#ifndef __INF_TOUCH_H
#define __INF_TOUCH_H

#include <stdint.h>

/*============================================================================
 *                        USER CONFIGURATION
 *============================================================================*/

/* LCD logical resolution (landscape 480x320) */
#define FT6336_SCR_WIDTH 480
#define FT6336_SCR_HEIGHT 320

/*============================================================================
 *                       TOUCH CALIBRATION (USER CONFIG)
 *============================================================================
 * The FT6336U reports coordinates in the TOUCH PANEL's coordinate system
 * (origin/range defined by the module vendor's firmware); LVGL works in the
 * SCREEN's coordinate system (480x320, origin at the top-left of the frame
 * buffer).  The four values below are the raw register readings at the four
 * screen edges, and they fully define the mapping between the two systems:
 *
 *   screen X = linear map of raw Y,  [FT6336_RAW_Y_LEFT .. FT6336_RAW_Y_RIGHT] -> [0 .. 479]
 *   screen Y = inverted linear map of raw X, [FT6336_RAW_X_BOTTOM .. FT6336_RAW_X_TOP] -> [319 .. 0]
 *
 * i.e. on this panel the register axes are rotated 90 degrees relative to the
 * screen: register X grows toward the TOP edge, register Y grows toward the
 * RIGHT edge.  That is why screen X comes from register Y and vice versa.
 *
 * HOW TO MEASURE (one time):
 *   1. In Inf_touch.c find the "[TOUCH-RAW]" printf and change its #if 0 to
 *      #if 1, rebuild and download.
 *   2. Touch the middle of each screen edge in turn and note the raw values
 *      from the serial log:
 *        top edge    -> raw_x  goes into FT6336_RAW_X_TOP
 *        bottom edge -> raw_x  goes into FT6336_RAW_X_BOTTOM
 *        left edge   -> raw_y  goes into FT6336_RAW_Y_LEFT
 *        right edge  -> raw_y  goes into FT6336_RAW_Y_RIGHT
 *   3. Paste the values below, set the printf back to #if 0, rebuild.
 *
 * The mapping is a pure constant function after this - no runtime
 * auto-calibration, no state, identical behaviour on every boot.
 *============================================================================*/
#define FT6336_RAW_X_TOP 295    /* raw X at the top edge middle    */
#define FT6336_RAW_X_BOTTOM 15  /* raw X at the bottom edge middle */
#define FT6336_RAW_Y_LEFT 25    /* raw Y at the left edge middle   */
#define FT6336_RAW_Y_RIGHT 472  /* raw Y at the right edge middle  */

/*========================= END USER CONFIG ===============================*/

/* I2C address (7bit 0x38) */
#define FT6336_I2C_ADDR 0x38
#define FT6336_I2C_WRITE (FT6336_I2C_ADDR << 1)
#define FT6336_I2C_READ ((FT6336_I2C_ADDR << 1) | 1)

/* FT6336U register definitions (official driver) */
#define FT6336_REG_TOUCH_FRAME 0x01  /* touch event frame base address        */
#define FT6336_REG_FOCALTECH_ID 0xA3 /* chip ID register                     */
#define FT6336_REG_VENDOR_ID 0xA8    /* vendor ID register (default 0x11)    */
#define FT6336_REG_WORKMODE 0x00     /* work mode: normal = 0x00             */

/* touch frame geometry */
#define FT6336_TOUCH_FRAME_SIZE 15 /* bytes read from reg 0x01 (officially \
                                      FTS_SIZE_DEFAULT)                     */
#define FT6336_POINT_LEN 6         /* bytes per touch point                 */
#define FT6336_POINT_BASE 2        /* first point starts at buf[2]          */
#define FT6336_MAX_POINTS 2        /* FT6336U: 2 touch points               */

/* Bus timing.  i2c_wait_flag() counts loop iterations, so the timeout is only
 * an upper bound: a full 15 byte frame at 100 kHz takes about 1.4 ms and each
 * individual flag wait is far below the limit below. */
#define FT6336_I2C_SPEED_HZ   100000u
#define FT6336_I2C_TIMEOUT    0x40000u
#define FT6336_I2C_ERR_RECOVER 3 /* consecutive failures before bus recovery */

/* event type nibble of register 0x02 high 4 bits */
#define FT6336_EVENT_DEFAULT 0x00
#define FT6336_EVENT_NUM 0x02
#define FT6336_EVENT_EXTRA_MSG 0x08
#define FT6336_EVENT_FW_INIT 0x81
#define FT6336_EVENT_IGNORE 0xFE
#define FT6336_EVENT_ERROR 0xFF

/* per-point event flag (XH bit7:6) */
#define FT6336_FLAG_DOWN 0
#define FT6336_FLAG_UP 1
#define FT6336_FLAG_CONTACT 2

/* Expected chip vendor ID (register 0xA8) */
#define FT6336_PANNEL_ID 0x11

/* -------------------- Functions -------------------- */
/** Reset and initialize FT6336 (wait for TP firmware ready) */
void ft6336_init(void);

/**
 * @brief  Diagnostics helper for touch bring-up debugging.
 * @note   Polls three registers to tell "bus dead" apart from
 *         "chip alive but reporting no touch":
 *           0xA3 = chip ID, 0xA8 = vendor ID, 0x02 = touch status.
 * @param  chip_id    : output, register 0xA3 value
 * @param  vendor_id  : output, register 0xA8 value
 * @param  status     : output, register 0x02 value
 * @retval 1 = I2C bus error while reading (no ACK, chip not responding),
 *         0 = bus OK (chip_id/vendor_id/status are valid)
 */
uint8_t ft6336_diag(uint8_t *chip_id, uint8_t *vendor_id, uint8_t *status);

/**
 * @brief  Read the primary touch point screen coordinates.
 * @param  x, y : output coordinates (valid while pressed, 0..479 / 0..319)
 * @retval 1 = a finger is currently down, 0 = no touch (or read failed)
 */
uint8_t ft6336_read_touch(uint16_t *x, uint16_t *y);

/**
 * @brief  Hardware-validation helper: read the primary touch point.
 * @note   Same result as ft6336_read_touch(); kept as the dedicated test
 *         entry so a hardware checkout does not depend on the LVGL API.
 * @param  x, y : output screen coordinates (valid while pressed)
 * @retval 1 = finger down, 0 = no touch (or read failed)
 *
 * Example (bare-metal loop):
 *   uint16_t tx, ty;
 *   if (ft6336_test_read(&tx, &ty))  // finger down -> tx/ty valid
 */
uint8_t ft6336_test_read(uint16_t *x, uint16_t *y);

/**
 * @brief  I2C fault code from the last failing read - debug helper.
 * @note   Value is the i2c_application middleware error_code:
 *           0  = OK (last transfer succeeded)
 *           1..12  = ERR_STEP_1 .. ERR_STEP_12  (which stage failed)
 *           13 = ERR_START      14 = ERR_ADDR10   15 = ERR_ADDR
 *           16 = ERR_STOP       17 = ERR_ACKFAIL  18 = ERR_TIMEOUT
 *           19 = ERR_INTERRUPT
 */
uint8_t ft6336_get_last_fault(void);

#endif /* __INF_TOUCH_H */
