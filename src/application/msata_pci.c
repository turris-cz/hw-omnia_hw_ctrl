/**
 ******************************************************************************
 * @file    msata_pci.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    06-August-2015
 * @brief   Driver for PCIe and mSATA indication.
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "msata_pci.h"
#include "led_driver.h"

/*******************************************************************************
  * @function   msata_pci_io_config
  * @brief      GPIO configuration for mSATA and PCIe indication signals.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void msata_pci_io_config(void)
{
    gpio_init_inputs(pin_pullup, CARD_DET_PIN, MSATALED_PIN, MSATAIND_PIN);
}

/*******************************************************************************
  * @function   msata_pci_indication_config
  * @brief      Main configuration function for mSATA and PCIe status indication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void msata_pci_indication_config(void)
{
    msata_pci_io_config();
}

/*******************************************************************************
  * @function   msata_pci_activity
  * @brief      Toggle LED according to the activity of the connected card.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void msata_pci_activity(void)
{
    struct led_rgb *rgb_leds = leds;

    if (rgb_leds[MSATA_PCI_LED].led_mode == LED_DEFAULT_MODE)
        rgb_leds[MSATA_PCI_LED].led_state_default = !gpio_read(MSATALED_PIN);
}

/*******************************************************************************
  * @function   msata_pci_card_detection
  * @brief      Detect inserted card (whether a card is inserted or not)
  * @param      None.
  * @retval     1 - a card inserted, 0 - no card inserted.
  *****************************************************************************/
bool msata_pci_card_detection(void)
{
    /* inverted due to the HW connection
    HW connection: 1 = no card inserted, 0 = card inserted */
    return !gpio_read(CARD_DET_PIN);
}

/*******************************************************************************
  * @function   msata_pci_type_card_detection
  * @brief      Detect a type of inserted card - mSATA or miniPCIe
  * @param      None.
  * @retval     1 - mSATA card inserted, 0 - miniPCIe card inserted.
  *****************************************************************************/
bool msata_pci_type_card_detection(void)
{
    return gpio_read(MSATAIND_PIN);
}
