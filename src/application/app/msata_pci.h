/**
 ******************************************************************************
 * @file    msata_pci.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    06-August-2015
 * @brief   Header file for PCIe and mSATA driver.
 ******************************************************************************
 ******************************************************************************
 **/
#ifndef MSATA_PCI_H
#define MSATA_PCI_H

#include "gpio.h"

#define CARD_DET_PIN		(DBG_ENABLE ? PIN_INVALID : PIN(A, 9))
#define MSATALED_PIN		PIN(A, 15)
#define MSATAIND_PIN		PIN(C, 14)

/*******************************************************************************
  * @function   msata_pci_indication_config
  * @brief      Main configuration function for mSATA and PCIe indication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void msata_pci_indication_config(void);

/*******************************************************************************
  * @function   msata_pci_activity
  * @brief      Toggle LED according to the activity of the connected card.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void msata_pci_activity(void);

/*******************************************************************************
  * @function   msata_pci_card_detection
  * @brief      Detect inserted card (whether is a card inserted or not)
  * @param      None.
  * @retval     1 - a card inserted, 0 - no card inserted.
  *****************************************************************************/
bool msata_pci_card_detection(void);

/*******************************************************************************
  * @function   msata_pci_type_card_detection
  * @brief      Detect a type of inserted card - mSATA or miniPCIe
  * @param      None.
  * @retval     1 - mSATA card inserted, 0 - miniPCIe card inserted.
  *****************************************************************************/
bool msata_pci_type_card_detection(void);

#endif // MSATA_PCI_H

