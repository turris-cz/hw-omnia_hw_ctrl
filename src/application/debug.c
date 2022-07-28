/**
 ******************************************************************************
 * @file    debug.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    25-August-2015
 * @brief   Debug prints on serial port.
 ******************************************************************************
 ******************************************************************************
 **/
#include "stm32f0xx_conf.h"
#include "debug.h"
#include "gpio.h"
#include "usart.h"

#define USART_PINS_ALT_FN	1
#define USART_PIN_TX		PIN(A, 9)
#define USART_PIN_RX		PIN(A, 10)

#if !defined(DBG_BAUDRATE)
#error build system did not define DBG_BAUDRATE macro
#endif

/*******************************************************************************
  * @function   debug_init
  * @brief      Configuration of UART peripheral.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void debug_init(void)
{
	gpio_init_alts(USART_PINS_ALT_FN, pin_pushpull, pin_spd_3, pin_pullup,
		       USART_PIN_RX, USART_PIN_TX);

	usart_init(DEBUG_USART, DBG_BAUDRATE);
}

void debug(const char *buffer)
{
	while (*buffer) {
		if (*buffer == '\n')
			usart_tx(DEBUG_USART, '\r');
		usart_tx(DEBUG_USART, *buffer++);
	}

	while (!usart_is_tx_complete(DEBUG_USART));
}
