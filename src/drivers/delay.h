/**
  ******************************************************************************
  * @file    delay.h
  * @author  CZ.NIC, z.s.p.o.
  * @date    22-July-2015
  * @brief   Header file delay file
  ******************************************************************************
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DELAY_H
#define __DELAY_H

typedef enum watchdog_state {
    STOP                 = 0,
    RUN                  = 1,
    INIT                 = 2
} watchdog_state_t;

enum watchdog_status {
    WDG_DISABLE          = 0,
    WDG_ENABLE           = 1
};

struct st_watchdog {
    watchdog_state_t watchdog_state;
    uint16_t watchdog_sts;
};

extern struct st_watchdog watchdog;

/*******************************************************************************
  * @function   delay_systimer_config
  * @brief      Setup SysTick Timer for 1 msec interrupts.
  * @param      None
  * @retval     None
  *****************************************************************************/
void delay_systimer_config(void);

/******************************************************************************
  * @function   delay
  * @brief      Inserts a delay time.
  * @param      nTime: specifies the delay time length, in miliseconds.
  * @retval     None
  *****************************************************************************/
void delay(volatile uint32_t nTime);

/******************************************************************************
  * @function   systick_irq_handler
  * @brief      Decrements the TimingDelay variable in System Timer and
  *             takes care of watchdog timeout.
  * @param      None
  * @retval     None
  *****************************************************************************/
void systick_irq_handler(void);

#endif /* __DELAY_H */
