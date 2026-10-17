/* add user code begin Header */
/**
 ******************************************************************************
 * File Name          : freertos_app.c
 * Description        : Code for freertos applications
 */
/* add user code end Header */

/* Includes ------------------------------------------------------------------*/
#include "freertos_app.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "Inf_key.h"
#include "Inf_encoder.h"
#include "Inf_touch.h"
#include "iap.h"

/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */
#define TASK_UI_IDX 1
#define TASK_INPUT_IDX 2
#define TASK_OUTPUT_IDX 3
#define TASK_COMMON_IDX 4
/* add user code end private define */

/* private macro -------------------------------------------------------------*/
/* add user code begin private macro */

/* add user code end private macro */

/* private variables ---------------------------------------------------------*/
/* add user code begin private variables */
/* LVGL touch: updated by input_task, read by indev callback.
 * MUST be volatile: written by one task (input_task) and read by another
 * (ui_task -> LVGL read_cb). Without volatile the compiler may cache them in
 * a register across calls, which kills touch at any -O level above -O0. */
volatile bool lvgl_touch_state = false;
volatile uint16_t lvgl_touch_x = 0;
volatile uint16_t lvgl_touch_y = 0;

/* add user code end private variables */

/* private function prototypes --------------------------------------------*/
/* add user code begin function prototypes */
volatile uint32_t task_life_signs[5] = {0, 0, 0, 0, 0};

/* add user code end function prototypes */

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

 
/* add user code end 0 */

/* task handler */
TaskHandle_t ui_task_handle;
TaskHandle_t input_task_handle;
TaskHandle_t output_task_handle;
TaskHandle_t common_task_handle;
TaskHandle_t wdt_task_handle;

/* Idle task control block and stack */
static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];
static StackType_t timer_task_stack[configTIMER_TASK_STACK_DEPTH];

static StaticTask_t idle_task_tcb;
static StaticTask_t timer_task_tcb;

/* External Idle and Timer task static memory allocation functions */
extern void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
extern void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer, StackType_t ** ppxTimerTaskStackBuffer, uint32_t * pulTimerTaskStackSize );

/*
  vApplicationGetIdleTaskMemory gets called when configSUPPORT_STATIC_ALLOCATION
  equals to 1 and is required for static memory allocation support.
*/
void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &idle_task_tcb;
  *ppxIdleTaskStackBuffer = &idle_task_stack[0];
  *pulIdleTaskStackSize = (uint32_t)configMINIMAL_STACK_SIZE;
}
/*
  vApplicationGetTimerTaskMemory gets called when configSUPPORT_STATIC_ALLOCATION
  equals to 1 and is required for static memory allocation support.
*/
void vApplicationGetTimerTaskMemory( StaticTask_t ** ppxTimerTaskTCBBuffer, StackType_t ** ppxTimerTaskStackBuffer, uint32_t * pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &timer_task_tcb;
  *ppxTimerTaskStackBuffer = &timer_task_stack[0];
  *pulTimerTaskStackSize = (uint32_t)configTIMER_TASK_STACK_DEPTH;
}

/* add user code begin 1 */

/* add user code end 1 */

/**
  * @brief  initializes all task.
  * @param  none
  * @retval none
  */
void freertos_task_create(void)
{
  /* create ui_task task */
  xTaskCreate(ui_function,
              "ui_task",
              2048,
              NULL,
              0,
              &ui_task_handle);

  /* create input_task task */
  xTaskCreate(input_function,
              "input_task",
              512,
              NULL,
              0,
              &input_task_handle);

  /* create output_task task */
  xTaskCreate(output_function,
              "output_task",
              128,
              NULL,
              0,
              &output_task_handle);

  /* create common_task task */
  xTaskCreate(common_function,
              "common_task",
              128,
              NULL,
              0,
              &common_task_handle);

  /* create wdt_task task */
  xTaskCreate(wdt_function,
              "wdt_task",
              128,
              NULL,
              1,
              &wdt_task_handle);
}

/**
  * @brief  freertos init and begin run.
  * @param  none
  * @retval none
  */
void wk_freertos_init(void)
{
  /* add user code begin freertos_init 0 */
  /* add user code end freertos_init 0 */

  /* enter critical */
  taskENTER_CRITICAL();

  freertos_task_create();
	
  /* add user code begin freertos_init 1 */
  /* add user code end freertos_init 1 */

  /* exit critical */
  taskEXIT_CRITICAL();

  /* start scheduler */
  vTaskStartScheduler();
}

/**
  * @brief ui_task function.
  * @param  none
  * @retval none
  */
void ui_function(void *pvParameters)
{
  /* add user code begin ui_function 0 */
 
  lv_init();
  lv_port_disp_init();
  lv_port_indev_init();  

  /* add user code end ui_function 0 */

  /* add user code begin ui_function 2 */
    //lv_demo_benchmark();
	lv_demo_widgets();
 
  /* add user code end ui_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin ui_function 1 */
    task_life_signs[TASK_UI_IDX]++;

    /* Run LVGL timers (rendering + refresh at LV_DISP_DEF_REFR_PERIOD).
     * disp_flush() only starts the DMA; the flush is completed inside
     * lv_timer_handler itself via disp_drv.wait_cb (LVGL waits for it in
     * draw_buf_flush's while(flushing) loop). */
    lv_timer_handler_run_in_period(5);
    vTaskDelay(5);
  /* add user code end ui_function 1 */
  }
}


/**
  * @brief input_task function.
  * @param  none
  * @retval none
  */
void input_function(void *pvParameters)
{
  /* add user code begin input_function 0 */
  /* add user code end input_function 0 */

  /* add user code begin input_function 2 */
  ft6336_init();
  key_init();
  encoder_init();
 
  /* add user code end input_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin input_function 1 */
    task_life_signs[TASK_INPUT_IDX]++;
	  
	  
	  /* Sample into locals first: lvgl_touch_x/y are volatile, passing their
	   * address to a uint16_t* parameter would drop the qualifier. */
	  uint16_t touch_x = 0, touch_y = 0;
	  uint8_t touch_rc = ft6336_read_touch(&touch_x, &touch_y);
      
      if (touch_rc ) {
        lvgl_touch_x = touch_x;
        lvgl_touch_y = touch_y;
        lvgl_touch_state = true;
 
      } else {
        lvgl_touch_state = false;
      }
	  
    /* ---- touch diagnostics: exposes every layer of the chain, 1 line/sec ----
     * Disabled: the printf blocks the UART for ~6 ms per line (a visible
     * once-per-second hitch). Change the 0 to 1 to bring it back when
     * debugging the touch chain.
     *   bus_err=0 & id=0xA3 -> I2C link is healthy (layer 1 OK)
     *   rc=1 while touching   -> frame parsed, raw values in x/y (layer 2 OK)
     *   fault = middleware error_code: 0=OK, 1..12=ERR_STEP_1..12,
     *          13=START, 14=ADDR10, 15=ADDR, 16=STOP, 17=ACKFAIL,
     *          18=TIMEOUT, 19=INTERRUPT. */
#if 0
    {
      static uint32_t dbg_t;
      if (xTaskGetTickCount() - dbg_t >= pdMS_TO_TICKS(1000))
      {
        uint8_t cid = 0xFF, vid = 0xFF, sts = 0xFF;
        uint8_t bus_err;

        dbg_t = xTaskGetTickCount();
        bus_err = ft6336_diag(&cid, &vid, &sts);

        printf("[TOUCH] rc=%u x=%u y=%u | bus_err=%u id=%02X vid=%02X sts=%02X | fault=%02X\r\n",
               touch_rc, lvgl_touch_x, lvgl_touch_y,
               bus_err, cid, vid, sts, ft6336_get_last_fault());
      }
    }
#else
    (void)touch_rc;   /* keep the variable referenced with diagnostics off */
#endif

    vTaskDelay(5);

    /* add user code end input_function 1 */
  }
}


/**
  * @brief output_task function.
  * @param  none
  * @retval none
  */
void output_function(void *pvParameters)
{
  /* add user code begin output_function 0 */

  static TickType_t last_beep_time = 0;
  sbeep.status = BEEP_SHORT;
  /* add user code end output_function 0 */

  /* add user code begin output_function 2 */
  vTaskDelay(200);
 
  /* add user code end output_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin output_function 1 */
    task_life_signs[TASK_OUTPUT_IDX]++;
    TickType_t now = xTaskGetTickCount();

    if (spa45.flash_updata)
    {

      vTaskDelay(1);
      continue;
    }

    if ((now - last_beep_time) >= pdMS_TO_TICKS(BEEP_TASK_TIME))
    {
      if (spa45.function.speaker)
      {
        beep_handle();
      }
      else
      {
      }

      last_beep_time = now;
    }

    vTaskDelay(5);
    /* add user code end output_function 1 */
  }
}


/**
  * @brief common_task function.
  * @param  none
  * @retval none
  */
void common_function(void *pvParameters)
{
  /* add user code begin common_function 0 */
  /* 上位机的实时曲线数据从这里定时送出，周期固定10ms，不要改时间周期，数据库文件按照10ms存储的，目前还没做其他时间周期的兼容。 */
  TickType_t last_usb_tick = xTaskGetTickCount();
  const TickType_t usb_period = pdMS_TO_TICKS(500);

  /* add user code end common_function 0 */

  /* add user code begin common_function 2 */
  iap_init();

  /* add user code end common_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin common_function 1 */
    task_life_signs[TASK_COMMON_IDX]++;
    /* iap function */
    iap_command_handle();
    if (spa45.flash_updata)
    {
      /* close output */

      vTaskDelay(1);
      continue;
    }
    gpio_bits_toggle(OUT_LED_H_GPIO_PORT,OUT_LED_H_PIN);
    vTaskDelay(1000);    
    
    /* add user code end common_function 1 */
  }
}


/**
  * @brief wdt_task function.
  * @param  none
  * @retval none
  */
void wdt_function(void *pvParameters)
{
  /* add user code begin wdt_function 0 */
  uint32_t last_signs[5] = {0};

  /* add user code end wdt_function 0 */

  /* add user code begin wdt_function 2 */
  // wdt_enable();
  /* add user code end wdt_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin wdt_function 1 */
    vTaskDelay(600);
    taskENTER_CRITICAL();
    int all_alive = 1;

    for (int i = 1; i <= 4; i++)
    {
      if (task_life_signs[i] == last_signs[i])
      {
        all_alive = 0;
        break;
      }
    }

    if (all_alive)
    {
      for (int i = 1; i <= 4; i++)
      {
        last_signs[i] = task_life_signs[i];
      }

      wdt_counter_reload();
    }

    taskEXIT_CRITICAL();
    /* add user code end wdt_function 1 */
  }
}


/* add user code begin 2 */

/* add user code end 2 */

