/**
 ******************************************************************************
 * @file    wan_lan_pci_status.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    10-August-2015
 * @brief   Header file for driver of WAN, LAN and PCIe status indication.
 ******************************************************************************
 ******************************************************************************
 **/
#ifndef WAN_LAN_PCI_STATUS_H
#define WAN_LAN_PCI_STATUS_H

#include "gpio.h"

/* PCIe status LED */
#define PCI_PLED0_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(F, 5) : PIN_INVALID)
#define PCI_LLED1_PIN		PIN(C, 2)
#define PCI_PLED1_PIN		(OMNIA_BOARD_REVISION < 32 && !DBG_ENABLE ? PIN(A, 10) : PIN_INVALID)
#define PCI_LLED2_PIN		PIN(C, 1)
#define PCI_PLED2_PIN		(OMNIA_BOARD_REVISION < 32 ? PIN(F, 4) : PIN_INVALID)

/* WAN LED */
#define WAN_LED0_PIN		PIN(F, 0)
#define WAN_LED1_PIN		(OMNIA_BOARD_REVISION >= 32 ? PIN(F, 1) : PIN_INVALID)

/* LAN LED */
#define LAN_LED_PORT		PORT_A
#define R0_P0_LED_PIN		PIN(A, 0)
#define R1_P1_LED_PIN		PIN(A, 1)
#define R2_P2_LED_PIN		PIN(A, 2)
#define C0_P3_LED_PIN		PIN(A, 6)
#define C1_LED_PIN		PIN(A, 8)
#define C2_P4_LED_PIN		PIN(A, 11)
#define C3_P5_LED_PIN		PIN(A, 12)


/*******************************************************************************
  * @function   wan_lan_pci_config
  * @brief      Main configuration function for WAN, LAN and PCIe status indication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void wan_lan_pci_config(void);

/*******************************************************************************
  * @function   wan_led_activity
  * @brief      Toggle WAN LED according to the WAN activity.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void wan_led_activity(void);

/*******************************************************************************
  * @function   pci_led_activity
  * @brief      Toggle PCIe LED according to the PCIe activity.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void pci_led_activity(void);

/*******************************************************************************
  * @function   lan_led_activity
  * @brief      Toggle LAN LEDs according to the LAN status.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void lan_led_activity(void);

#endif // WAN_LAN_PCI_STATUS_H
