/**
 ******************************************************************************
 * @file    power_control.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    22-July-2015
 * @brief   Header file for control of DC/DC converters.
 ******************************************************************************
 ******************************************************************************
 **/
#ifndef POWER_CONTROL_H
#define POWER_CONTROL_H

#include "stm32f0xx_tim.h"
#include "gpio.h"

/* Outputs */
#define INT_MCU_PIN		PIN(C, 0)
#define RES_RAM_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(C, 3) : PIN_INVALID)
#define ENABLE_5V_PIN		PIN(C, 4)
#define ENABLE_3V3_PIN		PIN(C, 5)
#define ENABLE_1V35_PIN		PIN(C, 6)
#define ENABLE_4V5_PIN		(USER_REGULATOR_ENABLED ? PIN(C, 7) : PIN_INVALID)
#define ENABLE_1V8_PIN		PIN(C, 8)
#define ENABLE_1V5_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(C, 9) : PIN_INVALID)
#define ENABLE_1V2_PIN		PIN(C, 10)
#define ENABLE_VTT_PIN		PIN(C, 11)
#define USB30_PWRON_PIN		PIN(C, 12)
#define USB31_PWRON_PIN		PIN(C, 13)
#define CFG_CTRL_PIN		PIN(C, 15)
#define PRG_4V5_PIN		(USER_REGULATOR_ENABLED ? PIN(F, 1) : PIN_INVALID)
/* v32 specific outputs */
#define PERST1_PIN		(DBG_ENABLE ? PIN_INVALID : PIN(A, 10))
#define RES_MMC_PIN		PIN(B, 2)
#define RES_LAN_PIN		PIN(B, 3)
#define RES_PHY_PIN		PIN(B, 7)
#define VHV_CTRL_PIN		PIN(B, 14)
#define PHY_SFP_PIN		PIN(C, 3)
#define PERST2_PIN		PIN(F, 4)
#define PERST0_PIN		PIN(F, 5)

/* Inputs */
#define MANRES_PIN		PIN(B, 0)
#define SYSRES_OUT_PIN		PIN(B, 1)
#define DBGRES_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(B, 2) : PIN_INVALID)
#define MRES_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(B, 3) : PIN_INVALID)
#define PG_5V_PIN		PIN(B, 4)
#define PG_3V3_PIN		PIN(B, 5)
#define PG_1V35_PIN		PIN(B, 6)
#define PG_4V5_PIN		PIN(B, 7)
#define PG_1V8_PIN		PIN(B, 8)
#define PG_1V5_PIN		PIN(B, 9)
#define PG_1V2_PIN		PIN(B, 10)
#define PG_VTT_PIN		PIN(B, 11)
#define USB30_OVC_PIN		PIN(B, 12)
#define USB31_OVC_PIN		PIN(B, 13)
#define RTC_ALARM_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(B, 14) : PIN_INVALID)
#define LED_BRT_PIN		PIN(B, 15)
/* v32 specific inputs */
#define SFP_NDET_PIN		PIN(D, 2)

typedef enum voltages {
    VOLTAGE_33 = 1,
    VOLTAGE_36 = 2,
    VOLTAGE_45 = 3,
    VOLTAGE_51 = 4,
}voltage_value_t;

typedef enum usb_states {
    USB_OFF = 0,
    USB_ON  = 1
}usb_state_t;

typedef enum usb_ports {
    USB3_PORT0 = 0,
    USB3_PORT1 = 1
}usb_ports_t;

typedef enum reg_types {
    REG_5V,
    REG_3V3,
    REG_1V35,
    REG_4V5,
    REG_1V8,
    REG_1V5,
    REG_1V2,
    REG_VTT,
}reg_type_t;

typedef enum error_types {
    NO_ERROR,
    PG_5V_ERROR,
    PG_3V3_ERROR,
    PG_1V35_ERROR,
    PG_4V5_ERROR,
    PG_1V8_ERROR,
    PG_1V5_ERROR,
    PG_1V2_ERROR,
    PG_VTT_ERROR,
}error_type_t;

typedef enum reset_types {
    NORMAL_RESET            = 0,
    PREVIOUS_SNAPSHOT       = 1,
    NORMAL_FACTORY_RESET    = 2,
    HARD_FACTORY_RESET      = 3,
    USER_RESET1             = 4,
    USER_RESET2             = 5,
    USER_RESET3             = 6,
    USER_RESET4             = 7,
    USER_RESET5             = 8,
    USER_RESET6             = 9,
    USER_RESET7             = 10,
    USER_RESET8             = 11,
}reset_type_t;

/*******************************************************************************
  * @function   system_control_io_config
  * @brief      GPIO config for EN, PG, Reset and USB control signals.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_io_config(void);

/*******************************************************************************
  * @function   power_control_start_regulators
  * @brief      Starts DC/DC regulators.
  * @param      None.
  * @retval     Error if timeout elapsed.
  *****************************************************************************/
error_type_t power_control_enable_regulators(void);

/*******************************************************************************
  * @function   power_control_disable_regulators
  * @brief      Shutdown DC/DC regulators.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_disable_regulators(void);

/*******************************************************************************
  * @function   power_control_usb
  * @brief      Enable / disable power supply for USB.
  * @param      usb_port: USB3_PORT0 or USB3_PORT1.
  * @param      usb_state: USB_ON or USB_OFF.
  * @retval     None.
  *****************************************************************************/
void power_control_usb(usb_ports_t usb_port, usb_state_t usb_state);

/*******************************************************************************
  * @function   power_control_first_startup
  * @brief      Handle SYSRES_OUT, MAN_RES and CFG_CTRL signals during startup.
  * @param      None.
  * @retval     Type of factory reset.
  *****************************************************************************/
reset_type_t power_control_first_startup(void);

/*******************************************************************************
  * @function   power_control_usb_timeout_config
  * @brief      Timer configuration for USB recovery timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_config(void);

/*******************************************************************************
  * @function   power_control_get_usb_overcurrent
  * @brief      Get USB overcurrent status.
  * @param      usb_port: USB3_PORT0 or USB3_PORT1.
  * @retval     1 - USB overcurrent ocurred; 0 - no USB overcurrent
  *****************************************************************************/
bool power_control_get_usb_overcurrent(usb_ports_t usb_port);

/*******************************************************************************
  * @function   power_control_get_usb_poweron
  * @brief      Get USB poweron status.
  * @param      usb_port: USB3_PORT0 or USB3_PORT1.
  * @retval     1 - USB power ON; 0 - USB power OFF
  *****************************************************************************/
bool power_control_get_usb_poweron(usb_ports_t usb_port);

/*******************************************************************************
  * @function   power_control_set_startup_condition
  * @brief      Set signals to reset state before board startup.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_set_startup_condition(void);

/*******************************************************************************
  * @function   power_control_start_regulator
  * @brief      Start DC/DC regulator and handle timeout.
  * @param      regulator: regulator type.
  * @retval     error, if problem with PG signal occures.
  *****************************************************************************/
error_type_t power_control_start_regulator(reg_type_t regulator);

/*******************************************************************************
  * @function   power_control_set_power_led
  * @brief      Set on power LED.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_set_power_led(void);

/*******************************************************************************
  * @function   power_led_activity
  * @brief      Set on power LED.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_led_activity(void);

/*******************************************************************************
  * @function   power_control_usb_timeout_enable
  * @brief      Enable USB recovery timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_enable(void);

/*******************************************************************************
  * @function   power_control_usb_timeout_disable
  * @brief      Disable USB recovery timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_disable(void);

/*******************************************************************************
  * @function   power_control_usb_timeout_handler
  * @brief      Handle USB timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void power_control_usb_timeout_handler(void);

/*******************************************************************************
  * @function   power_control_set_voltage
  * @brief      Set required voltage to the user regulator.
  * @param      voltage: enum value for desired voltage.
  * @retval     None.
  *****************************************************************************/
void power_control_set_voltage(voltage_value_t voltage);

/*******************************************************************************
  * @function   periph_control_io_config
  * @brief      Configuration of new IO pins for Omnia32
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void periph_control_io_config(void);

/*******************************************************************************
  * @function   periph_control_rst_init
  * @brief      Set reset init states for peripherals for Omnia32
  * @param      None.
  * @retval     The corresponding initial settings for extended control word.
  *****************************************************************************/
uint16_t periph_control_rst_init(void);

#endif // POWER_CONTROL_H

