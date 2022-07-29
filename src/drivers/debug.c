/**
 ******************************************************************************
 * @file    debug.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    25-August-2015
 * @brief   Debug prints on serial port.
 ******************************************************************************
 ******************************************************************************
 **/
#include "debug.h"
#include "usart.h"

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
