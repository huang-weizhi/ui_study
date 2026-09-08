/*=============================================================================
 * Inf_touch.c - FT6336U touch controller driver (official FT6336U protocol)
 *
 * Communication: soft I2C (SCL=PA8 / SDA=PC9), reset RST=PC8.
 * Provides only: initialization + primary touch point read (incl. raw ->
 * screen coordinate mapping).
 * The frame read/parse follows the Focaltech FT6336U kernel driver
 * ("Focaltech_Touch_FT6336U_Driver_for非MTK平台"):
 *   - touch event frame is read from register 0x01 (15 bytes);
 *   - register 0x02 high nibble = event type, low nibble = point number;
 *   - each point: [XH][XL][ID/YH][YL][P][AREA], XH bit7:6 = event flag
 *     (0 down / 1 up / 2 contact), X/Y are 12-bit values.
 *   - no legacy FT6x06 THGROUP/PERIODACTIVE/MODE registers are written at
 *     init; the chip keeps its factory configuration.
 *===========================================================================*/

#include "Inf_touch.h"

/* ---------- I2C instance for FT6336 ---------- */
static soft_i2c_t g_ft6336_i2c;

/* ---------- FT6336 pins on the UI test board ---------- */
#define FT6336_SCL_PORT GPIOA
#define FT6336_SCL_PIN GPIO_PINS_8
#define FT6336_SCL_CLK CRM_GPIOA_PERIPH_CLOCK

#define FT6336_SDA_PORT GPIOC
#define FT6336_SDA_PIN GPIO_PINS_9
#define FT6336_SDA_CLK CRM_GPIOC_PERIPH_CLOCK

#define FT6336_RST_PORT GPIOC
#define FT6336_RST_PIN GPIO_PINS_8
#define FT6336_RST_CLK CRM_GPIOC_PERIPH_CLOCK

/* ---------------------------------------------------------- */
/*      Hardware reset GPIO                                    */
/* ---------------------------------------------------------- */
static void ft6336_rst_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(FT6336_RST_CLK, TRUE);
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_pins = FT6336_RST_PIN;
    gpio_init(FT6336_RST_PORT, &gpio_init_struct);
    gpio_bits_set(FT6336_RST_PORT, FT6336_RST_PIN); /* release reset */
}

static void ft6336_hw_reset(void)
{
    gpio_bits_reset(FT6336_RST_PORT, FT6336_RST_PIN); /* assert reset */
    delay_us(10);
    gpio_bits_set(FT6336_RST_PORT, FT6336_RST_PIN); /* release reset */
    delay_us(50000);
}

/* ---------------------------------------------------------- */
/*      I2C read / write                                       */
/* ---------------------------------------------------------- */
static uint8_t ft6336_read_reg(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    soft_i2c_start(&g_ft6336_i2c);
    soft_i2c_send_byte(&g_ft6336_i2c, FT6336_I2C_WRITE);
    if (soft_i2c_wait_ack(&g_ft6336_i2c, I2C_ACK_TIMEOUT))
    {
        soft_i2c_stop(&g_ft6336_i2c);
        return 1;
    }
    soft_i2c_send_byte(&g_ft6336_i2c, reg);
    if (soft_i2c_wait_ack(&g_ft6336_i2c, I2C_ACK_TIMEOUT))
    {
        soft_i2c_stop(&g_ft6336_i2c);
        return 1;
    }
    soft_i2c_start(&g_ft6336_i2c);
    soft_i2c_send_byte(&g_ft6336_i2c, FT6336_I2C_READ);
    if (soft_i2c_wait_ack(&g_ft6336_i2c, I2C_ACK_TIMEOUT))
    {
        soft_i2c_stop(&g_ft6336_i2c);
        return 1;
    }
    for (i = 0; i < len; i++)
    {
        buf[i] = soft_i2c_receive_byte(&g_ft6336_i2c);
        if (i < len - 1)
            soft_i2c_ack(&g_ft6336_i2c);
        else
            soft_i2c_no_ack(&g_ft6336_i2c);
    }
    soft_i2c_stop(&g_ft6336_i2c);
    return 0;
}

/* ---------------------------------------------------------- */
/*      Init                                                   */
/* ---------------------------------------------------------- */
void ft6336_init(void)
{
    uint8_t id = 0xFF;
    uint8_t i;

    /* I2C: SCL=PA8, SDA=PC9 */
    soft_i2c_init(&g_ft6336_i2c,
                  FT6336_SCL_PORT, FT6336_SCL_PIN, FT6336_SCL_CLK,
                  FT6336_SDA_PORT, FT6336_SDA_PIN, FT6336_SDA_CLK);

    /* hardware reset and wait for the chip to stabilise */
    ft6336_rst_init();
    ft6336_hw_reset();
    delay_us(200000);

    /* wait until the TP firmware is ready (chip-ID register becomes valid).
     * The official driver polls the chip ID at 0xA3 after reset. */
    for (i = 0; i < 10; i++)
    {
        if (ft6336_read_reg(FT6336_REG_FOCALTECH_ID, &id, 1) == 0 &&
            (id != 0x00) && (id != 0xFF))
        {
            break;
        }
        delay_us(20000);
    }

    /* Work mode 0x00 is "normal working" by factory default. Unlike the old
     * FT6x06-style driver, no THGROUP/PERIODACTIVE/MODE(0xA4) registers are
     * written here: the official FT6336U driver does not touch them. */
    (void)id;
}

/* ---------------------------------------------------------- */
/*      Touch data reading & coordinate mapping               */
/* ---------------------------------------------------------- */
/* Runtime raw-range calibration state, seeded by the FT6336_RAW_* macros.
 * Every touch that reaches beyond the current extremes widens the range, so
 * after the finger has touched the real screen edges once, the mapping
 * becomes exact on both axes. */
static uint16_t s_raw_x_top    = FT6336_RAW_X_TOP;    /* raw X at top edge    */
static uint16_t s_raw_x_bottom = FT6336_RAW_X_BOTTOM; /* raw X at bottom edge */
static uint16_t s_raw_y_left   = FT6336_RAW_Y_LEFT;   /* raw Y at left edge   */
static uint16_t s_raw_y_right  = FT6336_RAW_Y_RIGHT;  /* raw Y at right edge  */

/**
 * @brief  Read the FT6336U touch frame (reg 0x01) and, when a finger is
 *         down/contact, output its raw X/Y register coordinates.
 * @param  raw_x, raw_y : output register coordinates (12-bit each)
 * @retval 1 = at least one active touch point, 0 = none / frame invalid
 */
static uint8_t ft6336_read_frame(uint16_t *raw_x, uint16_t *raw_y)
{
    uint8_t buf[FT6336_TOUCH_FRAME_SIZE];
    uint8_t etype, num, i, base;
    uint8_t xh, xl, yh, yl, flag;
    uint8_t found = 0;

    if (ft6336_read_reg(FT6336_REG_TOUCH_FRAME, buf, sizeof(buf)) != 0)
        return 0;

    /* reg 0x02 = buf[1]: high nibble event type, low nibble point number */
    etype = (buf[1] >> 4) & 0x0F;
    num = buf[1] & 0x0F;

    if ((etype != FT6336_EVENT_DEFAULT) && (etype != FT6336_EVENT_NUM))
        return 0; /* extra message / fw init / error / idle (all 0xFF) */
    if (num > FT6336_MAX_POINTS)
        num = FT6336_MAX_POINTS;

    for (i = 0; i < num; i++)
    {
        base = FT6336_POINT_BASE + FT6336_POINT_LEN * i;
        xh = buf[base];
        xl = buf[base + 1];
        yh = buf[base + 2];
        yl = buf[base + 3];

        if ((xh == 0xFF) && (xl == 0xFF) && (yh == 0xFF) && (yl == 0xFF))
            continue; /* empty slot */

        flag = (xh >> 6) & 0x03; /* 0 = down, 1 = up, 2 = contact */
        if (flag == FT6336_FLAG_UP)
            continue; /* this slot is reporting a release */

        if (found == 0)
        {
            /* first active point (primary touch) */
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
    if (raw_x > s_raw_x_top)
        s_raw_x_top = raw_x;
    else if (raw_x < s_raw_x_bottom)
        s_raw_x_bottom = raw_x;
    if (raw_y > s_raw_y_right)
        s_raw_y_right = raw_y;
    else if (raw_y < s_raw_y_left)
        s_raw_y_left = raw_y;

    /* Screen Y (0 top .. 319 bottom) = inverted register X */
    span = s_raw_x_top - s_raw_x_bottom;
    pos = (uint32_t)(s_raw_x_top - raw_x) * (FT6336_SCR_HEIGHT - 1u);
    mapped_y = (span > 0) ? (uint16_t)(pos / span) : 0u;

    /* Screen X (0 left .. 479 right) = register Y */
    span = s_raw_y_right - s_raw_y_left;
    pos = (uint32_t)(raw_y - s_raw_y_left) * (FT6336_SCR_WIDTH - 1u);
    mapped_x = (span > 0) ? (uint16_t)(pos / span) : 0u;

    if (x)
        *x = mapped_x;
    if (y)
        *y = mapped_y;
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
