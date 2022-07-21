/**
 ******************************************************************************
 * @file    app.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    25-September-2015
 * @brief   Header file for app.c
 ******************************************************************************
 ******************************************************************************
 **/
#ifndef __APP_H
#define __APP_H

#include "delay.h"

typedef enum {
    GO_TO_VTT_ERROR      = -8,
    GO_TO_1V2_ERROR      = -7,
    GO_TO_1V5_ERROR      = -6,
    GO_TO_1V8_ERROR      = -5,
    GO_TO_4V5_ERROR      = -4,
    GO_TO_1V35_ERROR     = -3,
    GO_TO_3V3_ERROR      = -2,
    GO_TO_5V_ERROR       = -1,
    OK                   = 0,
    GO_TO_LIGHT_RESET    = 1,
    GO_TO_HARD_RESET     = 2,
    GO_TO_BOOTLOADER     = 3,
}ret_value_t;

typedef enum {
    POWER_ON,
    LIGHT_RESET,
    HARD_RESET,
    ERROR_STATE,
    INPUT_MANAGER,
    I2C_MANAGER,
    LED_MANAGER,
    BOOTLOADER
} states_t;

/*******************************************************************************
  * @function   app_mcu_init
  * @brief      Initialization of MCU and its ports and peripherals.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void app_mcu_init(void);

/*******************************************************************************
  * @function   app_mcu_cyclic
  * @brief      Main cyclic function.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void app_mcu_cyclic(void);

#endif // __APP_H
