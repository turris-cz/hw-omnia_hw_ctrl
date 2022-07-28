/**
 ******************************************************************************
 * @file    delay.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    22-July-2015
 * @brief   This file ensures delay driver using of System Timer
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "delay.h"
#include "power_control.h"

#define WATCHDOG_ENABLE     1
#define WATCHDOG_TIMEOUT    120000 /* ms */

static volatile uint32_t timingdelay;

struct st_watchdog watchdog;

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
  * @function   delay_systimer_config
  * @brief      Setup SysTick Timer for 1 msec interrupts.
  * @param      None
  * @retval     None
  *****************************************************************************/
void delay_systimer_config(void)
{
    if (SysTick_Config(SystemCoreClock / 1000u))
    {
        /* Capture error */
        while (1);
    }
    NVIC_SetPriority(SysTick_IRQn, 2);
}

/******************************************************************************
  * @function   delay
  * @brief      Inserts a delay time.
  * @param      nTime: specifies the delay time length, in miliseconds.
  * @retval     None
  *****************************************************************************/
void delay(volatile uint32_t nTime)
{
    timingdelay = nTime;

    while(timingdelay != 0u);
}

/******************************************************************************
  * @function   delay_timing_decrement
  * @brief      Decrements the TimingDelay variable in System Timer and
  *             takes care of watchdog timeout.
  * @param      None
  * @retval     None
  *****************************************************************************/
void delay_timing_decrement(void)
{
    static uint32_t wdg_cnt;

    if (timingdelay != 0x00)
    {
        timingdelay--;
    }

#if WATCHDOG_ENABLE
    if (watchdog.watchdog_state == RUN)
    {
        wdg_cnt++;

        if (wdg_cnt >= WATCHDOG_TIMEOUT)
        {
            power_control_set_startup_condition();
            power_control_disable_regulators();
            NVIC_SystemReset(); /* SW reset */
        }
    }
    else
    {
        wdg_cnt = 0;
    }
#endif
}
