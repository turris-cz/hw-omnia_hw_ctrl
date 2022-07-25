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

#define SERIAL_PORT      USART1

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
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
    USART_DeInit(SERIAL_PORT);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    gpio_init_alts(USART_PINS_ALT_FN, pin_pushpull, pin_spd_3, pin_pullup,
                   USART_PIN_RX, USART_PIN_TX);

    USART_InitStructure.USART_BaudRate = DBG_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(SERIAL_PORT, &USART_InitStructure);

    USART_Cmd(SERIAL_PORT, ENABLE);
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
    while(count--)
    {
        /* Loop until the end of transmission */
        while(USART_GetFlagStatus(SERIAL_PORT, USART_FLAG_TXE) == RESET)
            ;
        USART_SendData(SERIAL_PORT, *buffer++);
    }
    while(USART_GetFlagStatus(SERIAL_PORT, USART_FLAG_TC) == RESET)
        ;
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
