/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp_template.h"
#include <stdbool.h>
#include "Inf_lcd.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *  STATIC VARIABLES
 **********************/
/* Global: saved disp_drv pointer for DMA ISR async callback */
volatile lv_disp_drv_t *lvgl_async_disp_drv = NULL;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*---------------------------------------------------
     * Initialize your LCD
     *--------------------------------------------------*/
    disp_init();

    /*---------------------------------------------------
     * Create double draw buffers for async DMA
     * (LVGL renders to one while DMA transfers the other)
     *--------------------------------------------------*/
    static lv_color_t buf_1_1[MY_DISP_HOR_RES * 10];      /* Buffer A: 854 x 10 x 2 = 17 KB */
    static lv_color_t buf_1_2[MY_DISP_HOR_RES * 10];      /* Buffer B: 854 x 10 x 2 = 17 KB */
    static lv_disp_draw_buf_t draw_buf_dsc;
    lv_disp_draw_buf_init(&draw_buf_dsc, buf_1_1, buf_1_2, MY_DISP_HOR_RES * 10);  /* Double buffer */

    /*---------------------------------------------------
     * Register the display in LVGL
     *--------------------------------------------------*/
    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                           /*Basic initialization*/

    /*Set the resolution of the display (landscape: 854x480)*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display*/
static void disp_init(void)
{
    /* LCD already initialized by xmc_init() + lcd_init() in main(). */
}

/*Flush the content of the internal buffer on the display.
 * LVGL v8 callback: color_p is lv_color_t* (uint16_t for RGB565).
 * Async DMA: starts transfer and returns immediately.
 * DMA ISR (DMA1_Channel3) calls lv_disp_flush_ready() when done.
 * This allows LVGL to render into the OTHER buffer while DMA runs. */
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    /* Save for DMA ISR callback */
    lvgl_async_disp_drv = disp_drv;

    /* 1. Set target window on LCD */
    lcd_set_block(area->x1, area->y1, area->x2, area->y2);

    /* 2. Start async DMA transfer (returns immediately) */
    uint32_t pixel_count = (uint32_t)(area->x2 - area->x1 + 1) * (uint32_t)(area->y2 - area->y1 + 1);
    lcd_wr_data_dma_async((uint16_t *)color_p, pixel_count * 2);

    /* 3. DO NOT call lv_disp_flush_ready()!
     *    DMA ISR will signal completion, so LVGL can render to the
     *    other buffer in parallel with the DMA transfer. */
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic. */
typedef int make_empty_compilation_unit_warning_quiet;
#endif
