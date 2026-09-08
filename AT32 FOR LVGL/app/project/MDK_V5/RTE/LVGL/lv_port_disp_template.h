/**
 * @file lv_port_disp_templ.h
 *
 */

/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES 480
#define MY_DISP_VER_RES 320

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void lv_port_disp_init(void);

/* Global display driver pointer for DMA ISR async flush callback */
extern volatile lv_disp_drv_t *lvgl_async_disp_drv;

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*End of "Content enable"*/
