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
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(lv_coord_t * x, lv_coord_t * y);

static void mouse_init(void);
static void mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static bool mouse_is_pressed(void);
static void mouse_get_xy(lv_coord_t * x, lv_coord_t * y);

static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static void encoder_handler(void);

static void button_init(void);
static void button_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static int8_t button_get_pressed_id(void);
static bool button_is_pressed(uint8_t id);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;
lv_indev_t * indev_mouse;
lv_indev_t * indev_keypad;
lv_indev_t * indev_encoder;
lv_indev_t * indev_button;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/

    /*Initialize your touchpad if you have*/
    touchpad_init();

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
 
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the touchpad*/
 
static void touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    /* Take a consistent snapshot: lvgl_touch_* are written by input_task and
     * read here, so sample them once inside a short critical section.
     * They are declared volatile in freertos_app.c - without that the
     * compiler is allowed to cache them and touch dies under optimisation. */
    bool     pressed;
    uint16_t tx, ty;

    taskENTER_CRITICAL();
    pressed = lvgl_touch_state;
    tx      = lvgl_touch_x;
    ty      = lvgl_touch_y;
    taskEXIT_CRITICAL();

    if (pressed) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = (lv_coord_t)tx;
        data->point.y = (lv_coord_t)ty;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    /* ---- LVGL-side diagnostics: proves what the indev callback receives ----
     * Disabled: printf() blocks the UART for ~3 ms per line and stalls the
     * LVGL input pipeline. Change the 0 to 1 to debug the indev path. */
#if 0
    {
        static uint32_t dbg_t;
        if (xTaskGetTickCount() - dbg_t >= pdMS_TO_TICKS(1000))
        {
            dbg_t = xTaskGetTickCount();
            printf("[INDEV] state=%u x=%d y=%d\r\n",
                   (unsigned)data->state, (int)data->point.x, (int)data->point.y);
        }
    }
#endif
}
 
#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
