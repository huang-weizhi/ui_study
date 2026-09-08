/*=============================================================================
 * Inf_lcd.h - LCD driver interface (LVGL backend only)
 *
 *   Simplified version: keeps only the low level interfaces needed for
 *   LVGL flushing / clearing.
 *   Removed: ASCII font, graphics/text/digit/image rendering, external flash
 *   image tables and the like (all drawing is handed over to LVGL).
 *   Panel: 480(W) x 320(H), landscape.
 *===========================================================================*/
#ifndef __INF_LCD_H
#define __INF_LCD_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "at32f403a_407.h"
#include "wk_system.h"

    /*============================================================================
     *                         DISPLAY DIMENSIONS (480x320)
     *============================================================================*/

#define LCD_PORTRAIT_WIDTH 320  /**< Portrait width (px) */
#define LCD_PORTRAIT_HEIGHT 480 /**< Portrait height (px) */
#define LCD_LANDSCAPE_WIDTH 480 /**< Landscape width (px) */
#define LCD_LANDSCAPE_HEIGHT 320 /**< Landscape height (px) */

#define LCD_DISPLAY_PORTRAIT 0 /**< Portrait mode (320x480) */
#define LCD_DISPLAY_LANDSCAPE 1 /**< Landscape mode (480x320) */

    /*============================================================================
     *                     SCANNING DIRECTION MACROS
     *   (correspond to the MADCTL 0x36 scan-direction bits; the driver ORs in
     *   the 0x08 BGR bit internally)
     *============================================================================*/
#define LCD_SCAN_LR_TB 0x00
#define LCD_SCAN_LR_BT 0x80
#define LCD_SCAN_RL_TB 0x40
#define LCD_SCAN_RL_BT 0xC0
#define LCD_SCAN_TB_LR 0x20
#define LCD_SCAN_TB_RL 0x60
#define LCD_SCAN_BT_LR 0xA0
#define LCD_SCAN_BT_RL 0xE0

    /*============================================================================
     *                 XMC ADDRESS MAPPING (COMMAND / DATA)
     *============================================================================*/
#define XMC_LCD_COMMAND (0x60000000)
#define XMC_LCD_DATA (0x60000000 | 0x20000)

    /*============================================================================
     *                           DMA CONFIGURATION
     *============================================================================*/
/** DMA channel used for LCD data transfers */
#define LCD_DMA_CHANNEL DMA1_CHANNEL3
#define LCD_DMA_IRQn DMA1_Channel3_IRQn
#define LCD_DMA_IRQHandler DMA1_Channel3_IRQHandler

    /*============================================================================
     *                         HARDWARE RESET LINE
     *============================================================================*/
/** Drive LCD reset pin high (release reset) */
#define LCD_RESET_HIGH gpio_bits_set(GPIOC, GPIO_PINS_6)
/** Drive LCD reset pin low (assert reset) */
#define LCD_RESET_LOW gpio_bits_reset(GPIOC, GPIO_PINS_6)

    /*============================================================================
     *                           COLOR DEFINITIONS (RGB565)
     *============================================================================*/
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0xBC40
#define GRAY 0x8430

    /*============================================================================
     *                       FUNCTION PROTOTYPES
     *   The LCD is a 16bit XMC parallel interface (RGB565); no 8bit branch.
     *============================================================================*/
    void lcd_init(void); /**< hardware reset + init sequence + 480x320 landscape window */

    void lcd_wr_command(uint16_t command);
    void lcd_wr_data(uint16_t data);

    /** Set the active window for subsequent pixel writes (coordinates x/y are
     *  480x320 in the current orientation) */
    void lcd_set_block(uint16_t xstart, uint16_t ystart, uint16_t xend, uint16_t yend);

    /** Blocking DMA pixel transfer; length is the byte count (must be even,
     *  16bit pixels) */
    void lcd_wr_data_dma(uint16_t *data, uint32_t length);

    /** Non-blocking DMA transfer, returns immediately; used by LVGL async
     *  flushing. Must wait for lcd_dma_transfer_complete == 1 before calling
     *  again */
    void lcd_wr_data_dma_async(uint16_t *data, uint32_t length);

    /** DMA completion flag, set to 1 by DMA1_Channel3_IRQHandler */
    extern volatile uint8_t lcd_dma_transfer_complete;

    /** Clear the full screen (480x320) */
    void lcd_clear(uint16_t color);

    /** Clear the full screen with a DMA batch fill */
    void lcd_clear_dma(uint16_t color);

    /** Fill a rectangle (solid) with a color.
     *  x/y = top-left corner, w/h = size, in the current orientation
     *  (480x320 landscape). Out-of-screen parts are clipped. 
	 	 *   lcd_draw_rect(0,    0,   30, 30, RED);         //top left
     *   lcd_draw_rect(479-30, 0,   30, 30, GREEN);     //top right   
     *   lcd_draw_rect(0,    319-30, 30, 30, BLUE);     //botton left  
     *   lcd_draw_rect(479-30, 319-30, 30, 30, YELLOW); //botton right 
	 */
    void lcd_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

    /** Draw a 1px rectangle outline (border) in a color.
     *  x/y = top-left corner, w/h = outer size. Very small rects
     *  (w<2 or h<2) are drawn as a solid fill. 

	 */
    void lcd_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#ifdef __cplusplus
}

#endif

#endif /* __INF_LCD_H */
