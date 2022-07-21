/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    04/16/2010
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "led_driver.h"
#include "stm32f0xx_conf.h"
#include "debounce.h"
#include "delay.h"
#include "msata_pci.h"
#include "wan_lan_pci_status.h"
#include "slave_i2c_device.h"
#include "power_control.h"
#include "debug_serial.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    delay_timing_decrement();
}

/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/

/**
  * @brief  This function handles TIM16 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM16_IRQHandler(void)
{
    if (TIM_GetITStatus(DEBOUNCE_TIMER, TIM_IT_Update) != RESET)
    {
        debounce_input_timer_handler();
        TIM_ClearITPendingBit(DEBOUNCE_TIMER, TIM_IT_Update);
    }
}

/**
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(LED_TIMER, TIM_IT_Update) != RESET)
    {
        led_driver_send_frame();
        TIM_ClearITPendingBit(LED_TIMER, TIM_IT_Update);
    }
}


/**
  * @brief  This function handles TIM17 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM17_IRQHandler(void)
{
    struct st_i2c_status *i2c_control = &i2c_status;

    if (TIM_GetITStatus(USB_TIMEOUT_TIMER, TIM_IT_Update) != RESET)
    {
        power_control_usb(USB3_PORT0, USB_ON);
        power_control_usb(USB3_PORT1, USB_ON);

        i2c_control->status_word |= STS_USB30_PWRON | STS_USB31_PWRON;

        power_control_usb_timeout_disable();

        TIM_ClearITPendingBit(USB_TIMEOUT_TIMER, TIM_IT_Update);
    }
}

/**
  * @brief  This function handles I2C global interrupt request.
  * @param  None
  * @retval None
  */
void I2C2_IRQHandler(void)
{
    slave_i2c_handler();
}

/**
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM6_IRQHandler(void)
{
    if (TIM_GetITStatus(LED_EFFECT_TIMER, TIM_IT_Update) != RESET)
    {
        led_driver_knight_rider_effect_handler();
        TIM_ClearITPendingBit(LED_EFFECT_TIMER, TIM_IT_Update);
    }
}

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
