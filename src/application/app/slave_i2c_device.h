/**
 ******************************************************************************
 * @file    slave_i2c_device.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    18-August-2015
 * @brief   Header for I2C driver.
 ******************************************************************************
 ******************************************************************************
 **/

#ifndef SLAVE_I2C_DEVICE_H
#define SLAVE_I2C_DEVICE_H

#include "bits.h"

#define MAX_RX_BUFFER_SIZE                 10
#define MAX_TX_BUFFER_SIZE                 20

typedef enum slave_i2c_states {
    SLAVE_I2C_OK,
    SLAVE_I2C_LIGHT_RST,
    SLAVE_I2C_HARD_RST,
    SLAVE_I2C_PWR4V5_ENABLE,
    SLAVE_I2C_GO_TO_BOOTLOADER
}slave_i2c_states_t;

struct st_i2c_status {
    uint16_t status_word;
    uint16_t ext_control_word;
    uint32_t ext_status_dword;
    uint8_t reset_type;
    slave_i2c_states_t state;             // reported in main state machine
    uint8_t rx_data_ctr;                  // RX data counter
    uint8_t tx_data_ctr;                  // TX data counter
    uint8_t rx_buf[MAX_RX_BUFFER_SIZE];   // RX buffer
    uint8_t tx_buf[MAX_TX_BUFFER_SIZE];   // TX buffer
    uint8_t data_tx_complete         : 1; // stop flag detected - all data sent
};

extern struct st_i2c_status i2c_status;

enum status_word_bits {
    GD32_MCU_STSBIT                     = BIT(0),
    MKL_MCU_STSBIT                      = BIT(1),
    FEATURES_SUPPORTED_STSBIT           = BIT(2),
    USER_REGULATOR_NOT_SUPPORTED_STSBIT = BIT(3),
    CARD_DET_STSBIT                     = BIT(4),
    MSATA_IND_STSBIT                    = BIT(5),
    USB30_OVC_STSBIT                    = BIT(6),
    USB31_OVC_STSBIT                    = BIT(7),
    USB30_PWRON_STSBIT                  = BIT(8),
    USB31_PWRON_STSBIT                  = BIT(9),
    ENABLE_4V5_STSBIT                   = BIT(10),
    BUTTON_MODE_STSBIT                  = BIT(11),
    BUTTON_PRESSED_STSBIT               = BIT(12),
    BUTTON_COUNTER_VALBITS              = GENMASK(15, 13)
};

enum features_bits {
    PERIPH_MCU_SUPPORTED   = BIT(0),
    EXT_CMDS_SUPPORTED     = BIT(1),
    WDT_PING_SUPPORTED     = BIT(2),
    LED_STATE_EXT_MASK     = GENMASK(4, 3),
    LED_STATE_EXT          = FIELD_PREP(LED_STATE_EXT_MASK, 1),
    LED_STATE_EXT_V32      = FIELD_PREP(LED_STATE_EXT_MASK, 2),
};

enum ext_status_dword_bits {
    SFP_NDET_STSBIT        = BIT(0),
    LED_STATES_MASK        = GENMASK(20, 1),
    WLAN0_MSATA_LED_STSBIT = BIT(1),
    WLAN1_LED_STSBIT       = BIT(2),
    WLAN2_LED_STSBIT       = BIT(3),
    WPAN0_LED_STSBIT       = BIT(4),
    WPAN1_LED_STSBIT       = BIT(5),
    WPAN2_LED_STSBIT       = BIT(6),
    WAN_LED0_STSBIT        = BIT(7),
    WAN_LED1_STSBIT        = BIT(8),
    LAN0_LED0_STSBIT       = BIT(9),
    LAN0_LED1_STSBIT       = BIT(10),
    LAN1_LED0_STSBIT       = BIT(11),
    LAN1_LED1_STSBIT       = BIT(12),
    LAN2_LED0_STSBIT       = BIT(13),
    LAN2_LED1_STSBIT       = BIT(14),
    LAN3_LED0_STSBIT       = BIT(15),
    LAN3_LED1_STSBIT       = BIT(16),
    LAN4_LED0_STSBIT       = BIT(17),
    LAN4_LED1_STSBIT       = BIT(18),
    LAN5_LED0_STSBIT       = BIT(19),
    LAN5_LED1_STSBIT       = BIT(20),
};

enum i2c_ext_control_mask {
    RES_MMC_MASK           = BIT(0),
    RES_LAN_MASK           = BIT(1),
    RES_PHY_MASK           = BIT(2),
    PERST0_MASK            = BIT(3),
    PERST1_MASK            = BIT(4),
    PERST2_MASK            = BIT(5),
    PHY_SFP_MASK           = BIT(6),
    PHY_SFP_AUTO_MASK      = BIT(7),
    VHV_CTRL_MASK          = BIT(8),
};

/*
 * Bit meanings in status_word:
 *  Bit Nr. |   Meanings
 * -----------------
 *    0,1   |   MCU_TYPE        : 00 -> STM32
 *                                01 -> GD32
 *                                10 -> MKL
 *                                11 -> reserved
 *
 * Caution! STM32 and GD32 uses Atsha for security, MKL doesn't!!!!!!!!!
 * IT IS NECESSARY TO READ AND DECODE THE FIRST TWO BITS PROPERLY!
 *
 *      2   |   FEATURES_SUPPORT: 1 - get features command supported, 0 - get features command not supported
 *      3   |   USER_REG_NOT_SUP: 1 - user regulator not supported (always "1" since GD32 MCU), 0 - user regulator may be supported (old STM32 MCU)
 *      4   |   CARD_DET        : 1 - mSATA/PCIe card detected, 0 - no card
 *      5   |   mSATA_IND       : 1 - mSATA card inserted, 0 - PCIe card inserted
 *      6   |   USB30_OVC       : 1 - USB3-port0 overcurrent, 0 - no overcurrent
 *      7   |   USB31_OVC       : 1 - USB3-port1 overcurrent, 0 - no overcurrent
 *      8   |   USB30_PWRON     : 1 - USB3-port0 power ON, 0 - USB-port0 power off
 *      9   |   USB31_PWRON     : 1 - USB3-port1 power ON, 0 - USB-port1 power off
 *     10   |   ENABLE_4V5      : 1 - 4.5V power is enabled, 0 - 4.5V power is disabled
 *     11   |   BUTTON_MODE     : 1 - user mode, 0 - default mode (brightness settings)
 *     12   |   BUTTON_PRESSED  : 1 - button pressed in user mode, 0 - button not pressed
 * 13..15   |   BUTTON_COUNT    : number of pressing of the button (max. 7) - valid in user mode
*/

/*
 * Bit meanings in features:
 *  Bit Nr. |   Meanings
 * -----------------
 *      0   |   PERIPH_MCU      : 1 - resets (eMMC, PHY, switch, PCIe), SerDes switch (PHY vs SFP cage) and VHV control are connected to MCU
 *                                    (available to set via CMD_EXT_CONTROL command)
 *                                0 - otherwise
 *      1   |   EXT_CMDS        : 1 - extended control and status commands are available, 0 - otherwise
 *      2   |   WDT_PING        : 1 - CMD_SET_WDT_TIMEOUT and CMD_GET_WDT_TIMELEFT supported, 0 - otherwise
 *    3,4   |   LED_STATE_EXT   : 00 -> LED status extension not supported in extended status word
 *                                01 -> LED status extension supported, board revision <32
 *                                10 -> LED status extension supported, board revision >=32
 *                                11 -> reserved
 *  5..15   |   reserved
*/

/*
 * Bit meanings in ext_status_dword:
 *  Bit Nr. |   Meanings
 * -----------------
 *      0   |   SFP_NDET        : 1 - no SFP detected, 0 - SFP detected
 *  1..20   |   LED states      : 1 - LED is on, 0 - LED is off
 * 21..31   |   reserved
 *
 * Meanings for LED states bits 1..20:
 *  Bit Nr. |   Meanings          | Note
 * -------------------------------------
 *      1   |   WLAN0_MSATA_LED   | note 1
 *      2   |   WLAN1_LED         | note 2
 *      3   |   WLAN2_LED         | note 2
 *      4   |   WPAN0_LED         | note 3
 *      5   |   WPAN1_LED         | note 3
 *      6   |   WPAN2_LED         | note 3
 *      7   |   WAN_LED0
 *      8   |   WAN_LED1          | note 4
 *      9   |   LAN0_LED0
 *     10   |   LAN0_LED1
 *     11   |   LAN1_LED0
 *     12   |   LAN1_LED1
 *     13   |   LAN2_LED0
 *     14   |   LAN2_LED1
 *     15   |   LAN3_LED0
 *     16   |   LAN3_LED1
 *     17   |   LAN4_LED0
 *     18   |   LAN4_LED1
 *     19   |   LAN5_LED0
 *     20   |   LAN5_LED1
 * 21..31   |   reserved
 *
 * Notes: in the following notes, pre-v32 and v32+ boards can be determined
 *        from the LED_STATE_EXT field of the features word.
 * note 1: On pre-v32 boards, WLAN0_MSATA_LED corresponds (as logical OR) to
 *         nLED_WLAN and DA_DSS pins of the MiniPCIe/mSATA port.
 *         On v32+ boards it corresponds also to the nLED_WWAN and nLED_WPAN
 *         pins.
 * note 2: On pre-v32 boards, WLAN*_LED corresponds to the nLED_WLAN pin of the
 *         MiniPCIe port.
 *         On v32+ boards it corresponds (as logical OR) to nLED_WWAN, nLED_WLAN
 *         and nLED_WPAN pins.
 * note 3: On pre-v32 boards, WPAN*_LED bits correspond to the nLED_WPAN pins of
 *         the MiniPCIe port.
 *         On v32+ boards, WPAN*_LED bits are unavailable, because their
 *         functionality is ORed in WLAN*_LED bits.
 * note 4: WAN_LED1 is only available on v32+ boards.
*/

/*
 * Byte meanings in reset_byte:
 *  Byte Nr. |   Meanings
 * -----------------
 *   1.B    |   RESET_TYPE      : 0 - normal reset, 1 - previous snapshot,
 *                              2 - normal factory reset, 3 - hard factory reset
*/

/*
 * Bit meanings in control_byte:
 *  Bit Nr. |   Meanings
 * -----------------
 *      0   |   LIGHT_RST   : 1 - do light reset, 0 - no reset
 *      1   |   HARD_RST    : 1 - do hard reset, 0 - no reset
 *      2   |   dont care
 *      3   |   USB30_PWRON : 1 - USB3-port0 power ON, 0 - USB-port0 power off
 *      4   |   USB31_PWRON : 1 - USB3-port1 power ON, 0 - USB-port1 power off
 *      5   |   ENABLE_4V5  : 1 - 4.5V power supply ON, 0 - 4.5V power supply OFF
 *      6   |   BUTTON_MODE : 1 - user mode, 0 - default mode (brightness settings)
 *      7   |   BOOTLOADER  : 1 - jump to bootloader
*/

/*
 * Bit meanings in extended control byte:
 *  Bit Nr. |   Meanings
 * -----------------
 *      0   |   RES_MMC   : 1 - reset of MMC, 0 - no reset
 *      1   |   RES_LAN   : 1 - reset of LAN switch, 0 - no reset
 *      2   |   RES_PHY   : 1 - reset of PHY WAN, 0 - no reset
 *      3   |   PERST0    : 1 - reset of PCIE0, 0 - no reset
 *      4   |   PERST1    : 1 - reset of PCIE1, 0 - no reset
 *      5   |   PERST2    : 1 - reset of PCIE2, 0 - no reset
 *      6   |   PHY_SFP   : 1 - PHY WAN mode, 0 - SFP WAN mode
 *      7   |   PHY_SFP_AUTO : 1 - automatically switch between PHY and SFP WAN modes
 *                             0 - PHY/SFP WAN mode determined by value written to PHY_SFP bit
 *      8   |   VHV_CTRL  : 1 - VHV control not active, 0 - VHV control voltage active
 *  9..15   |   reserved
*/

/*
 * Bit meanings in led_mode_byte:
 *  Bit Nr. |   Meanings
 * -----------------
 *   0..3   |   LED number [0..11] (or in case setting of all LED at once -> LED number = 12)
 *      4   |   LED mode    : 1 - USER mode, 0 - default mode
 *   5..7   |   dont care
*/

/*
 * Bit meanings in led_state_byte:
 *  Bit Nr. |   Meanings
 * -----------------
 *   0..3   |   LED number [0..11] (or in case setting of all LED at once -> LED number = 12)
 *      4   |   LED state    : 1 - LED ON, 0 - LED OFF
 *   5..7   |   dont care
*/

/*
 * Bit meanings in led_colour:
 * Byte Nr. |  Bit Nr. |   Meanings
 * -----------------
 *  1.B     |  0..3   |   LED number [0..11] (or in case setting of all LED at once -> LED number = 12)
 *  1.B     |  4..7   |   dont care
 *  2.B     |  8..15  |   red colour [0..255]
 *  3.B     |  16..23 |   green colour [0..255]
 *  4.B     |  24..31 |   blue colour [0..255]
*/

/*******************************************************************************
  * @function   slave_i2c_config
  * @brief      Configuration of I2C peripheral as a slave.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void slave_i2c_config(void);

/*******************************************************************************
  * @function   slave_i2c_handler
  * @brief      Interrupt handler for I2C communication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void slave_i2c_handler(void);

#endif /* SLAVE_I2C_DEVICE_H */

