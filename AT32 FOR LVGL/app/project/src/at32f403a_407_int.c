/* add user code begin Header */
/**
 **************************************************************************
 * @file     at32f403a_407_int.c
 * @brief    main interrupt service routines.
 **************************************************************************
 * Copyright (c) 2025, Artery Technology, All rights reserved.
 *
 * The software Board Support Package (BSP) that is made available to
 * download from Artery official website is the copyrighted work of Artery.
 * Artery authorizes customers to use, copy, and distribute the BSP
 * software and its related documentation for the purpose of design and
 * development in conjunction with Artery microcontrollers. Use of the
 * software is governed by this copyright notice and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
 * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
 * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
 * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
 * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
 *
 **************************************************************************
 */
/* add user code end Header */

/* includes ------------------------------------------------------------------*/
#include "at32f403a_407_int.h"
#include "wk_system.h"
#include "freertos_app.h"

/* private includes ----------------------------------------------------------*/
/* add user code begin private includes */
#include "PA45.h"

/* add user code end private includes */

/* private typedef -----------------------------------------------------------*/
/* add user code begin private typedef */

/* add user code end private typedef */

/* add user code end private typedef */

/* private define ------------------------------------------------------------*/
/* add user code begin private define */
#define FILTER_TIMES 10
/* add user code end private define */

/* private macro -------------------------------------------------------------*/
/* add user code begin private macro */

/* add user code end private macro */

/* private variables ---------------------------------------------------------*/
/* add user code begin private variables */
static uint16_t sample_buffer[FILTER_TIMES][ADC_CH_NUM];
static uint8_t sample_cnt = 0;
/* add user code end private variables */

/* private function prototypes --------------------------------------------*/
/* add user code begin function prototypes */

/* add user code end function prototypes */

/* private user code ---------------------------------------------------------*/
/* add user code begin 0 */

/* Called from HardFault_Handler's inline asm.
   r0 = stack pointer value (MSP or PSP according to EXC_RETURN),
   r1 = LR. Captures them into volatile locals so they survive & are
   visible in the debugger; extend with real register capture as needed.
   (Missed after a Workbench re-generation - kept here in a protected
   user block so regeneration does not drop it again.) */
/* Called from HardFault_Handler's inline asm.
   r0 = stack pointer value (MSP or PSP according to EXC_RETURN),
   r1 = LR. Captures them into volatile locals so they survive & are
   visible in the debugger; extend with real register capture as needed.
   `used` keeps the symbol alive under LTO even though only inline asm
   jumps to it (no C call-site). */
__attribute__((used)) void fault_capture(uint32_t sp, uint32_t lr)
{
    volatile uint32_t saved_sp = sp;
    volatile uint32_t saved_lr = lr;
    (void)saved_sp;
    (void)saved_lr;
}

/* add user code end 0 */

/* external variables ---------------------------------------------------------*/
/* add user code begin external variables */

/* add user code end external variables */

/**
  * @brief  this function handles nmi exception.
  * @param  none
  * @retval none
  */
void NMI_Handler(void)
{
  /* add user code begin NonMaskableInt_IRQ 0 */
  /* add user code end NonMaskableInt_IRQ 0 */

  /* add user code begin NonMaskableInt_IRQ 1 */
  /* add user code end NonMaskableInt_IRQ 1 */
}

/**
  * @brief  this function handles hard fault exception.
  * @param  none
  * @retval none
  */
void HardFault_Handler(void)
{
  /* add user code begin HardFault_IRQ 0 */
  __asm volatile
  (
      "tst   lr, #4        \n"
      "ite   eq            \n"
      "mrseq r0, msp       \n"
      "mrsne r0, psp       \n"
      "mov   r1, lr        \n"
      "b     fault_capture \n"
  );
  /* add user code end HardFault_IRQ 0 */
  /* go to infinite loop when hard fault exception occurs */
  while (1)
  {
    /* add user code begin W1_HardFault_IRQ 0 */
    /* add user code end W1_HardFault_IRQ 0 */
  }
}


/**
  * @brief  this function handles memory manage exception.
  * @param  none
  * @retval none
  */
void MemManage_Handler(void)
{
  /* add user code begin MemoryManagement_IRQ 0 */

  /* add user code end MemoryManagement_IRQ 0 */
  /* go to infinite loop when memory manage exception occurs */
  while (1)
  {
    /* add user code begin W1_MemoryManagement_IRQ 0 */
    /* add user code end W1_MemoryManagement_IRQ 0 */
  }
}

/**
  * @brief  this function handles bus fault exception.
  * @param  none
  * @retval none
  */
void BusFault_Handler(void)
{
  /* add user code begin BusFault_IRQ 0 */

  /* add user code end BusFault_IRQ 0 */
  /* go to infinite loop when bus fault exception occurs */
  while (1)
  {
    /* add user code begin W1_BusFault_IRQ 0 */
    /* add user code end W1_BusFault_IRQ 0 */
  }
}

/**
  * @brief  this function handles usage fault exception.
  * @param  none
  * @retval none
  */
void UsageFault_Handler(void)
{
  /* add user code begin UsageFault_IRQ 0 */

  /* add user code end UsageFault_IRQ 0 */
  /* go to infinite loop when usage fault exception occurs */
  while (1)
  {
    /* add user code begin W1_UsageFault_IRQ 0 */
    /* add user code end W1_UsageFault_IRQ 0 */
  }
}

/**
  * @brief  this function handles debug monitor exception.
  * @param  none
  * @retval none
  */
void DebugMon_Handler(void)
{
  /* add user code begin DebugMonitor_IRQ 0 */
  /* add user code end DebugMonitor_IRQ 0 */
  /* add user code begin DebugMonitor_IRQ 1 */
  /* add user code end DebugMonitor_IRQ 1 */
}

extern void xPortSysTickHandler(void);

/**
  * @brief  this function handles systick handler.
  * @param  none
  * @retval none
  */
void SysTick_Handler(void)
{
  /* add user code begin SysTick_IRQ 0 */
  /* add user code end SysTick_IRQ 0 */

#if (INCLUDE_xTaskGetSchedulerState == 1 )
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif /* INCLUDE_xTaskGetSchedulerState */
  xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState == 1 )
  }
#endif /* INCLUDE_xTaskGetSchedulerState */

  /* add user code begin SysTick_IRQ 1 */
  /* add user code end SysTick_IRQ 1 */
}

/**
  * @brief  this function handles DMA1 Channel 1 handler.
  * @param  none
  * @retval none
  */
void DMA1_Channel1_IRQHandler(void)
{
  /* add user code begin DMA1_Channel1_IRQ 0 */

  /* add user code end DMA1_Channel1_IRQ 0 */

  /* add user code begin DMA1_Channel1_IRQ 1 */

  /* add user code end DMA1_Channel1_IRQ 1 */
}

/**
  * @brief  this function handles DMA1 Channel 3 handler.
  * @param  none
  * @retval none
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* add user code begin DMA1_Channel3_IRQ 0 */

  /* add user code end DMA1_Channel3_IRQ 0 */

  if(dma_interrupt_flag_get(DMA1_FDT3_FLAG) != RESET)
  {   
    /* add user code begin DMA1_FDT3_FLAG */
    /* handle full data transfer and clear flag */
    dma_flag_clear(DMA1_FDT3_FLAG);
    dma_channel_enable(LCD_DMA_CHANNEL, FALSE);
    lcd_dma_transfer_complete = 1;

    /* add user code end DMA1_FDT3_FLAG */ 
  }

  /* add user code begin DMA1_Channel3_IRQ 1 */
  /* add user code end DMA1_Channel3_IRQ 1 */
}

/**
  * @brief  this function handles TMR1 Overflow and TMR10 handler.
  * @param  none
  * @retval none
  */
void TMR1_OVF_TMR10_IRQHandler(void)
{
  /* add user code begin TMR1_OVF_TMR10_IRQ 0 */

  /* add user code end TMR1_OVF_TMR10_IRQ 0 */

  wk_timebase_handler();

  /* add user code begin TMR1_OVF_TMR10_IRQ 1 */

  /* add user code end TMR1_OVF_TMR10_IRQ 1 */
}

/**
  * @brief  this function handles USART1 handler.
  * @param  none
  * @retval none
  */
void USART1_IRQHandler(void)
{
  /* add user code begin USART1_IRQ 0 */

  /* add user code end USART1_IRQ 0 */

  __IO uint16_t val_rx;

  if(usart_interrupt_flag_get(USART1, USART_TDC_FLAG) != RESET)
  {
    /* add user code begin USART1_USART_TDC_FLAG */
    /* clear flag */
    usart_flag_clear(USART1, USART_TDC_FLAG);
    /* add user code end USART1_USART_TDC_FLAG */
  }

  if(usart_interrupt_flag_get(USART1, USART_PERR_FLAG) != RESET)
  {
    /* add user code begin USART1_USART_PERR_FLAG */
    /* clear flag */
    usart_flag_clear(USART1, USART_PERR_FLAG);
    /* add user code end USART1_USART_PERR_FLAG */ 
  }

  if((usart_interrupt_flag_get(USART1, USART_NERR_FLAG) != RESET) || \
    (usart_interrupt_flag_get(USART1, USART_ROERR_FLAG) != RESET) || \
    (usart_interrupt_flag_get(USART1, USART_FERR_FLAG) != RESET))
  {
    /* add user code begin USART1_USART_NERR_FLAG, USART_ROERR_FLAG or USART_FERR_FLAG */
    /* clear flag */
    usart_flag_clear(USART1, USART_NERR_FLAG | USART_ROERR_FLAG | USART_FERR_FLAG);
    /* add user code end  USART1_USART_NERR_FLAG, USART_ROERR_FLAG or USART_FERR_FLAG */ 
  }

  if(usart_interrupt_flag_get(USART1, USART_RDBF_FLAG) != RESET)
  {
    /* add user code begin USART1_USART_RDBF_FLAG */
    /* handle data received and clear flag */
    val_rx = usart_data_receive(USART1);
    /* add user code end USART1_USART_RDBF_FLAG */ 
  }

  /* add user code begin USART1_IRQ 1 */

  /* add user code end USART1_IRQ 1 */
}

/* add user code begin 1 */

/* add user code end 1 */
