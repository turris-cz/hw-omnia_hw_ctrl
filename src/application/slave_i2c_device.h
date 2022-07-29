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
};

extern struct st_i2c_status i2c_status;

enum commands_e {
    CMD_GET_STATUS_WORD                 = 0x01, /* slave sends status word back */
    CMD_GENERAL_CONTROL                 = 0x02,
    CMD_LED_MODE                        = 0x03, /* default/user */
    CMD_LED_STATE                       = 0x04, /* LED on/off */
    CMD_LED_COLOUR                      = 0x05, /* LED number + RED + GREEN + BLUE */
    CMD_USER_VOLTAGE                    = 0x06,
    CMD_SET_BRIGHTNESS                  = 0x07,
    CMD_GET_BRIGHTNESS                  = 0x08,
    CMD_GET_RESET                       = 0x09,
    CMD_GET_FW_VERSION_APP              = 0x0A, /* 20B git hash number */
    CMD_WATCHDOG_STATE                  = 0x0B, /* 0 - STOP, 1 - RUN -> must be stopped in less than 2 mins after reset */
    CMD_WATCHDOG_STATUS                 = 0x0C, /* 0 - DISABLE, 1 - ENABLE -> permanently */
    CMD_GET_WATCHDOG_STATE              = 0x0D,
    CMD_GET_FW_VERSION_BOOT             = 0x0E, /* 20B git hash number */
    CMD_GET_FW_CHECKSUM                 = 0x0F, /* 4B length, 4B checksum */

    /* available if FEATURES_SUPPORTED bit set in status word */
    CMD_GET_FEATURES                    = 0x10,

    /* available if EXT_CMD bit set in features */
    CMD_GET_EXT_STATUS_DWORD            = 0x11,
    CMD_EXT_CONTROL                     = 0x12,
    CMD_GET_EXT_CONTROL_STATUS          = 0x13,

    /* available if WDT_PING bit set in features */
    CMD_SET_WDT_TIMEOUT                 = 0x20,
    CMD_GET_WDT_TIMELEFT                = 0x21,
};

enum sts_word_e {
    STS_MCU_TYPE_MASK                = GENMASK(1, 0),
    STS_MCU_TYPE_STM32               = FIELD_PREP(STS_MCU_TYPE_MASK, 0),
    STS_MCU_TYPE_GD32                = FIELD_PREP(STS_MCU_TYPE_MASK, 1),
    STS_MCU_TYPE_MKL                 = FIELD_PREP(STS_MCU_TYPE_MASK, 2),
    STS_FEATURES_SUPPORTED           = BIT(2),
    STS_USER_REGULATOR_NOT_SUPPORTED = BIT(3),
    STS_CARD_DET                     = BIT(4),
    STS_MSATA_IND                    = BIT(5),
    STS_USB30_OVC                    = BIT(6),
    STS_USB31_OVC                    = BIT(7),
    STS_USB30_PWRON                  = BIT(8),
    STS_USB31_PWRON                  = BIT(9),
    STS_ENABLE_4V5                   = BIT(10),
    STS_BUTTON_MODE                  = BIT(11),
    STS_BUTTON_PRESSED               = BIT(12),
    STS_BUTTON_COUNTER_MASK          = GENMASK(15, 13)
};

enum ctl_byte_e {
    CTL_LIGHT_RST   = BIT(0),
    CTL_HARD_RST    = BIT(1),
    /*CTL_RESERVED    = BIT(2),*/
    CTL_USB30_PWRON = BIT(3),
    CTL_USB31_PWRON = BIT(4),
    CTL_ENABLE_4V5  = BIT(5),
    CTL_BUTTON_MODE = BIT(6),
    CTL_BOOTLOADER  = BIT(7)
};

enum features_e {
    FEAT_PERIPH_MCU         = BIT(0),
    FEAT_EXT_CMDS           = BIT(1),
    FEAT_WDT_PING           = BIT(2),
    FEAT_LED_STATE_EXT_MASK = GENMASK(4, 3),
    FEAT_LED_STATE_EXT      = FIELD_PREP(FEAT_LED_STATE_EXT_MASK, 1),
    FEAT_LED_STATE_EXT_V32  = FIELD_PREP(FEAT_LED_STATE_EXT_MASK, 2),
};

enum ext_sts_dword_e {
    EXT_STS_SFP_NDET        = BIT(0),
    EXT_STS_LED_STATES_MASK = GENMASK(20, 1),
    EXT_STS_WLAN0_MSATA_LED = BIT(1),
    EXT_STS_WLAN1_LED       = BIT(2),
    EXT_STS_WLAN2_LED       = BIT(3),
    EXT_STS_WPAN0_LED       = BIT(4),
    EXT_STS_WPAN1_LED       = BIT(5),
    EXT_STS_WPAN2_LED       = BIT(6),
    EXT_STS_WAN_LED0        = BIT(7),
    EXT_STS_WAN_LED1        = BIT(8),
    EXT_STS_LAN0_LED0       = BIT(9),
    EXT_STS_LAN0_LED1       = BIT(10),
    EXT_STS_LAN1_LED0       = BIT(11),
    EXT_STS_LAN1_LED1       = BIT(12),
    EXT_STS_LAN2_LED0       = BIT(13),
    EXT_STS_LAN2_LED1       = BIT(14),
    EXT_STS_LAN3_LED0       = BIT(15),
    EXT_STS_LAN3_LED1       = BIT(16),
    EXT_STS_LAN4_LED0       = BIT(17),
    EXT_STS_LAN4_LED1       = BIT(18),
    EXT_STS_LAN5_LED0       = BIT(19),
    EXT_STS_LAN5_LED1       = BIT(20),
};

enum ext_ctl_e {
    EXT_CTL_nRES_MMC     = BIT(0),
    EXT_CTL_nRES_LAN     = BIT(1),
    EXT_CTL_nRES_PHY     = BIT(2),
    EXT_CTL_nPERST0      = BIT(3),
    EXT_CTL_nPERST1      = BIT(4),
    EXT_CTL_nPERST2      = BIT(5),
    EXT_CTL_PHY_SFP      = BIT(6),
    EXT_CTL_PHY_SFP_AUTO = BIT(7),
    EXT_CTL_nVHV_CTRL    = BIT(8),
};

/*
 * Bit meanings in status word:
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
 * Bit meanings in extended status dword:
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
 * Byte meanings in reset byte:
 *  Byte Nr. |   Meanings
 * -----------------
 *   1.B    |   RESET_TYPE      : 0 - normal reset, 1 - previous snapshot,
 *                              2 - normal factory reset, 3 - hard factory reset
*/

/*
 * Bit meanings in control byte:
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
 * Bit meanings in extended control dword:
 *  Bit Nr. |   Meanings
 * -----------------
 *      0   |   nRES_MMC     : 0 - reset of MMC, 1 - no reset
 *      1   |   nRES_LAN     : 0 - reset of LAN switch, 1 - no reset
 *      2   |   nRES_PHY     : 0 - reset of PHY WAN, 1 - no reset
 *      3   |   nPERST0      : 0 - reset of PCIE0, 1 - no reset
 *      4   |   nPERST1      : 0 - reset of PCIE1, 1 - no reset
 *      5   |   nPERST2      : 0 - reset of PCIE2, 1 - no reset
 *      6   |   PHY_SFP      : 1 - PHY WAN mode, 0 - SFP WAN mode
 *      7   |   PHY_SFP_AUTO : 1 - automatically switch between PHY and SFP WAN modes
 *                             0 - PHY/SFP WAN mode determined by value written to PHY_SFP bit
 *      8   |   nVHV_CTRL    : 1 - VHV control not active, 0 - VHV control voltage active
 *  9..15   |   reserved
*/

/*
 * Bit meanings in led mode byte:
 *  Bit Nr. |   Meanings
 * -----------------
 *   0..3   |   LED number [0..11] (or in case setting of all LED at once -> LED number = 12)
 *      4   |   LED mode    : 1 - USER mode, 0 - default mode
 *   5..7   |   dont care
*/

/*
 * Bit meanings in led state byte:
 *  Bit Nr. |   Meanings
 * -----------------
 *   0..3   |   LED number [0..11] (or in case setting of all LED at once -> LED number = 12)
 *      4   |   LED state    : 1 - LED ON, 0 - LED OFF
 *   5..7   |   dont care
*/

/*
 * Bit meanings in led colour:
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

#endif /* SLAVE_I2C_DEVICE_H */

