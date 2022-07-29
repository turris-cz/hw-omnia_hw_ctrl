/**
 ******************************************************************************
 * @file    main.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    21-July-2015
 * @brief   Main program body
 ******************************************************************************
 ******************************************************************************
 **/

/* Includes ------------------------------------------------------------------*/
#include "app.h"
#include "cpu.h"

#define APPLICATION_ADDRESS         0x08005000
#define RAM_ADDRESS                 0x20000000
#define BOOTLOADER_ADDRESS          0x08000000

typedef void (*pFunction)(void);

void start_bootloader(void);

/*******************************************************************************
 * @brief  Main program.
 * @param  None
 * @retval None
 ******************************************************************************/
int main(void)
{
    enable_irq();

    app_mcu_init();

    while(1)
    {
        app_mcu_cyclic();
    }
}

void start_bootloader(void)
{
    pFunction boot_entry;
    uint32_t boot_stack;

    disable_irq();

    /* Get the Bootloader stack pointer (First entry in the Bootloader vector table) */
    boot_stack = (uint32_t) *((volatile uint32_t*)BOOTLOADER_ADDRESS);

    /* Get the Bootloader entry point (Second entry in the Bootloader vector table) */
    boot_entry = (pFunction) *(volatile uint32_t*) (BOOTLOADER_ADDRESS + 4);

    /* Set the Bootloader stack pointer */
    set_msp(boot_stack);

        /* ISB = instruction synchronization barrier. It flushes the pipeline of
     * the processor, so that all instructions following the ISB are fetched
     * from cache or memory again, after the ISB instruction has been completed.
     * Must be called after changing stack pointer according to the documentation.
    */
    isb();

    /* Start the Bootloader */
    boot_entry();

    while(1);
}
