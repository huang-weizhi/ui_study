/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev_template.h"
//#include "Inf_touch.h"
#include "freertos_app.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_indev_t * indev_touchpad;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/
    touchpad_init();

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your touchpad (FT6336 already initialized in input_task) */
static void touchpad_init(void)
{
    /* FT6336 is already initialized by ft6336_init() in input_function */
}

/* Read touch state for LVGL (data refreshed by input_task) */
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    if (lvgl_touch_state) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = lvgl_touch_x;
        data->point.y = lvgl_touch_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic. */
typedef int make_empty_compilation_unit_warning_quiet;
#endif
