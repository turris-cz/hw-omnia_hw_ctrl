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
#include "debug_serial.h"

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
    GPIO_InitTypeDef  GPIO_InitStructure;
    uint32_t periph_clks;

    periph_clks =
        PCI_LLED2_PIN_PERIPH_CLOCK | PCI_LLED1_PIN_PERIPH_CLOCK |
        WAN_LED0_PIN_PERIPH_CLOCK | R0_P0_LED_PIN_PERIPH_CLOCK |
        R1_P1_LED_PIN_PERIPH_CLOCK | R2_P2_LED_PIN_PERIPH_CLOCK |
        C0_P3_LED_PIN_PERIPH_CLOCK | C1_LED_PIN_PERIPH_CLOCK |
        C2_P4_LED_PIN_PERIPH_CLOCK | C3_P5_LED_PIN_PERIPH_CLOCK;

    if (OMNIA_BOARD_REVISION >= 32)
        periph_clks |= WAN_LED1_PIN_PERIPH_CLOCK;
    else
        periph_clks |=
            PCI_PLED0_PIN_PERIPH_CLOCK |
            PCI_PLED1_PIN_PERIPH_CLOCK |
            PCI_PLED2_PIN_PERIPH_CLOCK;

    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(periph_clks, ENABLE);

    /* PCIe LED pins */
    GPIO_InitStructure.GPIO_Pin = PCI_LLED2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(PCI_LLED2_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PCI_LLED1_PIN;
    GPIO_Init(PCI_LLED1_PIN_PORT, &GPIO_InitStructure);

    if (OMNIA_BOARD_REVISION < 32) {
        GPIO_InitStructure.GPIO_Pin = PCI_PLED0_PIN;
        GPIO_Init(PCI_PLED0_PIN_PORT, &GPIO_InitStructure);

        /*
         * PCI_PLED1 pin is also used as MCU's UART RX pin, so only configure it
         * as GPIO if debugging is disabled
         */
        if (!DBG_ENABLE) {
            GPIO_InitStructure.GPIO_Pin = PCI_PLED1_PIN;
            GPIO_Init(PCI_PLED1_PIN_PORT, &GPIO_InitStructure);
        }

        GPIO_InitStructure.GPIO_Pin = PCI_PLED2_PIN;
        GPIO_Init(PCI_PLED2_PIN_PORT, &GPIO_InitStructure);
    }

    /* WAN LED pins */
    GPIO_InitStructure.GPIO_Pin = WAN_LED0_PIN;
    GPIO_Init(WAN_LED0_PIN_PORT, &GPIO_InitStructure);

    if (OMNIA_BOARD_REVISION >= 32) {
        GPIO_InitStructure.GPIO_Pin = WAN_LED1_PIN;
        GPIO_Init(WAN_LED1_PIN_PORT, &GPIO_InitStructure);
    }

    /* LAN LED input pins */
    GPIO_InitStructure.GPIO_Pin = R0_P0_LED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(R0_P0_LED_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = R1_P1_LED_PIN;
    GPIO_Init(R1_P1_LED_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = R2_P2_LED_PIN;
    GPIO_Init(R2_P2_LED_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = C0_P3_LED_PIN;
    GPIO_Init(C0_P3_LED_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = C1_LED_PIN;
    GPIO_Init(C1_LED_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = C2_P4_LED_PIN;
    GPIO_Init(C2_P4_LED_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = C3_P5_LED_PIN;
    GPIO_Init(C3_P5_LED_PIN_PORT, &GPIO_InitStructure);
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
    uint8_t led0_status;
    struct led_rgb *rgb_leds = leds;

    if (rgb_leds[WAN_LED].led_mode == LED_DEFAULT_MODE)
    {
        led0_status = GPIO_ReadInputDataBit(WAN_LED0_PIN_PORT, WAN_LED0_PIN);

        if (led0_status == 0)
        {
            rgb_leds[WAN_LED].led_state_default = LED_ON;
        }
        else
        {
            rgb_leds[WAN_LED].led_state_default = LED_OFF;
        }
    }
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

    pcie_led2 = GPIO_ReadInputDataBit(PCI_LLED2_PIN_PORT, PCI_LLED2_PIN);
    pcie_led1 = GPIO_ReadInputDataBit(PCI_LLED1_PIN_PORT, PCI_LLED1_PIN);

    if (OMNIA_BOARD_REVISION < 32) {
        uint8_t pcie_pled1, pcie_pled2;

        pcie_pled1 = GPIO_ReadInputDataBit(PCI_PLED1_PIN_PORT, PCI_PLED1_PIN);
        pcie_pled2 = GPIO_ReadInputDataBit(PCI_PLED2_PIN_PORT, PCI_PLED2_PIN);

        pcie_led1 = pcie_led1 && pcie_pled1;
        pcie_led2 = pcie_led2 && pcie_pled2;
    }

    if (rgb_leds[PCI2_LED].led_mode == LED_DEFAULT_MODE)
    {
        if(pcie_led2 == 0)
        {
            rgb_leds[PCI2_LED].led_state_default = LED_ON;
        }
        else
        {
            rgb_leds[PCI2_LED].led_state_default = LED_OFF;
        }
    }

    if (rgb_leds[PCI1_LED].led_mode == LED_DEFAULT_MODE)
    {
        if (pcie_led1 == 0)
        {
            rgb_leds[PCI1_LED].led_state_default = LED_ON;
        }
        else
        {
            rgb_leds[PCI1_LED].led_state_default = LED_OFF;
        }
    }
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

    lan_led = GPIO_ReadInputData(LAN_LED_PORT) & LAN_LED_MASK;

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
