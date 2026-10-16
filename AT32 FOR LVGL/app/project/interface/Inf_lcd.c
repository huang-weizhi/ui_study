/*=============================================================================
 * Inf_lcd.c - LCD driver (LVGL backend only)
 *
 *   Simplified version, keeps only:
 *     - lcd_init()              : reset + ILI-style init sequence + 480x320 window
 *     - lcd_set_block()         : active window
 *     - lcd_dma_flush()         : async DMA1_CH3 flush (LVGL flush backend)
 *     - lcd_clear(_dma)         : full screen fill
 *   Removed all self-drawn rendering (fonts/graphics/text/digits/flash images).
 *   Note: the panel parameters are unified to 480(W)x320(H). If the image is
 *       mirrored/rotated at power-up, change the scan argument of
 *       lcd_set_display_mode at the end of lcd_init().
 *===========================================================================*/
#include "at32f403a_407.h"
#include "Inf_lcd.h"

/*============================================================================
 *                          Internal display info
 *============================================================================*/
typedef struct
{
    uint16_t width;  /* display width in the current orientation */
    uint16_t height; /* display height in the current orientation */
} lcd_screen_t;

static lcd_screen_t g_lcd = {LCD_LANDSCAPE_WIDTH, LCD_LANDSCAPE_HEIGHT};

/*============================================================================
 *                       Low level register writes
 *============================================================================*/
void lcd_wr_command(uint16_t command)
{
    *(__IO uint16_t *)XMC_LCD_COMMAND = command;
}

void lcd_wr_data(uint16_t data)
{
    *(__IO uint16_t *)XMC_LCD_DATA = data;
}

/*============================================================================
 *               Display orientation / window range setup
 *============================================================================*/
/**
 * @brief  Set the MADCTL scan direction and the column/page (Gram) range of
 *         the 480x320 panel.
 * @param  display_mode : LCD_DISPLAY_LANDSCAPE (480x320) / PORTRAIT (320x480)
 * @param  scan_mode    : scan direction; if the image is mirrored/flipped on
 *                        the real hardware, adjust the parameter here
 * @note   Column (0x2A) X: 0..479 (0x01DF), page (0x2B) Y: 0..319 (0x013F),
 *         matching 480x320.
 */
static void lcd_set_display_mode(uint8_t display_mode, uint8_t scan_mode)
{
    uint8_t madctl_value;

    /* scan direction | BGR bit (0x08, common for RGB565) */
    madctl_value = scan_mode | 0x08;

    lcd_wr_command(0x36);
    lcd_wr_data(madctl_value);

    if (display_mode == LCD_DISPLAY_LANDSCAPE)
    {
        /* Landscape: width 480 height 320 => columns 0..0x01DF (479),
         * pages 0..0x013F (319) */
        lcd_wr_command(0x2A); /* Column address set (X) */
        lcd_wr_data(0x00);
        lcd_wr_data(0x00);
        lcd_wr_data(0x01);
        lcd_wr_data(0xDF);
        lcd_wr_command(0x2B); /* Page address set (Y) */
        lcd_wr_data(0x00);
        lcd_wr_data(0x00);
        lcd_wr_data(0x01);
        lcd_wr_data(0x3F);
        g_lcd.width = LCD_LANDSCAPE_WIDTH;
        g_lcd.height = LCD_LANDSCAPE_HEIGHT;
    }
    else
    {
        /* Portrait: width 320 height 480 => columns 0..0x013F (319),
         * pages 0..0x01DF (479) */
        lcd_wr_command(0x2A); /* Column address set (X) */
        lcd_wr_data(0x00);
        lcd_wr_data(0x00);
        lcd_wr_data(0x01);
        lcd_wr_data(0x3F);
        lcd_wr_command(0x2B); /* Page address set (Y) */
        lcd_wr_data(0x00);
        lcd_wr_data(0x00);
        lcd_wr_data(0x01);
        lcd_wr_data(0xDF);
        g_lcd.width = LCD_PORTRAIT_WIDTH;
        g_lcd.height = LCD_PORTRAIT_HEIGHT;
    }
}

/*============================================================================
 *                              Window setup
 *============================================================================*/
void lcd_set_block(uint16_t xstart, uint16_t ystart, uint16_t xend, uint16_t yend)
{
    lcd_wr_command(0x2a); /* Set column address range (0x2A) */
    lcd_wr_data(xstart >> 8);
    lcd_wr_data(xstart);
    lcd_wr_data(xend >> 8);
    lcd_wr_data(xend);
    lcd_wr_command(0x2b); /* Set page address range (0x2B) */
    lcd_wr_data(ystart >> 8);
    lcd_wr_data(ystart);
    lcd_wr_data(yend >> 8);
    lcd_wr_data(yend);
    lcd_wr_command(0x2c); /* Enable memory write (0x2C) */
}

/*============================================================================
 *                         DMA pixel transfer
 *   DMA1_Channel3 is configured in wk_dma1_channel3_init(); its completion
 *   ISR must call lcd_dma_isr() (see at32f403a_407_int.c).
 *   Internal state machine: only one transfer may be in flight at a time.
 *============================================================================*/
static volatile uint8_t dma_busy;              /* 0 idle / 1 transfer running */
static volatile lcd_flush_done_cb_t dma_done_cb; /* pending completion callback */
static volatile void *dma_done_arg;            /* user data for the callback */

/** Kick off one DMA transfer; the caller must ensure the channel is idle. */
static void lcd_dma_tx_start(uint16_t *data, uint32_t length)
{
    dma_busy = 1;

    LCD_DMA_CHANNEL->dtcnt = length / 2; /* 16bit pixels, word count */
    LCD_DMA_CHANNEL->paddr = (uint32_t)data;
    LCD_DMA_CHANNEL->maddr = XMC_LCD_DATA;
    dma_channel_enable(LCD_DMA_CHANNEL, TRUE);
}

/** Blocking transfer used by the fill/clear helpers below. */
static void lcd_dma_tx_blocking(uint16_t *data, uint32_t length)
{
    while (dma_busy)
    {
    }
    lcd_dma_tx_start(data, length);
    while (dma_busy)
    {
    }
}

/** Asynchronous LVGL flush: wait for any previous transfer, then start this
 *  one and return; the callback fires from the DMA ISR when it completes. */
void lcd_dma_flush(uint16_t *data, uint32_t length, lcd_flush_done_cb_t done_cb, void *user_data)
{
    while (dma_busy)
    {
    }
    dma_done_cb = done_cb;
    dma_done_arg = user_data;
    lcd_dma_tx_start(data, length);
}

/** Block until the running DMA flush finishes. */
void lcd_dma_flush_wait(void)
{
    while (dma_busy)
    {
    }
}

/** DMA1_CH3 full-transfer ISR entry (clears flag, stops channel, fires cb). */
void lcd_dma_isr(void)
{
    lcd_flush_done_cb_t cb;
    void *arg;

    dma_flag_clear(DMA1_FDT3_FLAG);
    dma_channel_enable(LCD_DMA_CHANNEL, FALSE);
    dma_busy = 0;

    cb = dma_done_cb;
    dma_done_cb = NULL;
    arg = (void *)dma_done_arg;
    if (cb != NULL)
    {
        cb(arg);
    }
}

/*============================================================================
 *                            Full screen clear
 *============================================================================*/
#define CLEAR_BATCH_WORDS 2048 /* 2048 words per batch = 4KB */

/** Full screen DMA batch fill */
void lcd_clear_dma(uint16_t color)
{
    static uint16_t clear_buffer[CLEAR_BATCH_WORDS];
    uint32_t remaining, current;
    uint32_t i;

    lcd_set_block(0, 0, g_lcd.width - 1, g_lcd.height - 1);

    for (i = 0; i < CLEAR_BATCH_WORDS; i++)
        clear_buffer[i] = color;

    remaining = (uint32_t)g_lcd.width * g_lcd.height;
    while (remaining > 0)
    {
        current = (remaining > CLEAR_BATCH_WORDS) ? CLEAR_BATCH_WORDS : remaining;
        lcd_dma_tx_blocking(clear_buffer, current * 2); /* convert to byte count */
        remaining -= current;
    }
}

/** Full screen clear (synchronous) */
void lcd_clear(uint16_t color)
{
    lcd_clear_dma(color);
}

/*============================================================================
 *                           Rectangle drawing
 *============================================================================*/
#define LCD_FILL_BATCH_WORDS 1024 /* fill batch = 2KB */

/** Fill a rectangle (solid). x/y top-left, w/h size; clipped to the screen. */
void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    static uint16_t fill_buffer[LCD_FILL_BATCH_WORDS];
    uint32_t xend, yend, fill_w, fill_h, total, current, i;

    if (w == 0 || h == 0)
        return;
    if (x >= g_lcd.width || y >= g_lcd.height)
        return;

    /* clip into the visible area */
    xend = (uint32_t)x + w - 1;
    if (xend >= g_lcd.width)
        xend = g_lcd.width - 1;
    yend = (uint32_t)y + h - 1;
    if (yend >= g_lcd.height)
        yend = g_lcd.height - 1;
    fill_w = xend - x + 1;
    fill_h = yend - y + 1;

    for (i = 0; i < LCD_FILL_BATCH_WORDS; i++)
        fill_buffer[i] = color;

    lcd_set_block((uint16_t)x, (uint16_t)y, (uint16_t)xend, (uint16_t)yend);

    total = fill_w * fill_h;
    while (total > 0)
    {
        current = (total > LCD_FILL_BATCH_WORDS) ? LCD_FILL_BATCH_WORDS : total;
        lcd_dma_tx_blocking(fill_buffer, current * 2); /* convert to byte count */
        total -= current;
    }
}

/** Draw a 1px rectangle outline. x/y top-left, w/h outer size. */
void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (w == 0 || h == 0)
        return;

    if (w < 2 || h < 2)
    {
        lcd_fill_rect(x, y, w, h, color); /* too small for an outline */
        return;
    }

    lcd_fill_rect(x, y, w, 1, color);             /* top edge */
    lcd_fill_rect(x, (uint16_t)(y + h - 1), w, 1, color); /* bottom edge */
    lcd_fill_rect(x, y, 1, h, color);             /* left edge */
    lcd_fill_rect((uint16_t)(x + w - 1), y, 1, h, color); /* right edge */
}

/*============================================================================
 *                            LCD initialization
 *============================================================================*/
/**
 * @brief  Reset the LCD and send the init sequence, finally set the 480x320
 *         landscape mode and turn the display on.
 * @note   The init sequence follows the original 8080 16bit RGB565 parallel
 *         panel; when another panel model is used, replace it with the
 *         matching datasheet sequence.
 */
void lcd_init(void)
{
    wk_delay_ms(50);
    /* Hardware reset sequence */
    LCD_RESET_HIGH;
    wk_delay_ms(1);
    LCD_RESET_LOW;
    wk_delay_ms(10);
    LCD_RESET_HIGH;
    wk_delay_ms(120);

    /* LCD initialization commands */
    lcd_wr_command(0x36);
    lcd_wr_data(0x00);
    lcd_wr_command(0x3a);
    lcd_wr_data(0x55);
    lcd_wr_command(0xC0);
    lcd_wr_data(0x0F);
    lcd_wr_data(0x0F);      /* 4.5V */
    lcd_wr_command(0xC1);
    lcd_wr_data(0x41);      /* VGH = 6*VCI   VGL = -4*VCI */
    lcd_wr_command(0xC5);
    lcd_wr_data(0x00);
    lcd_wr_data(0x57);
    lcd_wr_data(0x80);
    lcd_wr_command(0xB1);
    lcd_wr_data(0xA0);
    lcd_wr_command(0xB4);
    lcd_wr_data(0x02);      /* 2 DOT */
    lcd_wr_command(0x36);
    lcd_wr_data(0x48);
    lcd_wr_command(0x3A);
    lcd_wr_data(0x55);      /* 55  66 */
    lcd_wr_command(0x21);
    lcd_wr_data(0x00);      /* IPS mode */
    lcd_wr_command(0xE9);
    lcd_wr_data(0x00);
    lcd_wr_command(0xF7);
    lcd_wr_data(0xA9);
    lcd_wr_data(0x51);
    lcd_wr_data(0x2C);
    lcd_wr_data(0x82);
    lcd_wr_command(0xE0);   /* Positive gamma correction */
    lcd_wr_data(0x00);
    lcd_wr_data(0x09);
    lcd_wr_data(0x0C);
    lcd_wr_data(0x02);
    lcd_wr_data(0x0E);
    lcd_wr_data(0x05);
    lcd_wr_data(0x2F);
    lcd_wr_data(0x46);
    lcd_wr_data(0x47);
    lcd_wr_data(0x04);
    lcd_wr_data(0x0E);
    lcd_wr_data(0x0B);
    lcd_wr_data(0x2D);
    lcd_wr_data(0x36);
    lcd_wr_data(0x0F);
    lcd_wr_command(0xE1);   /* Negative gamma correction */
    lcd_wr_data(0x00);
    lcd_wr_data(0x10);
    lcd_wr_data(0x13);
    lcd_wr_data(0x03);
    lcd_wr_data(0x10);
    lcd_wr_data(0x06);
    lcd_wr_data(0x3A);
    lcd_wr_data(0x46);
    lcd_wr_data(0x51);
    lcd_wr_data(0x06);
    lcd_wr_data(0x10);
    lcd_wr_data(0x0D);
    lcd_wr_data(0x33);
    lcd_wr_data(0x37);
    lcd_wr_data(0x0F);

    /* 480x320 landscape + scan direction (TB_LR); change the second argument
     * if the image orientation is wrong */
    lcd_set_display_mode(LCD_DISPLAY_LANDSCAPE, LCD_SCAN_TB_LR);

    lcd_wr_command(0x11); /* Sleep out */
    wk_delay_ms(120);
    lcd_wr_command(0x29); /* Display on */

    dma_busy = 0; /* DMA state machine starts idle */
    dma_done_cb = NULL;
    dma_done_arg = NULL;
}
