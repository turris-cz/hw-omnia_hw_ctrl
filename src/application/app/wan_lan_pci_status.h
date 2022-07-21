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

#include "stm32f0xx_conf.h"

/* PCIe status LED */
#define PCI_PLED0_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOF
#define PCI_PLED0_PIN_PORT                  GPIOF
#define PCI_PLED0_PIN                       GPIO_Pin_5

#define PCI_LLED1_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOC
#define PCI_LLED1_PIN_PORT                  GPIOC
#define PCI_LLED1_PIN                       GPIO_Pin_2

#define PCI_PLED1_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define PCI_PLED1_PIN_PORT                  GPIOA
#define PCI_PLED1_PIN                       GPIO_Pin_10

#define PCI_LLED2_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOC
#define PCI_LLED2_PIN_PORT                  GPIOC
#define PCI_LLED2_PIN                       GPIO_Pin_1

#define PCI_PLED2_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOF
#define PCI_PLED2_PIN_PORT                  GPIOF
#define PCI_PLED2_PIN                       GPIO_Pin_4

/* WAN LED */
#define WAN_LED0_PIN_PERIPH_CLOCK           RCC_AHBPeriph_GPIOF
#define WAN_LED0_PIN_PORT                   GPIOF
#define WAN_LED0_PIN                        GPIO_Pin_0

#define WAN_LED1_PIN_PERIPH_CLOCK           RCC_AHBPeriph_GPIOF
#define WAN_LED1_PIN_PORT                   GPIOF
#define WAN_LED1_PIN                        GPIO_Pin_1


/* LAN LED */
#define LAN_LED_PORT                        GPIOA
#define R0_P0_LED_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define R0_P0_LED_PIN_PORT                  GPIOA
#define R0_P0_LED_PIN                       GPIO_Pin_0

#define R1_P1_LED_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define R1_P1_LED_PIN_PORT                  GPIOA
#define R1_P1_LED_PIN                       GPIO_Pin_1

#define R2_P2_LED_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define R2_P2_LED_PIN_PORT                  GPIOA
#define R2_P2_LED_PIN                       GPIO_Pin_2

#define C0_P3_LED_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define C0_P3_LED_PIN_PORT                  GPIOA
#define C0_P3_LED_PIN                       GPIO_Pin_6

#define C1_LED_PIN_PERIPH_CLOCK             RCC_AHBPeriph_GPIOA
#define C1_LED_PIN_PORT                     GPIOA
#define C1_LED_PIN                          GPIO_Pin_8

#define C2_P4_LED_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define C2_P4_LED_PIN_PORT                  GPIOA
#define C2_P4_LED_PIN                       GPIO_Pin_11

#define C3_P5_LED_PIN_PERIPH_CLOCK          RCC_AHBPeriph_GPIOA
#define C3_P5_LED_PIN_PORT                  GPIOA
#define C3_P5_LED_PIN                       GPIO_Pin_12


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
