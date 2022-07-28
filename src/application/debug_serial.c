/**
 ******************************************************************************
 * @file    debug_serial.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    25-August-2015
 * @brief   Debug prints on serial port.
 ******************************************************************************
 ******************************************************************************
 **/
#include "string.h"
#include "stm32f0xx_conf.h"
#include "debug_serial.h"
#include "gpio.h"
#include "usart.h"

#define USART_PINS_ALT_FN	1
#define USART_PIN_TX		PIN(A, 9)
#define USART_PIN_RX		PIN(A, 10)

#if DBG_ENABLE && !defined(DBG_BAUDRATE)
#error build system did not define DBG_BAUDRATE macro
#endif

/*******************************************************************************
  * @function   debug_serial_config
  * @brief      Configuration of UART peripheral.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void debug_serial_config(void)
{
#if DBG_ENABLE
    gpio_init_alts(USART_PINS_ALT_FN, pin_pushpull, pin_spd_3, pin_pullup,
                   USART_PIN_RX, USART_PIN_TX);

    usart_init(DEBUG_USART, DBG_BAUDRATE);
#endif
}

/*******************************************************************************
  * @function   debug_send_data
  * @brief      Send data over the serial port.
  * @param      buffer: pointer to data buffer.
  * @param      count: number of bytes
  * @retval     None.
  *****************************************************************************/
static void debug_send_data(const char *buffer, uint16_t count)
{
    while (count--) {
        if (*buffer == '\n')
            usart_tx(DEBUG_USART, '\r');
        usart_tx(DEBUG_USART, *buffer++);
    }

    while (!usart_is_tx_complete(DEBUG_USART));
}

/*******************************************************************************
  * @function   debug_print
  * @brief      Send buffer over the serial port.
  * @param      buffer: pointer to data buffer.
  * @retval     None.
  *****************************************************************************/
void debug_print(const char *buffer)
{
    uint16_t count = strlen(buffer);
    debug_send_data(buffer, count);
}
