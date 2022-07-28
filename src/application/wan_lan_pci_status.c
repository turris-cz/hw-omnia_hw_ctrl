/**
 ******************************************************************************
 * @file    wan_lan_pci_status.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    10-August-2015
 * @brief   Driver for WAN, LAN and PCIe status indication.
 ******************************************************************************
 ******************************************************************************
 **/
#include "wan_lan_pci_status.h"
#include "led_driver.h"
#include "debug.h"

enum lan_led_masks {
    LAN_LED_MASK        = 0x1947,
    LAN_R0_MASK         = 0x0001,
    LAN_R1_MASK         = 0x0002,
    LAN_R2_MASK         = 0x0004,
    LAN_C0_MASK         = 0x0040,
    LAN_C1_MASK         = 0x0100,
    LAN_C2_MASK         = 0x0800,
    LAN_C3_MASK         = 0x1000,
};

/*******************************************************************************
  * @function   wan_lan_pci_io_config
  * @brief      GPIO configuration for WAN, LAN and PCIe indication signals.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void wan_lan_pci_io_config(void)
{
    gpio_init_inputs(pin_pullup,
                     PCI_PLED0_PIN, PCI_PLED1_PIN, PCI_PLED2_PIN,
                     PCI_LLED1_PIN, PCI_LLED2_PIN,
                     WAN_LED0_PIN, WAN_LED1_PIN);

    gpio_init_inputs(pin_nopull,
                     R0_P0_LED_PIN, R1_P1_LED_PIN, R2_P2_LED_PIN,
                     C0_P3_LED_PIN, C1_LED_PIN, C2_P4_LED_PIN, C3_P5_LED_PIN);
}


/*******************************************************************************
  * @function   wan_lan_pci_config
  * @brief      Main configuration function for WAN, LAN and PCIe status indication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void wan_lan_pci_config(void)
{
    wan_lan_pci_io_config();
}

/*******************************************************************************
  * @function   wan_led_activity
  * @brief      Toggle WAN LED according to the WAN activity.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void wan_led_activity(void)
{
    struct led_rgb *rgb_leds = leds;

    if (rgb_leds[WAN_LED].led_mode == LED_DEFAULT_MODE)
        rgb_leds[WAN_LED].led_state_default = !gpio_read(WAN_LED0_PIN);
}

/*******************************************************************************
  * @function   pci_led_activity
  * @brief      Toggle PCIe LED according to the PCIe activity.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void pci_led_activity(void)
{
    uint8_t pcie_led1, pcie_led2;
    struct led_rgb *rgb_leds = leds;

    pcie_led2 = gpio_read(PCI_LLED2_PIN);
    pcie_led1 = gpio_read(PCI_LLED1_PIN);

    if (OMNIA_BOARD_REVISION < 32) {
        uint8_t pcie_pled1, pcie_pled2;

        pcie_pled1 = gpio_read(PCI_PLED1_PIN);
        pcie_pled2 = gpio_read(PCI_PLED2_PIN);

        pcie_led1 = pcie_led1 && pcie_pled1;
        pcie_led2 = pcie_led2 && pcie_pled2;
    }

    if (rgb_leds[PCI2_LED].led_mode == LED_DEFAULT_MODE)
        rgb_leds[PCI2_LED].led_state_default = !pcie_led2;

    if (rgb_leds[PCI1_LED].led_mode == LED_DEFAULT_MODE)
        rgb_leds[PCI1_LED].led_state_default = !pcie_led1;
}

/*******************************************************************************
  * @function   lan_led_activity
  * @brief      Toggle LAN LEDs according to the LAN status.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void lan_led_activity(void)
{
    uint16_t lan_led;
    struct led_rgb *rgb_leds = leds;

    lan_led = gpio_read_port(LAN_LED_PORT) & LAN_LED_MASK;

    if (rgb_leds[LAN0_LED].led_mode == LED_DEFAULT_MODE)
    {
        if((lan_led & LAN_R0_MASK) == 0)
        {
            if (lan_led & LAN_C0_MASK)
                rgb_leds[LAN0_LED].led_state_default = LED_ON;
        }
        else
        {
            if (lan_led & LAN_C0_MASK)
                rgb_leds[LAN0_LED].led_state_default = LED_OFF;
        }
    }

    if (rgb_leds[LAN1_LED].led_mode == LED_DEFAULT_MODE)
    {
        if((lan_led & LAN_R0_MASK) == 0)
        {
            if (lan_led & LAN_C1_MASK)
                rgb_leds[LAN1_LED].led_state_default = LED_ON;
        }
        else
        {
            if (lan_led & LAN_C1_MASK)
                rgb_leds[LAN1_LED].led_state_default = LED_OFF;
        }
    }

    if (rgb_leds[LAN2_LED].led_mode == LED_DEFAULT_MODE)
    {
        if((lan_led & LAN_R1_MASK) == 0)
        {
            if (lan_led & LAN_C0_MASK)
                rgb_leds[LAN2_LED].led_state_default = LED_ON;
        }
        else
        {
            if (lan_led & LAN_C0_MASK)
                rgb_leds[LAN2_LED].led_state_default = LED_OFF;
        }
    }

    if (rgb_leds[LAN3_LED].led_mode == LED_DEFAULT_MODE)
    {
        if((lan_led & LAN_R1_MASK) == 0)
        {
            if (lan_led & LAN_C1_MASK)
                rgb_leds[LAN3_LED].led_state_default = LED_ON;
        }
        else
        {
            if (lan_led & LAN_C1_MASK)
                rgb_leds[LAN3_LED].led_state_default = LED_OFF;
        }
    }

    if (rgb_leds[LAN4_LED].led_mode == LED_DEFAULT_MODE)
    {
        if((lan_led & LAN_R2_MASK) == 0)
        {
            if (lan_led & LAN_C0_MASK)
                rgb_leds[LAN4_LED].led_state_default = LED_ON;
        }
        else
        {
            if (lan_led & LAN_C0_MASK)
                rgb_leds[LAN4_LED].led_state_default = LED_OFF;
        }
    }
}
