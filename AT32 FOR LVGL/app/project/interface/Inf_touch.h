/*=============================================================================
 * Inf_touch.h - FT6336U touch driver interface (official FT6336U frame
 *               protocol, adapted for LVGL)
 *
 *   ft6336_init()       : reset + wait TP ready (call once at startup)
 *   ft6336_read_touch() : read touch frame (reg 0x01), return whether a
 *                         finger is down and output screen coordinates
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

#include "soft_i2c.h"

/*============================================================================
 *                        USER CONFIGURATION
 *============================================================================*/

/* LCD logical resolution (landscape 480x320) */
#define FT6336_SCR_WIDTH 480
#define FT6336_SCR_HEIGHT 320

/**
 * @brief  Raw coordinate INITIAL SEEDS for this panel.
 *         Measured on THIS board, the FT6336 registers behave as:
 *           register X grows toward the TOP edge,
 *           register Y grows toward the RIGHT edge.
 *         So screen Y = inverted register X, screen X = register Y.
 *         These four values are only the startup seeds of the mapping: every
 *         touch reaching beyond them automatically widens the used range
 *         (see ft6336_read_touch()). If a seed is too small (e.g. the right
 *         edge was not measured at the true glass edge), the coordinate
 *         saturated before the screen edge; just touch the real edges once
 *         and it self-corrects.
 */
#define FT6336_RAW_X_TOP 305   /* reg X value at the top edge middle    */
#define FT6336_RAW_X_BOTTOM 13 /* reg X value at the bottom edge middle */
#define FT6336_RAW_Y_LEFT 15   /* reg Y value at the left edge middle   */
#define FT6336_RAW_Y_RIGHT 319 /* reg Y value at the right edge middle  */

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

#endif /* __INF_TOUCH_H */
