/* add user code begin Header */
/**
 ******************************************************************************
 * File Name          : freertos_app.h
 * Description        : Code for freertos applications
 */
/* add user code end Header */
  
#ifndef FREERTOS_APP_H
#define FREERTOS_APP_H

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"
#include "event_groups.h"
#include "wk_system.h"

/* private includes -------------------------------------------------------------*/
/* add user code begin private includes */
#include "at32f403a_407_wk_config.h"
#include <EventRecorder.h>
#include "pc_handle.h"

#include "beep_handle.h"

#include "output_handle.h"
#include "input_handle.h"
/* add user code end private includes */

/* exported types -------------------------------------------------------------*/
/* add user code begin exported types */

/* add user code end exported types */

/* exported constants --------------------------------------------------------*/
/* add user code begin exported constants */

/* add user code end exported constants */

/* exported macro ------------------------------------------------------------*/
/* add user code begin exported macro */

/* add user code end exported macro */

/* task handler */
extern TaskHandle_t ui_task_handle;
extern TaskHandle_t input_task_handle;
extern TaskHandle_t output_task_handle;
extern TaskHandle_t common_task_handle;
extern TaskHandle_t wdt_task_handle;
/* declaration for task function */
void ui_function(void *pvParameters);
void input_function(void *pvParameters);
void output_function(void *pvParameters);
void common_function(void *pvParameters);
void wdt_function(void *pvParameters);

/* queue handler */
extern QueueHandle_t usart_1_tx_handle;
extern QueueHandle_t usart_1_rx_handle;

/* add user code begin 0 */

/* LVGL touch globals (freertos_app.c) */
extern volatile bool     lvgl_touch_state;
extern volatile uint16_t lvgl_touch_x;
extern volatile uint16_t lvgl_touch_y;

/* add user code end 0 */

void freertos_task_create(void);
void freertos_queue_create(void);
void wk_freertos_init(void);

/* add user code begin 1 */

/* add user code end 1 */

#endif /* FREERTOS_APP_H */
