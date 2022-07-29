/**
 ******************************************************************************
 * @file    bootloader.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    17-April-2016
 * @brief   Bootloader state machine.
 ******************************************************************************
 ******************************************************************************
 **/
#include "boot_i2c.h"
#include "power_control.h"
#include "delay.h"
#include "eeprom.h"
#include "debug.h"
#include "bootloader.h"
#include "led_driver.h"
#include "flash.h"
#include "flash_defs.h"
#include "debounce.h"
#include "gpio.h"
#include "timer.h"
#include "cpu.h"

typedef enum bootloader_states {
    POWER_ON,
    STARTUP_MANAGER,
    RESET_MANAGER,
    FLASH_MANAGER,
    START_APPLICATION,
    RESET_TO_APPLICATION
} boot_state_t;

typedef enum bootloader_return_val {
    GO_TO_POWER_ON,
    GO_TO_STARTUP_MANAGER,
    GO_TO_RESET_MANAGER,
    GO_TO_FLASH,
    GO_TO_APPLICATION
} boot_value_t;

typedef void (*pFunction)(void);

/*******************************************************************************
  * @function   bootloader_init
  * @brief      Init of bootloader
  * @param      None
  * @retval     None
  *****************************************************************************/
void bootloader_init(void)
{
    /* peripheral initialization*/
    delay_systimer_config();
    led_driver_config();
    boot_i2c_config();

    FLASH_Unlock(); /* Unlock the Flash Program Erase controller */
    EE_Init(); /* EEPROM Init */
    flash_init();
    timer_deinit(DEBOUNCE_TIMER);
    timer_deinit(USB_TIMEOUT_TIMER);
    enable_irq();

    led_driver_set_colour(LED_COUNT, GREEN_COLOUR);
    led_driver_reset_effect(ENABLE);
    debug_init();

    gpio_init_outputs(pin_opendrain, pin_spd_2, 1, SYSRES_OUT_PIN); /* dont control this ! */

    debug("Init\n");
}

/*******************************************************************************
  * @function   bootloader_init
  * @brief      Init of bootloader.
  * @param      None
  * @retval     None
  *****************************************************************************/
static void start_application(void)
{
    pFunction app_entry;
    uint32_t app_stack;

    disable_irq();

    /* Get the application stack pointer (First entry in the application vector table) */
    app_stack = (uint32_t) *((volatile uint32_t*)APPLICATION_BEGIN);

    /* Get the application entry point (Second entry in the application vector table) */
    app_entry = (pFunction) *(volatile uint32_t*) (APPLICATION_BEGIN + 4);

    /* Set the application stack pointer */
    set_msp(app_stack);

    /* ISB = instruction synchronization barrier. It flushes the pipeline of
     * the processor, so that all instructions following the ISB are fetched
     * from cache or memory again, after the ISB instruction has been completed.
     * Must be called after changing stack pointer according to the documentation.
    */
    isb();

    /* Start the application */
    app_entry();
}

/*******************************************************************************
  * @function   startup_manager
  * @brief      Determine a reset reason and following reaction.
  * @param      None
  * @retval     None
  *****************************************************************************/
static boot_value_t startup_manager(void)
{
    eeprom_var_t ee_var;
    uint16_t ee_data;
    boot_value_t retval = GO_TO_RESET_MANAGER;

    ee_var = EE_ReadVariable(RESET_VIRT_ADDR, &ee_data);

    switch(ee_var)
    {
        /* power on reset - first boot - everything is flashed;
           request for reflashing has never ocurred */
        case VAR_NOT_FOUND:
        {
            retval = GO_TO_APPLICATION;

            debug("R1\n");
        } break;

        case VAR_FOUND:
        {
            switch (ee_data)
            {
                case BOOTLOADER_REQ:
                {
                    retval = GO_TO_FLASH;
                    debug("req\n");
                } break;

                case FLASH_NOT_CONFIRMED: /* error */
                {
                    retval = GO_TO_POWER_ON;
                    debug("ERR\n");
                } break;

                case FLASH_CONFIRMED: /* application was flashed correctly */
                {
                    retval = GO_TO_APPLICATION;
                    debug("R2\n");
                } break;

                /* flag has not been saved correctly */
                default: retval = GO_TO_POWER_ON; break;
            }
        } break;

        case VAR_NO_VALID_PAGE :
        {
            retval = GO_TO_POWER_ON;
            debug("Boot-No valid page\n");
        }
            break;

        default:
            break;
    }

    return retval;
}

/*******************************************************************************
  * @function   bootloader
  * @brief      Main bootloader state machine.
  * @param      None
  * @retval     None
  *****************************************************************************/
void bootloader(void)
{
    static boot_state_t next_state = STARTUP_MANAGER;
    static boot_value_t val = GO_TO_RESET_MANAGER;
    static flash_i2c_state_t flash_sts = FLASH_CMD_NOT_RECEIVED;
    static uint8_t flash_confirmed;
    static uint8_t power_supply_failure; /* if power supply disconnection occurred */
    uint8_t system_reset;

    switch(next_state)
    {
        case STARTUP_MANAGER:
        {
            val = startup_manager();

            switch (val)
            {
                case GO_TO_POWER_ON:
                {
                    EE_WriteVariable(RESET_VIRT_ADDR, FLASH_NOT_CONFIRMED);
                    next_state = POWER_ON;
                } break;
                case GO_TO_APPLICATION:
                {
                    next_state = START_APPLICATION;
                } break;
                case GO_TO_FLASH:
                {
                    EE_WriteVariable(RESET_VIRT_ADDR, FLASH_NOT_CONFIRMED);
                    next_state = FLASH_MANAGER;
                } break;

                default: next_state = POWER_ON;    break;
            }
        } break;

        case POWER_ON:
        {
            power_control_io_config();
            power_control_set_startup_condition();
            power_control_disable_regulators();
            delay(100);

            power_control_enable_regulators();
            power_control_first_startup();
            power_supply_failure = 1;
            next_state = FLASH_MANAGER;
        } break;

        case FLASH_MANAGER:
        {
            flash_sts = boot_i2c_flash_data();

            switch(flash_sts)
            {
                case FLASH_CMD_RECEIVED: /* flashing has just started */
                {
                    next_state = RESET_MANAGER;
                } break;

                case FLASH_CMD_NOT_RECEIVED: /* nothing has received */
                {
                    next_state = RESET_MANAGER;
                } break;

                case FLASH_WRITE_OK: /* flashing was successfull */
                {
                    if (!flash_confirmed)
                    {
                        EE_WriteVariable(RESET_VIRT_ADDR, FLASH_CONFIRMED);
                        flash_confirmed = 1;
                    }

                    next_state = RESET_MANAGER;
                    debug("F_CONF\n");
                } break;

                case FLASH_WRITE_ERROR: /* flashing was corrupted */
                {
                    /* flag FLASH_NOT_CONFIRMED is already set */
                    next_state = RESET_MANAGER;
                } break;
            }
        } break;

        case RESET_MANAGER:
        {
            system_reset = gpio_read(SYSRES_OUT_PIN);

            if(system_reset == 0) /* reset is active in low level */
            {
                next_state = RESET_TO_APPLICATION;
            }
            else
            {
                next_state = FLASH_MANAGER;
            }
        } break;

        case START_APPLICATION:
        {
            start_application();
        } break;

        case RESET_TO_APPLICATION:
        {
            /* power supply wasnt disconnected and no command for flashing was received */
            if ((power_supply_failure == 0) && (flash_sts == FLASH_CMD_NOT_RECEIVED))
            {
                /* we have old, but valid FW */
                EE_WriteVariable(RESET_VIRT_ADDR, FLASH_CONFIRMED);
            }

            /* shutdown regulators before reset, otherwise power supply can
            * stay there and causes wrong detection of mmc during boot */
            power_control_set_startup_condition();
            power_control_disable_regulators();
            delay(100);
            NVIC_SystemReset();
        } break;
    }
}
