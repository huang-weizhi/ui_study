/*=============================================================================
 * Inf_touch.c - FT6336U touch controller driver (hardware I2C3)
 *
 * Bus: AT32F403A I2C3, SCL = PA8, SDA = PC9, 100 kHz.
 *      Peripheral + GPIOs + clock are configured by WorkBench
 *      (wk_i2c3_init() in src/wk_i2c.c, called from main.c before the RTOS
 *      starts).  The transfer itself is delegated to the Artery
 *      "i2c_application" middleware - i2c_memory_read() handles the whole
 *      master-receive sequence including the AT32-specific single-byte
 *      NACK/STOP ordering, so this file is only a thin FT6336U protocol
 *      layer on top of it.  Reset line RST = PC8 is owned by this driver.
 *
 * Why hardware I2C instead of the old bit-banged soft_i2c.c:
 *   The software driver produced SCL/SDA with CPU instructions plus
 *   delay_us().  A full frame read took ~1.7 ms and every bit was destroyed
 *   by an interrupt or by the LCD DMA (SRAM -> XMC) occupying the AHB bus.
 *   That is why touch only worked at -O0 (slow LVGL, few DMA transfers) and
 *   died or returned garbage coordinates at -O1/-O2.  The I2C3 peripheral
 *   generates the whole waveform itself, so optimisation level, interrupt
 *   load and DMA traffic no longer influence the transfer.  No interrupt
 *   masking is required any more - the RTOS tick is never blocked.
 *
 * FT6336U frame format (Focaltech official driver):
 *   read 15 bytes starting at register 0x01;
 *   buf[0] = reg 0x01 (header, unused)
 *   buf[1] = reg 0x02: high nibble event type, low nibble point count
 *   point i at buf[2 + 6*i]: [XH][XL][ID/YH][YL][P][AREA]
 *   XH bit7:6 = event flag (0=down / 1=up / 2=contact), X/Y are 12 bit.
 *===========================================================================*/
#include <stdio.h>
#include "Inf_touch.h"
#include "at32f403a_407_conf.h"   /* register-level I2C + CRM + GPIO        */
#include "perf_counter.h"         /* delay_us()                              */
#include "i2c_app.h"              /* i2c_application middleware + hi2c3     */

/*---------------------------------------------------------------------------
 *                          Hardware reset GPIO (PC8)
 *-------------------------------------------------------------------------*/
#define FT6336_RST_PORT GPIOC
#define FT6336_RST_PIN  GPIO_PINS_8
#define FT6336_RST_CLK  CRM_GPIOC_PERIPH_CLOCK

static void ft6336_rst_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(FT6336_RST_CLK, TRUE);
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pull           = GPIO_PULL_NONE;
    gpio_init_struct.gpio_pins           = FT6336_RST_PIN;
    gpio_init(FT6336_RST_PORT, &gpio_init_struct);
    gpio_bits_set(FT6336_RST_PORT, FT6336_RST_PIN); /* release reset */
}

static void ft6336_hw_reset(void)
{
    gpio_bits_reset(FT6336_RST_PORT, FT6336_RST_PIN); /* assert reset */
    delay_us(10);
    gpio_bits_set(FT6336_RST_PORT, FT6336_RST_PIN);   /* release reset */
    delay_us(50000);
}

/*---------------------------------------------------------------------------
 *                          I2C low-level transfer
 *---------------------------------------------------------------------------
 * The I2C3 master-receive sequence is delegated to the Artery
 * "i2c_application" middleware (app/middlewares/i2c_application_library).
 * That library already handles every AT32-specific quirk of the master
 * receiver - the NACK+early-STOP ordering for a single byte read, the
 * ACK-next mode for two byte reads, the 9th-SCL / TDC sequencing for longer
 * bursts - so this driver is only a thin wrapper around i2c_memory_read().
 *
 * I2C3 itself (peripheral, GPIOs, clock, 100 kHz timing) is configured by
 * WorkBench: wk_i2c3_init() + wk_i2c_app_init() run from main.c before the
 * RTOS starts.  wk_i2c_app_init() sets up the global "hi2c3" handle
 * (hi2c3.i2cx = I2C3), which the middleware functions take as their first
 * argument.
 */
#define FT6336_I2C_TIMEOUT_LOOPS 0xFFFFFFFFu

/* i2c3 handle owned by wk_i2c_app.c (initialised by wk_i2c_app_init()) */
extern i2c_handle_type hi2c3;

/* Per-call fault code: mirrors the middleware error_code after a failing
 * transfer, read by the 1Hz diagnostic in input_function via
 * ft6336_get_last_fault().  Lets us see exactly which I2C step is failing
 * without having to instrument the transfer with blocking printf calls
 * (which would themselves break the timing).  Values are the middleware
 * i2c_status_type enum: 0=OK, 1..12=ERR_STEP_1..12, 13=ERR_START,
 * 14=ERR_ADDR10, 15=ERR_ADDR, 16=ERR_STOP, 17=ERR_ACKFAIL,
 * 18=ERR_TIMEOUT, 19=ERR_INTERRUPT. */
static volatile uint8_t s_last_fault = 0;

/**
 * @brief  Master-read sequence: write "reg" then read "len" bytes from the
 *         FT6336U, implemented on top of the i2c_application middleware.
 * @retval 0 on success, 1 on any bus error / timeout / NACK.
 */
static uint8_t ft6336_do_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
    if (i2c_memory_read(&hi2c3, I2C_MEM_ADDR_WIDTH_8, FT6336_I2C_READ,
                        (uint16_t)reg, buf, (uint16_t)len,
                        FT6336_I2C_TIMEOUT_LOOPS) == I2C_OK) {
        s_last_fault = 0;
        return 0;
    }

    s_last_fault = (uint8_t)hi2c3.error_code;
    return 1;
}

static uint8_t ft6336_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return ft6336_do_read(reg, buf, len);
}

/*---------------------------------------------------------------------------
 *                                  Init
 *-------------------------------------------------------------------------*/
void ft6336_init(void)
{
    uint8_t id = 0xFF;
    uint8_t i;

    /* Reset line is owned by this driver.  I2C3 itself is set up by
     * wk_i2c3_init() from main() before the RTOS starts. */
    ft6336_rst_init();
    ft6336_hw_reset();
    delay_us(200000);

    /* Wait for the touch firmware to be ready (chip-ID register becomes
     * valid and non-zero / non-0xFF). */
    for (i = 0; i < 10; i++) {
        if ((ft6336_read_reg(FT6336_REG_FOCALTECH_ID, &id, 1) == 0) &&
            (id != 0x00) && (id != 0xFF)) {
            break;
        }
        delay_us(20000);
    }
    (void)id;
}

/*---------------------------------------------------------------------------
 *                    Touch data reading & coordinate mapping
 *-------------------------------------------------------------------------*/
/* Runtime raw-range calibration state, seeded by the FT6336_RAW_* macros.
 * Every touch that reaches beyond the current extremes widens the range, so
 * after the finger has touched the real screen edges once, the mapping
 * becomes exact on both axes. */
static uint16_t s_raw_x_top    = FT6336_RAW_X_TOP;    /* raw X at top edge    */
static uint16_t s_raw_x_bottom = FT6336_RAW_X_BOTTOM; /* raw X at bottom edge */
static uint16_t s_raw_y_left   = FT6336_RAW_Y_LEFT;   /* raw Y at left edge   */
static uint16_t s_raw_y_right  = FT6336_RAW_Y_RIGHT;  /* raw Y at right edge  */

static uint8_t ft6336_read_frame(uint16_t *raw_x, uint16_t *raw_y)
{
    uint8_t buf[FT6336_TOUCH_FRAME_SIZE];
    uint8_t etype, num, i, base;
    uint8_t xh, xl, yh, yl, flag;
    uint8_t found = 0;

    if (ft6336_read_reg(FT6336_REG_TOUCH_FRAME, buf, sizeof(buf)) != 0)
        return 0;

    etype = (buf[1] >> 4) & 0x0F;
    num   = buf[1] & 0x0F;

    if ((etype != FT6336_EVENT_DEFAULT) && (etype != FT6336_EVENT_NUM))
        return 0; /* extra message / fw init / error / idle (all 0xFF) */
    if (num > FT6336_MAX_POINTS)
        num = FT6336_MAX_POINTS;

    for (i = 0; i < num; i++) {
        base = FT6336_POINT_BASE + FT6336_POINT_LEN * i;
        xh = buf[base];
        xl = buf[base + 1];
        yh = buf[base + 2];
        yl = buf[base + 3];

        if ((xh == 0xFF) && (xl == 0xFF) && (yh == 0xFF) && (yl == 0xFF))
            continue; /* empty slot */

        flag = (xh >> 6) & 0x03; /* 0=down, 1=up, 2=contact */
        if (flag == FT6336_FLAG_UP)
            continue; /* this slot is reporting a release */

        if (found == 0) {
            *raw_x = (uint16_t)((xh & 0x0F) << 8) | xl;
            *raw_y = (uint16_t)((yh & 0x0F) << 8) | yl;
            found = 1;
        }
    }
    return found;
}

/**
 * @brief  Read the primary touch point and map it to screen coordinates.
 * @note   Register-to-screen mapping (calibrated on this panel):
 *           reg X grows toward the TOP edge  -> screen Y = invert(reg X)
 *           reg Y grows toward the RIGHT edge -> screen X = reg Y
 *         The FT6336_RAW_* macros are only initial seeds; the extremes are
 *         widened automatically while touches reach the screen edges.
 */
uint8_t ft6336_read_touch(uint16_t *x, uint16_t *y)
{
    uint16_t raw_x, raw_y, mapped_x, mapped_y;
    uint32_t span, pos;

    if (ft6336_read_frame(&raw_x, &raw_y) == 0)
        return 0;

    /* widen the calibration range when this touch reaches a new extreme */
    if (raw_x > s_raw_x_top)        s_raw_x_top    = raw_x;
    else if (raw_x < s_raw_x_bottom) s_raw_x_bottom = raw_x;
    if (raw_y > s_raw_y_right)       s_raw_y_right  = raw_y;
    else if (raw_y < s_raw_y_left)   s_raw_y_left   = raw_y;

    /* Screen Y (0 top .. 319 bottom) = inverted register X */
    span = s_raw_x_top - s_raw_x_bottom;
    pos  = (uint32_t)(s_raw_x_top - raw_x) * (FT6336_SCR_HEIGHT - 1u);
    mapped_y = (span > 0) ? (uint16_t)(pos / span) : 0u;

    /* Screen X (0 left .. 479 right) = register Y */
    span = s_raw_y_right - s_raw_y_left;
    pos  = (uint32_t)(raw_y - s_raw_y_left) * (FT6336_SCR_WIDTH - 1u);
    mapped_x = (span > 0) ? (uint16_t)(pos / span) : 0u;

    /* Clamp into the panel: before the seed range has been widened by real
     * edge touches the linear map can produce values beyond the screen, and
     * LVGL silently drops out-of-range points (that is why only part of a
     * widget responds until the calibration settles). */
    if (mapped_x > (FT6336_SCR_WIDTH - 1u))  mapped_x = FT6336_SCR_WIDTH - 1u;
    if (mapped_y > (FT6336_SCR_HEIGHT - 1u)) mapped_y = FT6336_SCR_HEIGHT - 1u;

    if (x) *x = mapped_x;
    if (y) *y = mapped_y;

    /* Raw-value probe for calibrating the FT6336_RAW_* seeds in Inf_touch.h:
     * touch the four screen edges and note the raw extremes.  Default off -
     * printf() in this path blocks the UART for ~3 ms. */
#if 1
    printf("[TOUCH-RAW] raw_x=%u raw_y=%u -> x=%u y=%u | seeds x[%u..%u] y[%u..%u]\r\n",
           raw_x, raw_y, mapped_x, mapped_y,
           s_raw_x_bottom, s_raw_x_top, s_raw_y_left, s_raw_y_right);
#endif
    return 1;
}

/**
 * @brief  Hardware test helper: primary touch point, for manual checkout.
 * @note   Usage (poll every ~10 ms, ft6336_init() must have run first):
 *           uint16_t tx, ty;
 *           uint8_t down = ft6336_test_read(&tx, &ty);
 *           -> down = 1 while touched; tx/ty hold the screen coordinates
 *              (X 0..479, Y 0..319 after the built-in mapping).
 *         Same result as ft6336_read_touch(); kept as the dedicated test
 *         entry so the hardware checkout does not depend on the LVGL API.
 * @retval 1 = finger down, 0 = no touch (or read failed)
 */
uint8_t ft6336_test_read(uint16_t *x, uint16_t *y)
{
    return ft6336_read_touch(x, y);
}

uint8_t ft6336_get_last_fault(void)
{
    return s_last_fault;
}

uint8_t ft6336_diag(uint8_t *chip_id, uint8_t *vendor_id, uint8_t *status)
{
    uint8_t c = 0xFF, v = 0xFF, s = 0xFF;
    uint8_t bus_err = 0;

    /* single-byte reads of the three diagnostic registers */
    if (ft6336_read_reg(FT6336_REG_FOCALTECH_ID, &c, 1) != 0) bus_err = 1;
    if (ft6336_read_reg(FT6336_REG_VENDOR_ID,    &v, 1) != 0) bus_err = 1;
    if (ft6336_read_reg(0x02,                    &s, 1) != 0) bus_err = 1;

    if (chip_id)   *chip_id   = c;
    if (vendor_id) *vendor_id = v;
    if (status)    *status    = s;
    return bus_err;
}
