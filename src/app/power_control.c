/**
 ******************************************************************************
 * @file    power_control.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    22-July-2015
 * @brief   Functions for control of DC/DC converters.
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_conf.h"
#include "power_control.h"
#include "delay.h"
#include "led_driver.h"

/* Private define ------------------------------------------------------------*/
//#define USE_4V5_POWER
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
  * @function   system_control_io_config
  * @brief      GPIO config for EN, PG, Reset and USB signals.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_io_config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(RES_RAM_PIN_PERIPH_CLOCK | ENABLE_5V_PIN_PERIPH_CLOCK |
       ENABLE_3V3_PIN_PERIPH_CLOCK | ENABLE_1V35_PIN_PERIPH_CLOCK |
       ENABLE_4V5_PIN_PERIPH_CLOCK | ENABLE_1V8_PIN_PERIPH_CLOCK |
       ENABLE_1V5_PIN_PERIPH_CLOCK | ENABLE_1V2_PIN_PERIPH_CLOCK |
       ENABLE_VTT_PIN_PERIPH_CLOCK | USB30_PWRON_PIN_PERIPH_CLOCK |
       USB31_PWRON_PIN_PERIPH_CLOCK, ENABLE);

    /* Output signals */
    GPIO_InitStructure.GPIO_Pin = RES_RAM_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(RES_RAM_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_5V_PIN;
    GPIO_Init(ENABLE_5V_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_3V3_PIN;
    GPIO_Init(ENABLE_3V3_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_1V35_PIN;
    GPIO_Init(ENABLE_1V35_PIN_PORT, &GPIO_InitStructure);
#ifdef USE_4V5_POWER
    GPIO_InitStructure.GPIO_Pin = ENABLE_4V5_PIN;
    GPIO_Init(ENABLE_4V5_PIN_PORT, &GPIO_InitStructure);
#endif
    GPIO_InitStructure.GPIO_Pin = ENABLE_1V8_PIN;
    GPIO_Init(ENABLE_1V8_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_1V5_PIN;
    GPIO_Init(ENABLE_1V5_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_1V2_PIN;
    GPIO_Init(ENABLE_1V2_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_VTT_PIN;
    GPIO_Init(ENABLE_VTT_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USB30_PWRON_PIN;
    GPIO_Init(USB30_PWRON_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USB31_PWRON_PIN;
    GPIO_Init(USB31_PWRON_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SYSRES_OUT_PIN;
    GPIO_Init(SYSRES_OUT_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = CFG_CTRL_PIN;
    GPIO_Init(CFG_CTRL_PIN_PORT, &GPIO_InitStructure);


    /* Input signals */

    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(MANRES_PIN_PERIPH_CLOCK | SYSRES_OUT_PIN_PERIPH_CLOCK |
                          DGBRES_PIN_PERIPH_CLOCK | MRES_PIN_PERIPH_CLOCK |
                          PG_5V_PIN_PERIPH_CLOCK | PG_3V3_PIN_PERIPH_CLOCK |
                          PG_1V35_PIN_PERIPH_CLOCK | PG_4V5_PIN_PERIPH_CLOCK |
                          PG_1V8_PIN_PERIPH_CLOCK | PG_1V5_PIN_PERIPH_CLOCK |
                          PG_1V2_PIN_PERIPH_CLOCK | PG_VTT_PIN_PERIPH_CLOCK |
                          USB30_OVC_PIN_PERIPH_CLOCK | USB31_OVC_PIN_PERIPH_CLOCK |
                          RTC_ALARM_PIN_PERIPH_CLOCK | LED_BRT_PIN_PERIPH_CLOCK,
                          ENABLE);

    GPIO_InitStructure.GPIO_Pin = MRES_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(MRES_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MANRES_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(MANRES_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = DGBRES_PIN;
    GPIO_Init(DGBRES_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_5V_PIN;
    GPIO_Init(PG_5V_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_3V3_PIN;
    GPIO_Init(PG_3V3_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_1V35_PIN;
    GPIO_Init(PG_1V35_PIN_PORT, &GPIO_InitStructure);
#ifdef USE_4V5_POWER
    GPIO_InitStructure.GPIO_Pin = PG_4V5_PIN;
    GPIO_Init(PG_4V5_PIN_PORT, &GPIO_InitStructure);
#endif
    GPIO_InitStructure.GPIO_Pin = PG_1V8_PIN;
    GPIO_Init(PG_1V8_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_1V5_PIN;
    GPIO_Init(PG_1V5_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_1V2_PIN;
    GPIO_Init(PG_1V2_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_VTT_PIN;
    GPIO_Init(PG_VTT_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USB30_OVC_PIN;
    GPIO_Init(USB30_OVC_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USB31_OVC_PIN;
    GPIO_Init(USB31_OVC_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = RTC_ALARM_PIN;
    GPIO_Init(RTC_ALARM_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = LED_BRT_PIN;
    GPIO_Init(LED_BRT_PIN_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(SYSRES_OUT_PIN_PORT, SYSRES_OUT_PIN);
    GPIO_SetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);
}

/*******************************************************************************
  * @function   power_control_enable_regulator
  * @brief      Starts DC/DC regulators.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_enable_regulator(void)
{
    /*
     * power-up sequence:
     * 1) 5V regulator
     * 2) 4.5V regulator - if used
     * 3) 3.3V regulator
     * 4) 1.8V regulator
     * 5) 1.5V regulator
     * 6) 1.35V regulator
     *    VTT regulator
     * 7) 1.2V regulator
     */
    GPIO_SetBits(ENABLE_5V_PIN_PORT, ENABLE_5V_PIN);
    while(!(GPIO_ReadInputDataBit(PG_5V_PIN_PORT, PG_5V_PIN)))
        ;

#ifdef USE_4V5_POWER
    GPIO_SetBits(ENABLE_4V5_PIN_PORT, ENABLE_4V5_PIN);
    while(!(GPIO_ReadInputDataBit(PG_4V5_PIN_PORT, PG_4V5_PIN)))
        ;
#endif

    GPIO_SetBits(ENABLE_3V3_PIN_PORT, ENABLE_3V3_PIN);
    while(!(GPIO_ReadInputDataBit(PG_3V3_PIN_PORT, PG_3V3_PIN)))
        ;

    GPIO_SetBits(ENABLE_1V8_PIN_PORT, ENABLE_1V8_PIN);
    while(!(GPIO_ReadInputDataBit(PG_1V8_PIN_PORT, PG_1V8_PIN)))
        ;

    GPIO_SetBits(ENABLE_1V5_PIN_PORT, ENABLE_1V5_PIN);
    while(!(GPIO_ReadInputDataBit(PG_1V5_PIN_PORT, PG_1V5_PIN)))
        ;

    GPIO_SetBits(ENABLE_1V35_PIN_PORT, ENABLE_1V35_PIN);
    GPIO_SetBits(ENABLE_VTT_PIN_PORT, ENABLE_VTT_PIN);
    while(!(GPIO_ReadInputDataBit(PG_1V35_PIN_PORT, PG_1V35_PIN)))
        ;
    while(!(GPIO_ReadInputDataBit(PG_VTT_PIN_PORT, PG_VTT_PIN)))
        ;

    GPIO_SetBits(ENABLE_1V2_PIN_PORT, ENABLE_1V2_PIN);
    while(!(GPIO_ReadInputDataBit(PG_1V2_PIN_PORT, PG_1V2_PIN)))
        ;
}

/*******************************************************************************
  * @function   power_control_usb
  * @brief      Enable / disable power supply for USB.
  * @param      usb_port: USB3_PORT0 or USB3_PORT1.
  * @param      usb_state: USB_ON or USB_OFF.
  * @retval     None.
  *****************************************************************************/
void power_control_usb(usb_ports_t usb_port, usb_state_t usb_state)
{
    if (usb_port == USB3_PORT0)
    {
        if (usb_state == USB_ON)
            GPIO_ResetBits(USB30_PWRON_PIN_PORT, USB30_PWRON_PIN);
        else
            GPIO_SetBits(USB30_PWRON_PIN_PORT, USB30_PWRON_PIN);
    }
    else //USB3_PORT1
    {
        if (usb_state == USB_ON)
            GPIO_ResetBits(USB31_PWRON_PIN_PORT, USB31_PWRON_PIN);
        else
            GPIO_SetBits(USB31_PWRON_PIN_PORT, USB31_PWRON_PIN);
    }
}

/*******************************************************************************
  * @function   power_control_disable_regulator
  * @brief      Shutdown DC/DC regulators.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_disable_regulator(void)
{
    GPIO_ResetBits(ENABLE_1V2_PIN_PORT, ENABLE_1V2_PIN);
    GPIO_ResetBits(ENABLE_1V35_PIN_PORT, ENABLE_1V35_PIN);
    GPIO_ResetBits(ENABLE_VTT_PIN_PORT, ENABLE_VTT_PIN);
    GPIO_ResetBits(ENABLE_1V5_PIN_PORT, ENABLE_1V5_PIN);
    GPIO_ResetBits(ENABLE_1V8_PIN_PORT, ENABLE_1V8_PIN);
    GPIO_SetBits(ENABLE_3V3_PIN_PORT, ENABLE_3V3_PIN);
#ifdef USE_4V5_POWER
    GPIO_ResetBits(ENABLE_4V5_PIN_PORT, ENABLE_4V5_PIN)
#endif
    GPIO_ResetBits(ENABLE_5V_PIN_PORT, ENABLE_5V_PIN);
}

/*******************************************************************************
  * @function   sysres_out_startup
  * @brief      Handle SYSRES_OUT and CFG_CTRL signals during startup.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void sysres_out_startup(void)
{
    GPIO_SetBits(SYSRES_OUT_PIN_PORT, SYSRES_OUT_PIN);
    GPIO_SetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);

    // wait for main board reset signal
    while (!GPIO_ReadInputDataBit(SYSRES_OUT_PIN_PORT, SYSRES_OUT_PIN))
        ;

    delay(5); // 5ms delay after releasing of reset signal

    GPIO_ResetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);
}
