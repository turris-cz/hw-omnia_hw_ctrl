/**
 ******************************************************************************
 * @file    debug_serial.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    25-August-2015
 * @brief   Debug prints on serial port.
 ******************************************************************************
 ******************************************************************************
 **/
#ifndef DEBUG_SERIAL_H
#define DEBUG_SERIAL_H

/* ! CAUTION ! UART pins are used for CARD detection and PCI1 PLED pin ! */
#ifndef DBG_ENABLE
#error build system did not define DBG_ENABLE macro
#endif

#if DBG_ENABLE
#define DBG(buf)        debug_print(buf);
#else
#define DBG(...)
#endif

/*******************************************************************************
  * @function   debug_serial_config
  * @brief      Configuration of UART peripheral.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void debug_serial_config(void);

/*******************************************************************************
  * @function   debug_print
  * @brief      Send buffer over the serial port.
  * @param      buffer: pointer to data buffer.
  * @retval     None.
  *****************************************************************************/
void debug_print(const char *buffer);

#endif // DEBUG_SERIAL_H

