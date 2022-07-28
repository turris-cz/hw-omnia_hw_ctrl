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
void debug_init(void);

void debug(const char *buffer);
#else
static inline void debug_init(void)
{
}

static inline void debug(const char *buffer)
{
}
#endif

#endif // DEBUG_SERIAL_H

