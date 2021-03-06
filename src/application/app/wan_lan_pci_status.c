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

    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(PCI_LLED2_PIN_PERIPH_CLOCK
                          | PCI_LLED1_PIN_PERIPH_CLOCK | WAN_LED0_PIN_PERIPH_CLOCK
                          | PCI_PLED2_PIN_PERIPH_CLOCK | PCI_PLED0_PIN_PERIPH_CLOCK
                          | PCI_PLED1_PIN_PERIPH_CLOCK | R0_P0_LED_PIN_PERIPH_CLOCK
                          | R1_P1_LED_PIN_PERIPH_CLOCK | R2_P2_LED_PIN_PERIPH_CLOCK
                          | C0_P3_LED_PIN_PERIPH_CLOCK | C1_LED_PIN_PERIPH_CLOCK
                          | C2_P4_LED_PIN_PERIPH_CLOCK | C3_P5_LED_PIN_PERIPH_CLOCK
                          | SFP_DIS_PIN_PERIPH_CLOCK, ENABLE);

    /* for compatibility with older versions of board */
    GPIO_InitStructure.GPIO_Pin = SFP_DIS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(SFP_DIS_PIN_PORT, &GPIO_InitStructure);

    GPIO_ResetBits(SFP_DIS_PIN_PORT, SFP_DIS_PIN);

    /* PCIe LED pins */
    GPIO_InitStructure.GPIO_Pin = PCI_LLED2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(PCI_LLED2_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PCI_LLED1_PIN;
    GPIO_Init(PCI_LLED1_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PCI_PLED0_PIN;
    GPIO_Init(PCI_PLED0_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PCI_PLED1_PIN;
    GPIO_Init(PCI_PLED1_PIN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = PCI_PLED2_PIN;
    GPIO_Init(PCI_PLED2_PIN_PORT, &GPIO_InitStructure);

    /* WAN LED pins */
    GPIO_InitStructure.GPIO_Pin = WAN_LED0_PIN;
    GPIO_Init(WAN_LED0_PIN_PORT, &GPIO_InitStructure);

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

	if (led_is_user_mode(WAN_LED))
		return;

	led0_status = GPIO_ReadInputDataBit(WAN_LED0_PIN_PORT, WAN_LED0_PIN);
	led_set_state(WAN_LED, !led0_status);
}

/*******************************************************************************
  * @function   pci_led_activity
  * @brief      Toggle PCIe LED according to the PCIe activity.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void pci_led_activity(void)
{
    uint8_t pcie_led1, pcie_pled1, pcie_led2, pcie_pled2;

    pcie_led2 = GPIO_ReadInputDataBit(PCI_LLED2_PIN_PORT, PCI_LLED2_PIN);
    pcie_led1 = GPIO_ReadInputDataBit(PCI_LLED1_PIN_PORT, PCI_LLED1_PIN);
    pcie_pled1 = GPIO_ReadInputDataBit(PCI_PLED1_PIN_PORT, PCI_PLED1_PIN);
    pcie_pled2 = GPIO_ReadInputDataBit(PCI_PLED2_PIN_PORT, PCI_PLED2_PIN);

    if (!led_is_user_mode(PCI2_LED))
        led_set_state(PCI2_LED, (!pcie_led2 || !pcie_pled2));

    if (!led_is_user_mode(PCI1_LED))
        led_set_state(PCI1_LED, (!pcie_led1 || !pcie_pled1));
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

	lan_led = GPIO_ReadInputData(LAN_LED_PORT) & LAN_LED_MASK;

	if (lan_led & LAN_C0_MASK) {
		if (!led_is_user_mode(LAN0_LED))
			led_set_state(LAN0_LED, !(lan_led & LAN_R0_MASK));
		if (!led_is_user_mode(LAN2_LED))
			led_set_state(LAN2_LED, !(lan_led & LAN_R1_MASK));
		if (!led_is_user_mode(LAN4_LED))
			led_set_state(LAN4_LED, !(lan_led & LAN_R2_MASK));
	}

	if (lan_led & LAN_C1_MASK) {
		if (!led_is_user_mode(LAN1_LED))
			led_set_state(LAN1_LED, !(lan_led & LAN_R0_MASK));
		if (!led_is_user_mode(LAN3_LED))
			led_set_state(LAN3_LED, !(lan_led & LAN_R1_MASK));
	}
}
