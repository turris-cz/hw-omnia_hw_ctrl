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
#include "debug_serial.h"

#if !defined(USER_REGULATOR_ENABLED)
#error build system did not define USER_REGULATOR_ENABLED macro
#endif

#if USER_REGULATOR_ENABLED
#error user regulator not supported on this branch, wait until branches are merged
#endif

/* Private define ------------------------------------------------------------*/

/* defines for timeout handling during regulator startup */
#define DELAY_AFTER_ENABLE      5
#define DELAY_BETWEEN_READINGS  20
#define TIMEOUT                 100 /* DELAY_BETWEEN_READINGS * 100 = 2 sec */

/* define for timeout handlling during reset */
#define RESET_STATE_READING     5 /* ms */
#define RESET_TIMEOFFSET        2

#define RGB_COLOUR_LEVELS       255

typedef enum reset_states {
    RST_INIT,
    RST_LED0,
    RST_LED1,
    RST_LED2,
    RST_LED3,
    RST_LED4,
    RST_LED5,
    RST_LED6,
    RST_LED7,
    RST_LED8,
    RST_LED9,
    RST_LED10,
    RST_LED11,
} reset_state_t;

/*******************************************************************************
  * @function   system_control_io_config
  * @brief      GPIO config for EN, PG, Reset and USB signals.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_io_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(ENABLE_5V_PIN_PERIPH_CLOCK |
       ENABLE_3V3_PIN_PERIPH_CLOCK | ENABLE_1V35_PIN_PERIPH_CLOCK |
       ENABLE_1V8_PIN_PERIPH_CLOCK |
       ENABLE_1V5_PIN_PERIPH_CLOCK | ENABLE_1V2_PIN_PERIPH_CLOCK |
       ENABLE_VTT_PIN_PERIPH_CLOCK | USB30_PWRON_PIN_PERIPH_CLOCK |
       USB31_PWRON_PIN_PERIPH_CLOCK | SYSRES_OUT_PIN_PERIPH_CLOCK |
       INT_MCU_PIN_PERIPH_CLOCK | MANRES_PIN_PERIPH_CLOCK, ENABLE);

    /* Output signals */
    GPIO_InitStructure.GPIO_Pin = INT_MCU_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(INT_MCU_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_5V_PIN;
    GPIO_Init(ENABLE_5V_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_3V3_PIN;
    GPIO_Init(ENABLE_3V3_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = ENABLE_1V35_PIN;
    GPIO_Init(ENABLE_1V35_PIN_PORT, &GPIO_InitStructure);

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

    GPIO_InitStructure.GPIO_Pin = CFG_CTRL_PIN;
    GPIO_Init(CFG_CTRL_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = SYSRES_OUT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(SYSRES_OUT_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = MANRES_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(MANRES_PIN_PORT, &GPIO_InitStructure);


    /* Input signals */

    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(PG_5V_PIN_PERIPH_CLOCK | PG_3V3_PIN_PERIPH_CLOCK |
                          PG_1V35_PIN_PERIPH_CLOCK |
                          PG_1V8_PIN_PERIPH_CLOCK | PG_1V5_PIN_PERIPH_CLOCK |
                          PG_1V2_PIN_PERIPH_CLOCK | PG_VTT_PIN_PERIPH_CLOCK |
                          USB30_OVC_PIN_PERIPH_CLOCK | USB31_OVC_PIN_PERIPH_CLOCK |
                          LED_BRT_PIN_PERIPH_CLOCK,
                          ENABLE);


    GPIO_InitStructure.GPIO_Pin = PG_5V_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(PG_5V_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_3V3_PIN;
    GPIO_Init(PG_3V3_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PG_1V35_PIN;
    GPIO_Init(PG_1V35_PIN_PORT, &GPIO_InitStructure);

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

    GPIO_InitStructure.GPIO_Pin = LED_BRT_PIN;
    GPIO_Init(LED_BRT_PIN_PORT, &GPIO_InitStructure);

    GPIO_SetBits(SYSRES_OUT_PIN_PORT, SYSRES_OUT_PIN); /* dont control this ! */
    GPIO_SetBits(INT_MCU_PIN_PORT, INT_MCU_PIN);
}

/*******************************************************************************
  * @function   power_control_set_startup_condition
  * @brief      Set signals to reset state before board startup.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_set_startup_condition(void)
{
    GPIO_SetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN); //disconnect switches
    GPIO_ResetBits(MANRES_PIN_PORT, MANRES_PIN); //board reset activated
    power_control_usb(USB3_PORT0, USB_ON);
    power_control_usb(USB3_PORT1, USB_ON);
}

/*******************************************************************************
  * @function   power_control_start_regulator
  * @brief      Start DC/DC regulator and handle timeout.
  * @param      regulator: regulator type.
  * @retval     error, if problem with PG signal occures.
  *****************************************************************************/
error_type_t power_control_start_regulator(reg_type_t regulator)
{
    error_type_t error = NO_ERROR;
    uint16_t counter = 0;

    switch(regulator)
    {
        case REG_5V:
        {
            GPIO_SetBits(ENABLE_5V_PIN_PORT, ENABLE_5V_PIN);
            delay(DELAY_AFTER_ENABLE);

            while(!(GPIO_ReadInputDataBit(PG_5V_PIN_PORT, PG_5V_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_5V_ERROR;
                    break;
                }
            }
        }break;

        case REG_3V3:
        {
            GPIO_SetBits(ENABLE_3V3_PIN_PORT, ENABLE_3V3_PIN);
            delay(DELAY_AFTER_ENABLE);

            while(!(GPIO_ReadInputDataBit(PG_3V3_PIN_PORT, PG_3V3_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_3V3_ERROR;
                    break;
                }
            }

        } break;

        case REG_1V35:
        {
            GPIO_SetBits(ENABLE_1V35_PIN_PORT, ENABLE_1V35_PIN);
            delay(DELAY_AFTER_ENABLE);
            while(!(GPIO_ReadInputDataBit(PG_1V35_PIN_PORT, PG_1V35_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_1V35_ERROR;
                    break;
                }
            }
        }break;

        case REG_1V8:
        {
            GPIO_SetBits(ENABLE_1V8_PIN_PORT, ENABLE_1V8_PIN);
            delay(DELAY_AFTER_ENABLE);
            while(!(GPIO_ReadInputDataBit(PG_1V8_PIN_PORT, PG_1V8_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_1V8_ERROR;
                    break;
                }
            }
        }break;

        case REG_1V5:
        {
            GPIO_SetBits(ENABLE_1V5_PIN_PORT, ENABLE_1V5_PIN);
            delay(DELAY_AFTER_ENABLE);
            while(!(GPIO_ReadInputDataBit(PG_1V5_PIN_PORT, PG_1V5_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_1V5_ERROR;
                    break;
                }
            }
        }break;

        case REG_1V2:
        {
            GPIO_SetBits(ENABLE_1V2_PIN_PORT, ENABLE_1V2_PIN);
            delay(DELAY_AFTER_ENABLE);
            while(!(GPIO_ReadInputDataBit(PG_1V2_PIN_PORT, PG_1V2_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_1V2_ERROR;
                    break;
                }
            }
        }break;

        case REG_VTT:
        {
            GPIO_SetBits(ENABLE_VTT_PIN_PORT, ENABLE_VTT_PIN);
            delay(DELAY_AFTER_ENABLE);
            while(!(GPIO_ReadInputDataBit(PG_VTT_PIN_PORT, PG_VTT_PIN)))
            {
                delay(DELAY_BETWEEN_READINGS);
                counter++;
                if (counter >= TIMEOUT)
                {
                    error = PG_VTT_ERROR;
                    break;
                }
            }
        }break;

        default:
            break;
    }

    return error;
}

/*******************************************************************************
  * @function   power_control_enable_regulators
  * @brief      Starts DC/DC regulators.
  * @param      None.
  * @retval     Error if timeout elapsed.
  *****************************************************************************/
error_type_t power_control_enable_regulators(void)
{
    error_type_t value = NO_ERROR;

    /*
     * power-up sequence:
     * 1) 5V regulator
     * 2) 4.5V regulator - user selectable - not populated on the board
     * 3) 3.3V regulator
     * 4) 1.8V regulator
     * 5) 1.5V regulator - not populated on the board
     * 6) 1.35V regulator
     *    VTT regulator
     * 7) 1.2V regulator
     */

    value = power_control_start_regulator(REG_5V);
    if (value != NO_ERROR)
        return value;

#if USER_REGULATOR_ENABLED
//    value = power_control_start_regulator(REG_4V5);
//    if (value != NO_ERROR)
//        return value;
#endif

    value = power_control_start_regulator(REG_3V3);
    if (value != NO_ERROR)
        return value;

    value = power_control_start_regulator(REG_1V8);
    if (value != NO_ERROR)
        return value;

//    value = power_control_start_regulator(REG_1V5);
//    if (value != NO_ERROR)
//        return value;

    value = power_control_start_regulator(REG_1V35);
    if (value != NO_ERROR)
        return value;

    value = power_control_start_regulator(REG_VTT);
    if (value != NO_ERROR)
        return value;

    value = power_control_start_regulator(REG_1V2);
    if (value != NO_ERROR)
        return value;

    return value;
}

/*******************************************************************************
  * @function   power_control_disable_regulators
  * @brief      Shutdown DC/DC regulators.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_disable_regulators(void)
{
    GPIO_ResetBits(ENABLE_1V2_PIN_PORT, ENABLE_1V2_PIN);
    GPIO_ResetBits(ENABLE_1V35_PIN_PORT, ENABLE_1V35_PIN);
    GPIO_ResetBits(ENABLE_VTT_PIN_PORT, ENABLE_VTT_PIN);
    GPIO_ResetBits(ENABLE_1V8_PIN_PORT, ENABLE_1V8_PIN);
    GPIO_ResetBits(ENABLE_3V3_PIN_PORT, ENABLE_3V3_PIN);
    GPIO_ResetBits(ENABLE_5V_PIN_PORT, ENABLE_5V_PIN);
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
  * @function   power_control_get_usb_overcurrent
  * @brief      Get USB overcurrent status.
  * @param      usb_port: USB3_PORT0 or USB3_PORT1.
  * @retval     1 - USB overcurrent ocurred; 0 - no USB overcurrent
  *****************************************************************************/
uint8_t power_control_get_usb_overcurrent(usb_ports_t usb_port)
{
    if (usb_port == USB3_PORT0)
        return (!(GPIO_ReadInputDataBit(USB30_OVC_PIN_PORT, USB30_OVC_PIN)));
    else //USB3_PORT1
        return (!(GPIO_ReadInputDataBit(USB31_OVC_PIN_PORT, USB31_OVC_PIN)));
}

/*******************************************************************************
  * @function   power_control_get_usb_poweron
  * @brief      Get USB poweron status.
  * @param      usb_port: USB3_PORT0 or USB3_PORT1.
  * @retval     1 - USB power ON; 0 - USB power OFF
  *****************************************************************************/
uint8_t power_control_get_usb_poweron(usb_ports_t usb_port)
{
    if (usb_port == USB3_PORT0)
        return (!(GPIO_ReadInputDataBit(USB30_PWRON_PIN_PORT, USB30_PWRON_PIN)));
    else //USB3_PORT1
        return (!(GPIO_ReadInputDataBit(USB31_PWRON_PIN_PORT, USB31_PWRON_PIN)));
}

/*******************************************************************************
  * @function   power_control_usb_timeout_config
  * @brief      Timer configuration for USB recovery timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_config(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM17, DISABLE);
    TIM_DeInit(USB_TIMEOUT_TIMER);

    /* Clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM17, ENABLE);

    /* Time base configuration - 1sec interrupt */
    TIM_TimeBaseStructure.TIM_Period = 8000 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 6000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(USB_TIMEOUT_TIMER, &TIM_TimeBaseStructure);

    TIM_ARRPreloadConfig(USB_TIMEOUT_TIMER, ENABLE);
    /* TIM Interrupts enable */
    TIM_ITConfig(USB_TIMEOUT_TIMER, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM17_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x05;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
  * @function   power_control_usb_timeout_enable
  * @brief      Enable USB recovery timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_enable(void)
{
    /* TIM enable counter */
    TIM_Cmd(USB_TIMEOUT_TIMER, ENABLE);
}

/*******************************************************************************
  * @function   power_control_usb_timeout_disable
  * @brief      Disable USB recovery timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_disable(void)
{
    /* disable timer and set initial condition */
    TIM_Cmd(USB_TIMEOUT_TIMER, DISABLE);
    USB_TIMEOUT_TIMER->CNT = 0;
}

/*******************************************************************************
  * @function   power_control_first_startup
  * @brief      Handle SYSRES_OUT, MAN_RES, CFG_CTRL signals and factory reset
  *             during startup.
  * @param      None.
  * @retval     Type of factory reset.
  *****************************************************************************/
reset_type_t power_control_first_startup(void)
{
    reset_type_t reset_type = NORMAL_RESET;
    reset_state_t reset_state = RST_INIT;
    uint16_t reset_cnt = 0;
    uint8_t red, green, idx;
    uint32_t colour = 0;
    uint16_t user_brightness;

    GPIO_SetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);
    delay(50);
    GPIO_SetBits(MANRES_PIN_PORT, MANRES_PIN);

    /* save brightness value to restore it */
    user_brightness = led_driver_pwm_get_brightness();

    /* wait for main board reset signal */
    while (!GPIO_ReadInputDataBit(SYSRES_OUT_PIN_PORT, SYSRES_OUT_PIN))
    {
        /* handle factory reset timeouts */
        delay(RESET_STATE_READING);
        reset_cnt++;

        if (reset_cnt >= RESET_TIMEOFFSET)
        {
            switch(reset_state)
            {
                case RST_INIT:
                {
                    led_driver_set_colour(LED_COUNT, GREEN_COLOUR);
                    led_driver_set_led_state(LED_COUNT, LED_OFF);
                    led_driver_set_led_state(LED11, LED_ON);
                    led_driver_pwm_set_brightness(100);
                    reset_state = RST_LED11;
                    idx = 0;
                    red = 0;
                    green = RGB_COLOUR_LEVELS;
                } break;

                case RST_LED11:
                {
                    reset_type = NORMAL_RESET;

                    led_driver_set_led_state(LED11, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);

                    led_driver_set_colour(LED11, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED10; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED11;
                    }
                } break;

                case RST_LED10:
                {
                    reset_type = PREVIOUS_SNAPSHOT;

                    led_driver_set_led_state(LED10, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED10, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED9; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED10;
                    }
                } break;

                case RST_LED9:
                {
                    reset_type = NORMAL_FACTORY_RESET;

                    led_driver_set_led_state(LED9, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED9, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED8; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED9;
                    }
                } break;

                case RST_LED8:
                {
                    reset_type = HARD_FACTORY_RESET;

                    led_driver_set_led_state(LED8, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED8, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED7; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED8;
                    }
                } break;

                case RST_LED7:
                {
                    reset_type = USER_RESET1;

                    led_driver_set_led_state(LED7, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED7, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED6; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED7;
                    }
                } break;

                case RST_LED6:
                {
                    reset_type = USER_RESET2;

                    led_driver_set_led_state(LED6, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED6, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED5; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED6;
                    }
                } break;

                case RST_LED5:
                {
                    reset_type = USER_RESET3;

                    led_driver_set_led_state(LED5, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED5, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED4; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED5;
                    }
                } break;

                case RST_LED4:
                {
                    reset_type = USER_RESET4;

                    led_driver_set_led_state(LED4, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED4, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED3; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED4;
                    }
                } break;

                case RST_LED3:
                {
                    reset_type = USER_RESET5;

                    led_driver_set_led_state(LED3, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED3, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED2; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED3;
                    }
                } break;

                case RST_LED2:
                {
                    reset_type = USER_RESET6;

                    led_driver_set_led_state(LED2, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED2, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED1; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED2;
                    }
                } break;

                case RST_LED1:
                {
                    reset_type = USER_RESET7;

                    led_driver_set_led_state(LED1, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED1, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED0; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                    }
                    else
                    {
                        reset_state = RST_LED1;
                    }
                } break;

                case RST_LED0:
                {
                    reset_type = USER_RESET8;

                    led_driver_set_led_state(LED0, LED_ON);

                    idx++; /* increase colour level */
                    red++;
                    green--;
                    colour = (red << 16) | (green << 8);;

                    led_driver_set_colour(LED0, colour);

                    if (idx >= RGB_COLOUR_LEVELS)
                    {
                        reset_state = RST_LED11; /* next state */
                        idx = 0;
                        red = 0;
                        green = RGB_COLOUR_LEVELS;
                        /* final state - go back to start */
                        led_driver_set_colour(LED_COUNT, GREEN_COLOUR);
                        led_driver_set_led_state(LED_COUNT, LED_OFF);
                    }
                    else
                    {
                        reset_state = RST_LED0;
                    }
                } break;
            }

            reset_cnt = 0;
        }
    }

    delay(10); /* 10 + 5ms (in while loop) delay after releasing of reset signal */
    GPIO_ResetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);

    if (reset_type != NORMAL_RESET)
    {
        led_driver_pwm_set_brightness(0);
        delay(300);
        led_driver_pwm_set_brightness(100);
        delay(300);
        led_driver_pwm_set_brightness(0);
        delay(300);
        led_driver_pwm_set_brightness(100);
        delay(600);
    }

    /* restore brightness and colour */
    led_driver_pwm_set_brightness(user_brightness);
    led_driver_set_led_state(LED_COUNT, LED_OFF);
    led_driver_set_colour(LED_COUNT, WHITE_COLOUR);

    return reset_type;
}

/*******************************************************************************
  * @function   power_control_set_power_led
  * @brief      Set on power LED.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_set_power_led(void)
{
    struct led_rgb *rgb_leds = leds;

    led_driver_set_colour(POWER_LED, WHITE_COLOUR);
    rgb_leds[POWER_LED].led_state_default = LED_ON;
}

/*******************************************************************************
  * @function   power_led_activity
  * @brief      Set on power LED.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_led_activity(void)
{
    struct led_rgb *rgb_leds = leds;

    if (rgb_leds[POWER_LED].led_mode == LED_DEFAULT_MODE)
    {
        rgb_leds[POWER_LED].led_state_default = LED_ON;
    }
}

/*******************************************************************************
  * @function   power_io_new_config
  * @brief      Configuration of new IO pins for Omnia32
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_new_io_config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHBPeriphClockCmd(PERST0_PIN_PERIPH_CLOCK | PERST1_PIN_PERIPH_CLOCK
                          | PERST2_PIN_PERIPH_CLOCK | SFP_NDET_PIN_PERIPH_CLOCK
                          | PHY_SFP_PIN_PERIPH_CLOCK | VHV_CTRL_PIN_PERIPH_CLOCK
                          | RES_PHY_PIN_PERIPH_CLOCK | RES_LAN_PERIPH_CLOCK
                          | RES_MMC_PIN_PERIPH_CLOCK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = SFP_NDET_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(SFP_NDET_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = RES_MMC_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(RES_MMC_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PERST0_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_2;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(PERST0_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PERST1_PIN;
    /*
     * PERST1 pin is also used as MCU's UART RX pin, so only configure it as
     * GPIO if debugging is disabled
     */
    if (!DBG_ENABLE)
        GPIO_Init(PERST1_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PERST2_PIN;
    GPIO_Init(PERST2_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = RES_LAN_PIN;
    GPIO_Init(RES_LAN_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = RES_PHY_PIN;
    GPIO_Init(RES_PHY_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = VHV_CTRL_PIN;
    GPIO_Init(VHV_CTRL_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PHY_SFP_PIN;
    GPIO_Init(PHY_SFP_PIN_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(RES_MMC_PIN_PORT, RES_MMC_PIN);
    GPIO_ResetBits(RES_LAN_PIN_PORT, RES_LAN_PIN);
    GPIO_ResetBits(RES_PHY_PIN_PORT, RES_PHY_PIN);

    GPIO_ResetBits(PERST0_PIN_PORT, PERST0_PIN);
    /*
     * PERST1 pin is also used as MCU's UART RX pin, so only configure it as
     * GPIO if debugging is disabled
     */
    if (!DBG_ENABLE)
        GPIO_ResetBits(PERST1_PIN_PORT, PERST1_PIN);

    GPIO_ResetBits(PERST2_PIN_PORT, PERST2_PIN);

    GPIO_SetBits(VHV_CTRL_PIN_PORT, VHV_CTRL_PIN);
    GPIO_SetBits(PHY_SFP_PIN_PORT, PHY_SFP_PIN);
}

/*******************************************************************************
  * @function   power_control_periph_rst_init
  * @brief      Set reset init states for peripherals for Omnia32
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_periph_rst_init(void)
{
    GPIO_ResetBits(RES_MMC_PIN_PORT, RES_MMC_PIN);
    GPIO_ResetBits(RES_LAN_PIN_PORT, RES_LAN_PIN);
    GPIO_ResetBits(RES_PHY_PIN_PORT, RES_PHY_PIN);

    GPIO_ResetBits(PERST0_PIN_PORT, PERST0_PIN);
    /*
     * PERST1 pin is also used as MCU's UART RX pin, so only configure it as
     * GPIO if debugging is disabled
     */
    if (!DBG_ENABLE)
        GPIO_ResetBits(PERST1_PIN_PORT, PERST1_PIN);

    GPIO_ResetBits(PERST2_PIN_PORT, PERST2_PIN);

    GPIO_SetBits(VHV_CTRL_PIN_PORT, VHV_CTRL_PIN);
    GPIO_SetBits(PHY_SFP_PIN_PORT, PHY_SFP_PIN);
}
