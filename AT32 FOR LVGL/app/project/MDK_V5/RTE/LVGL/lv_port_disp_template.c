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
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 480 is used for now.
    #define MY_DISP_HOR_RES    480
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 320 is used for now.
    #define MY_DISP_VER_RES    320
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void disp_wait_cb(lv_disp_drv_t * disp_drv);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
//    static lv_disp_draw_buf_t draw_buf_dsc_1;
//    static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 2) */
    static lv_disp_draw_buf_t draw_buf_dsc_2;
    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 20];                        /*A buffer for 20 rows*/
    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 20];                        /*An other buffer for 20 rows*/
    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 20);   /*Initialize the display buffer*/

    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
//    static lv_disp_draw_buf_t draw_buf_dsc_3;
//    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
//    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
//                          MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Called by LVGL while it waits for the previous DMA flush to finish
     *(draw_buf_flush's `while(flushing)` loop) - completes the flush there*/
    disp_drv.wait_cb = disp_wait_cb;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_2;

    /*Required for Example 3)*/
    //disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
	
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/* Asynchronous flush pipeline: disp_flush() starts the DMA, disp_wait_cb()
 * completes it (see the LVGL 8.4 draw_buf_flush() wait loop below). */

/* Asynchronous flush state.
 *
 * disp_flush() only *starts* the DMA transfer and returns immediately, so the
 * CPU is free to render the next LVGL area (double buffer) while the pixels
 * are on the bus.  lv_disp_flush_ready() is deliberately NOT called from the
 * DMA ISR - LVGL is not re-entrant against its own render pass and that
 * crashed under -O1/-O2.
 *
 * LVGL 8.4 draws_buf_flush() waits for the previous flush with
 *     while(draw_buf->flushing) { if(drv->wait_cb) drv->wait_cb(drv); }
 * inside lv_timer_handler(), so the ONLY correct place to complete a flush
 * is disp_wait_cb() below - it runs in the same task as lv_timer_handler and
 * unblocks that while loop.  Completing the flush from anywhere else (e.g. a
 * poll after lv_timer_handler returns) deadlocks: the while never exits, so
 * the code after lv_timer_handler never runs. */

/*Flush the content of the internal buffer the specific area on the display*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(disp_flush_enabled) {
        /* window + async DMA: kick the transfer off and return immediately.
         * lcd_dma_flush() waits for a previous transfer to finish first, so
         * at most one DMA is in flight.  The DMA buffer (LVGL's draw buffer)
         * stays untouched until disp_wait_cb() reports the transfer done and
         * lv_disp_flush_ready() releases it. */
        lcd_set_block((uint16_t)area->x1, (uint16_t)area->y1,
                      (uint16_t)area->x2, (uint16_t)area->y2);

        uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
        uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
        lcd_dma_flush((uint16_t *)color_p, w * h * sizeof(lv_color_t),
                      NULL, NULL);
    }
    else {
        /* flushing disabled: still must release LVGL or it hangs */
        lv_disp_flush_ready(disp_drv);
    }
}

/* Called by LVGL from inside draw_buf_flush()'s `while(draw_buf->flushing)`
 * wait loop.  Complete the pending DMA flush here: once the pixels are out
 * on the bus, tell LVGL the buffer is free (this is the same task context as
 * lv_timer_handler, which is the only safe context for it). */
static void disp_wait_cb(lv_disp_drv_t * disp_drv)
{
    if(lcd_dma_flush_done()) {
        lv_disp_flush_ready(disp_drv);
    }
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
