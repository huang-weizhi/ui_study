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

/* queue handler */
QueueHandle_t usart_1_tx_handle;
QueueHandle_t usart_1_rx_handle;

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
              1024,
              NULL,
              0,
              &ui_task_handle);

  /* create input_task task */
  xTaskCreate(input_function,
              "input_task",
              256,
              NULL,
              0,
              &input_task_handle);

  /* create output_task task */
  xTaskCreate(output_function,
              "output_task",
              256,
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
  * @brief  initializes all queue.
  * @param  none
  * @retval none
  */
void freertos_queue_create(void)
{
  /* Create the usart_1_tx, storing the returned handle in the xQueue variable. */
  usart_1_tx_handle = xQueueCreate(256, sizeof(uint8_t));

  /* Create the usart_1_rx, storing the returned handle in the xQueue variable. */
  usart_1_rx_handle = xQueueCreate(2048, sizeof(uint8_t));
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

  freertos_queue_create();
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
  /* add user code end ui_function 0 */

  /* add user code begin ui_function 2 */

  /* add user code end ui_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin ui_function 1 */
    task_life_signs[TASK_UI_IDX]++;

    if (spa45.flash_updata)
    {
      /* 升级期间暂停 UI 刷新(升级提示后续由 LVGL 页面显示) */
      vTaskDelay(2);
      continue;
    }

    /* LVGL 接入后: 在此周期调用 lv_timer_handler() 驱动界面刷新 */
    vTaskDelay(2);
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
	
  int8_t key;
  int16_t enc;
  int16_t ab;
  uint16_t tx, ty;
  uint8_t down;
  uint32_t slow_tick = 0;
	
  /* LVGL 接入后: 此任务周期轮询 ft6336_read_touch() 并刷新触摸全局变量 */
  /* add user code end input_function 2 */

  /* Infinite loop */
  while(1)
  {
  /* add user code begin input_function 1 */
    task_life_signs[TASK_INPUT_IDX]++;

    /* Encoder quadrature is decoded in the EXINT9_5 ISR (both A/B edges), so
     * no fast polling is needed here; just read keys/touch/encoder slowly. */
    slow_tick++;
    if ((slow_tick % 5u) == 0u)           /* every ~10 ms */
    {
      key = key_test_read();              /* 0 none / 1..4 KEY1..4 / 5 enc mid */
      down = ft6336_test_read(&tx, &ty);
      ab = encoder_test_read_ab();        /* optional: raw phases cycle 0-1-3-2 */
      enc = encoder_get_diff();           /* +1/-1 per click since last read */
    }
	  
	  
    vTaskDelay(2); /* ~2 ms encoder scan, ~10 ms key/touch cadence */

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

    /* pc function */
    pc_comm_handle();
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

