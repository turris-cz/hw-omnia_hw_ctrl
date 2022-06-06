/**
 ******************************************************************************
 * @file    slave_i2c_device.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    18-August-2015
 * @brief   Driver for IC2 communication with master device (main CPU).
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "compiler.h"
#include "stm32f0xx_conf.h"
#include "slave_i2c_device.h"
#include "debug_serial.h"
#include "led_driver.h"
#include "wan_lan_pci_status.h"
#include "power_control.h"
#include "delay.h"
#include "debounce.h"
#include "eeprom.h"
#include "msata_pci.h"

static const uint8_t version[] = VERSION;

#define I2C_SDA_SOURCE                  GPIO_PinSource7
#define I2C_SCL_SOURCE                  GPIO_PinSource6

#define I2C_ALTERNATE_FUNCTION          GPIO_AF_1
#define I2C_TIMING                      0x10800000 /* 100kHz for 48MHz system clock */

#define I2C_GPIO_CLOCK                  RCC_AHBPeriph_GPIOF
#define I2C_PERIPH_NAME                 I2C2
#define I2C_PERIPH_CLOCK                RCC_APB1Periph_I2C2
#define I2C_DATA_PIN                    GPIO_Pin_7 /* I2C2_SDA - GPIOF */
#define I2C_CLK_PIN                     GPIO_Pin_6 /* I2C2_SCL - GPIOF */
#define I2C_GPIO_PORT                   GPIOF

#define I2C_SLAVE_ADDRESS               0x55  /* address in linux: 0x2A */
#define I2C_SLAVE_ADDRESS_EMULATOR      0x56  /* address in linux: 0x2B */

#define CMD_INDEX                       0
#define NUMBER_OF_BYTES_VERSION         MAX_TX_BUFFER_SIZE
#define BOOTLOADER_VERSION_ADDR         0x080000C0

static const uint16_t slave_features_supported =
	PERIPH_MCU_SUPPORTED |
	EXT_CMDS_SUPPORTED;

enum i2c_commands {
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

    /* new commands */
    CMD_GET_FEATURES                    = 0x10,
    CMD_GET_EXT_STATUS_DWORD            = 0x11,
    CMD_EXT_CONTROL                     = 0x12,
    CMD_GET_EXT_CONTROL_STATUS          = 0x13,

    /* available if WDT_PING bit set in features */
    CMD_SET_WDT_TIMEOUT                 = 0x20,
    CMD_GET_WDT_TIMELEFT                = 0x21,
};

enum i2c_control_byte_mask {
    LIGHT_RST_MASK                      = 0x01,
    HARD_RST_MASK                       = 0x02,
    RESERVED_MASK                       = 0x04,
    USB30_PWRON_MASK                    = 0x08,
    USB31_PWRON_MASK                    = 0x10,
    ENABLE_4V5_MASK                     = 0x20,
    BUTTON_MODE_MASK                    = 0x40,
    BOOTLOADER_MASK                     = 0x80
};

enum expected_bytes_in_cmd {
    ONE_BYTE_EXPECTED                   = 1,
    TWO_BYTES_EXPECTED                  = 2,
    FOUR_BYTES_EXPECTED                 = 4,
    TWENTY_BYTES_EXPECTED               = 20
};

typedef enum i2c_dir {
    I2C_DIR_TRANSMITTER_MCU             = 0,
    I2C_DIR_RECEIVER_MCU                = 1,
    I2C_DIR_TRANSMITTER_EMULATOR        = 3,
    I2C_DIR_RECEIVER_EMULATOR           = 4,
} i2c_dir_t;

enum boot_requests {
    BOOTLOADER_REQ                      = 0xAA,
    FLASH_ERROR                         = 0x55,
    FLASH_OK                            = 0x88
};

struct st_i2c_status i2c_status;

/*******************************************************************************
  * @brief  This function reads data from flash, byte after byte
  * @param  flash_address: start of selected flash area to be read
  * @param  data: data from flash
  * @retval None.
  *****************************************************************************/
static void flash_read(volatile uint32_t *flash_address, uint8_t *data)
{
   *data = *(uint8_t*)*flash_address;
    (*flash_address)++;
}

/*******************************************************************************
  * @brief  This function reads version of bootloader (stored in flash)
  * @param  flash_address: start of selected flash area to be read
  * @param  data: data from flash
  * @retval None.
  *****************************************************************************/
static void read_bootloader_version(uint8_t buff[])
{
    uint8_t idx;
    uint32_t boot_version_addr = BOOTLOADER_VERSION_ADDR;

    for(idx = 0; idx < NUMBER_OF_BYTES_VERSION; idx++)
    {
        flash_read(&boot_version_addr, &(buff[idx]));
    }
}

/*******************************************************************************
  * @function   slave_i2c_config
  * @brief      Configuration of pins for I2C.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void slave_i2c_io_config(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* I2C Peripheral Disable */
    RCC_APB1PeriphClockCmd(I2C_PERIPH_CLOCK, DISABLE);

    /* I2C Periph clock enable */
    RCC_APB1PeriphClockCmd(I2C_PERIPH_CLOCK, ENABLE);

    RCC_AHBPeriphClockCmd(I2C_GPIO_CLOCK, ENABLE);

    /* Connect PXx to I2C_SCL */
    GPIO_PinAFConfig(I2C_GPIO_PORT, I2C_SCL_SOURCE, I2C_ALTERNATE_FUNCTION);

    /* Connect PXx to I2C_SDA */
    GPIO_PinAFConfig(I2C_GPIO_PORT, I2C_SDA_SOURCE, I2C_ALTERNATE_FUNCTION);

    /* Configure I2C pins: SCL */
    GPIO_InitStructure.GPIO_Pin = I2C_CLK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStructure);

    /* Configure I2C pins: SDA */
    GPIO_InitStructure.GPIO_Pin = I2C_DATA_PIN;
    GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStructure);
}

/*******************************************************************************
  * @function   slave_i2c_periph_config
  * @brief      Configuration of I2C peripheral as a slave.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void slave_i2c_periph_config(void)
{
    I2C_InitTypeDef  I2C_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    I2C_DeInit(I2C_PERIPH_NAME);
    I2C_Cmd(I2C_PERIPH_NAME, DISABLE);

    /* I2C configuration */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
    I2C_InitStructure.I2C_DigitalFilter = 0x00;
    I2C_InitStructure.I2C_OwnAddress1 = I2C_SLAVE_ADDRESS;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_Timing = I2C_TIMING;

    /* Apply I2C configuration after enabling it */
    I2C_Init(I2C_PERIPH_NAME, &I2C_InitStructure);

    I2C_DualAddressCmd(I2C_PERIPH_NAME, DISABLE);
    I2C_OwnAddress2Config(I2C_PERIPH_NAME, I2C_SLAVE_ADDRESS_EMULATOR, I2C_OA2_Mask01);
    I2C_DualAddressCmd(I2C_PERIPH_NAME, ENABLE);

    I2C_SlaveByteControlCmd(I2C_PERIPH_NAME, ENABLE);
    I2C_ReloadCmd(I2C_PERIPH_NAME, ENABLE);

    /* Address match, transfer complete, stop and transmit interrupt */
    I2C_ITConfig(I2C_PERIPH_NAME, I2C_IT_ADDRI | I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_TXI, ENABLE);

    /* I2C Peripheral Enable */
    I2C_Cmd(I2C_PERIPH_NAME, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = I2C2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
  * @function   slave_i2c_config
  * @brief      Configuration of I2C peripheral and its timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void slave_i2c_config(void)
{
    slave_i2c_io_config();
    slave_i2c_periph_config();
}

/*******************************************************************************
  * @function   slave_i2c_check_control_byte
  * @brief      Decodes a control byte and perform suitable reaction.
  * @param      control_byte: control byte sent from master (CPU)
  * @param      bit_mask: 0 - dont care bit, 1 - write bit
  * @retval     None.
  *****************************************************************************/
static void slave_i2c_check_control_byte(uint8_t control_byte, uint8_t bit_mask)
{
    struct st_i2c_status *i2c_control = &i2c_status;
    struct button_def *button = &button_front;
    eeprom_var_t ee_var;

    i2c_control->state = SLAVE_I2C_OK;

    if ((control_byte & LIGHT_RST_MASK) && (bit_mask & LIGHT_RST_MASK))
    {
        /* confirm received byte of I2C and reset */
        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
        /* release SCL line */
        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
        /* set CFG_CTRL pin to high state ASAP */
        GPIO_SetBits(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);
        /* reset of CPU */
        GPIO_ResetBits(MANRES_PIN_PORT, MANRES_PIN);
        return;
    }

    if ((control_byte & HARD_RST_MASK) && (bit_mask & HARD_RST_MASK))
    {
        i2c_control->state = SLAVE_I2C_HARD_RST;
        return;
    }

    if (bit_mask & USB30_PWRON_MASK)
    {
        if (control_byte & USB30_PWRON_MASK)
        {
            power_control_usb(USB3_PORT0, USB_ON);
            i2c_control->status_word |= USB30_PWRON_STSBIT;
        }
        else
        {
            power_control_usb(USB3_PORT0, USB_OFF);
            i2c_control->status_word &= (~USB30_PWRON_STSBIT);
        }
    }

    if (bit_mask & USB31_PWRON_MASK)
    {
        if (control_byte & USB31_PWRON_MASK)
        {
            power_control_usb(USB3_PORT1, USB_ON);
            i2c_control->status_word |= USB31_PWRON_STSBIT;
        }
        else
        {
            power_control_usb(USB3_PORT1, USB_OFF);
            i2c_control->status_word &= (~USB31_PWRON_STSBIT);
        }
    }

#if USER_REGULATOR_ENABLED
    if (bit_mask & ENABLE_4V5_MASK)
    {
        if (control_byte & ENABLE_4V5_MASK)
        {
            i2c_control->state = SLAVE_I2C_PWR4V5_ENABLE;
        }
        else
        {
            GPIO_ResetBits(ENABLE_4V5_PIN_PORT, ENABLE_4V5_PIN);
            i2c_control->status_word &= (~ENABLE_4V5_STSBIT);
        }
    }
#endif

    if (bit_mask & BUTTON_MODE_MASK)
    {
        if (control_byte & BUTTON_MODE_MASK)
        {
           button->button_mode = BUTTON_USER;
           i2c_control->status_word |= BUTTON_MODE_STSBIT;
        }
        else
        {
           button->button_mode = BUTTON_DEFAULT;
           button->button_pressed_counter = 0;
           i2c_control->status_word &= (~BUTTON_MODE_STSBIT);
        }
    }

    if (bit_mask & BOOTLOADER_MASK)
    {
        if (control_byte & BOOTLOADER_MASK)
        {
            ee_var = EE_WriteVariable(RESET_VIRT_ADDR, BOOTLOADER_REQ);

            switch(ee_var)
            {
                case VAR_FLASH_COMPLETE:    DBG("RST: OK\r\n"); break;
                case VAR_PAGE_FULL:         DBG("RST: Pg full\r\n"); break;
                case VAR_NO_VALID_PAGE:     DBG("RST: No Pg\r\n"); break;
                default:
                    break;
            }

            i2c_control->state = SLAVE_I2C_GO_TO_BOOTLOADER;
        }
    }
}

static const struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint16_t mask;
    bool inv;
} ext_control_pins[] = {
#define ECTRL(n, i) \
    { n ## _PIN_PORT, n ## _PIN, n ## _MASK, i }
    ECTRL(RES_MMC, 1),
    ECTRL(RES_LAN, 1),
    ECTRL(RES_PHY, 1),
    ECTRL(PERST0, 1),
    ECTRL(PERST1, 1),
    ECTRL(PERST2, 1),
    ECTRL(PHY_SFP, 0),
    ECTRL(VHV_CTRL, 1),
#undef ECTRL
};

/*******************************************************************************
  * @function   slave_i2c_ext_control
  * @brief      Decodes an extended control word and performs suitable reaction.
  * @param      ext_control_word: extended control word sent from master (CPU)
  * @param      bit_mask: 0 - dont care bit, 1 - write bit
  * @retval     None.
  *****************************************************************************/
void slave_i2c_ext_control(uint16_t ext_control_word, uint16_t bit_mask)
{
    /* save the requested value */
    ext_control_word = (i2c_status.ext_control_word & ~bit_mask) |
                       (ext_control_word & bit_mask);
    i2c_status.ext_control_word = ext_control_word;

    /*
     * PHY_SFP_AUTO isn't a GPIO, rather an internal setting.
     * If set, we let PHY_SFP to be set in app.c' input_manager() according to
     * value read from SFP_NDET, so we don't change it here.
     * If not set, we want to set PHY_SFP according to value in
     * ext_control_word.
     */
    if (ext_control_word & PHY_SFP_AUTO_MASK)
        bit_mask &= ~PHY_SFP_MASK;
    else
        bit_mask |= PHY_SFP_MASK;

    for_each_const(pin, ext_control_pins)
        if (bit_mask & pin->mask)
            GPIO_WriteBit(pin->port, pin->pin,
                          !!(ext_control_word & pin->mask) ^ pin->inv);
}

/*******************************************************************************
  * @function   slave_i2c_get_ext_control_status
  * @brief      Get extended control status (peripheral's resets, ...).
  * @param      None.
  * @retval     ext_control_status.
  *****************************************************************************/
static uint16_t slave_i2c_get_ext_control_status(void)
{
    uint16_t ext_control_status = 0;

    for_each_const(pin, ext_control_pins)
        if (GPIO_ReadInputDataBit(pin->port, pin->pin) ^ pin->inv)
            ext_control_status |= pin->mask;

    /* PHY_SFP_AUTO isn't a GPIO, rather an internal setting about behavior */
    ext_control_status |= i2c_status.ext_control_word & PHY_SFP_AUTO_MASK;

    return ext_control_status;
}

/*******************************************************************************
  * @function   slave_i2c_handler
  * @brief      Interrupt handler for I2C communication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void slave_i2c_handler(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    struct st_watchdog *wdg = &watchdog;
    static i2c_dir_t direction;
    struct led_rgb *led = leds;
    uint16_t idx;
    uint8_t led_index;
    uint32_t colour;
    eeprom_var_t ee_var;
    uint8_t address;

    __disable_irq();

    /* address match interrupt */
    if(I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_ADDR) == SET)
    {
        /* Clear IT pending bit */
        I2C_ClearITPendingBit(I2C_PERIPH_NAME, I2C_IT_ADDR);

        /* Check if transfer direction is read (slave transmitter) */
        if ((I2C_PERIPH_NAME->ISR & I2C_ISR_DIR) == I2C_ISR_DIR)
        {
            address = I2C_GetAddressMatched(I2C_PERIPH_NAME);

            if (address == I2C_SLAVE_ADDRESS_EMULATOR)
            {
                direction = I2C_DIR_TRANSMITTER_EMULATOR;
            }
            else
            {
                direction = I2C_DIR_TRANSMITTER_MCU;
            }

            DBG("S.TX\r\n");
        }
        else
        {
            address = I2C_GetAddressMatched(I2C_PERIPH_NAME);

            if (address == I2C_SLAVE_ADDRESS_EMULATOR)
            {
                direction = I2C_DIR_RECEIVER_EMULATOR;
            }
            else
            {
                direction = I2C_DIR_RECEIVER_MCU;
            }

            I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
            DBG("S.RX\r\n");
        }
    }
    /* transmit interrupt */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_TXIS) == SET)
    {
        I2C_SendData(I2C_PERIPH_NAME, i2c_state->tx_buf[i2c_state->tx_data_ctr++]);
        DBG("send\r\n");
    }
    /* transfer complete interrupt (TX and RX) */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_TCR) == SET)
    {
        if(direction == I2C_DIR_RECEIVER_MCU)
        {
            i2c_state->rx_buf[i2c_state->rx_data_ctr++] = I2C_ReceiveData(I2C_PERIPH_NAME);

            /* if more bytes than MAX_RX_BUFFER_SIZE received -> NACK */
            if (i2c_state->rx_data_ctr > MAX_RX_BUFFER_SIZE)
            {
                i2c_state->rx_data_ctr = 0;
                DBG("NACK-MAX\r\n");
                I2C_AcknowledgeConfig(I2C_PERIPH_NAME, DISABLE);
                I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                return;
            }

            /* check if the command (register) exists and send ACK */
            switch(i2c_state->rx_buf[CMD_INDEX])
            {
                case CMD_GENERAL_CONTROL:
                {
                    if((i2c_state->rx_data_ctr -1) == TWO_BYTES_EXPECTED)
                    {
                        slave_i2c_check_control_byte(i2c_state->rx_buf[1], \
                        i2c_state->rx_buf[2]);
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_LED_MODE:
                {
                    if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                    {
                        led_driver_set_led_mode(i2c_state->rx_buf[1] & 0x0F, \
                        (i2c_state->rx_buf[1] & 0x10) >> 4);

                        DBG("set LED mode - LED index : ");
                        DBG((const char*)(i2c_state->rx_buf[1] & 0x0F));
                        DBG("\r\nLED mode: ");
                        DBG((const char*)((i2c_state->rx_buf[1] & 0x0F) >> 4));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_LED_STATE:
                {
                    if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                    {
                        led_driver_set_led_state_user(i2c_state->rx_buf[1] &
                                     0x0F, (i2c_state->rx_buf[1] & 0x10) >> 4);

                        DBG("set LED state - LED index : ");
                        DBG((const char*)(i2c_state->rx_buf[1] & 0x0F));
                        DBG("\r\nLED state: ");
                        DBG((const char*)((i2c_state->rx_buf[1] & 0x0F) >> 4));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_LED_COLOUR:
                {
                    if((i2c_state->rx_data_ctr -1) == FOUR_BYTES_EXPECTED)
                    {
                        led_index = i2c_state->rx_buf[1] & 0x0F;
                        /* colour = Red + Green + Blue */
                        colour = (i2c_state->rx_buf[2] << 16) | \
                        (i2c_state->rx_buf[3] << 8) | i2c_state->rx_buf[4];

                        led_driver_set_colour(led_index, colour);

                        DBG("set LED colour - LED index : ")
                        DBG((const char*)&led_index);
                        DBG("\r\nRED: ");
                        DBG((const char*)(i2c_state->rx_buf + 2));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_SET_BRIGHTNESS:
                {
                    if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                    {
                        led_driver_pwm_set_brightness(i2c_state->rx_buf[1]);

                        DBG("brightness: ");
                        DBG((const char*)(i2c_state->rx_buf + 1));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

#if USER_REGULATOR_ENABLED
                case CMD_USER_VOLTAGE:
                {
                    if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                    {
                        power_control_set_voltage(i2c_state->rx_buf[1]);

                        DBG("user voltage: ");
                        DBG((const char*)(i2c_state->rx_buf + 1));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;
#endif

                case CMD_WATCHDOG_STATE:
                {
                    if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                    {
                        wdg->watchdog_state = i2c_state->rx_buf[1];

                        DBG("WDT STATE: ");
                        DBG((const char*)(i2c_state->rx_buf + 1));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_WATCHDOG_STATUS:
                {
                    if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                    {
                        wdg->watchdog_sts = i2c_state->rx_buf[1];

                        ee_var = EE_WriteVariable(WDG_VIRT_ADDR, wdg->watchdog_sts);

                        switch(ee_var)
                        {
                            case VAR_FLASH_COMPLETE: DBG("WDT: OK\r\n"); break;
                            case VAR_PAGE_FULL: DBG("WDT: Pg full\r\n"); break;
                            case VAR_NO_VALID_PAGE: DBG("WDT: No Pg\r\n"); break;
                            default:
                                break;
                        }

                        DBG("WDT: ");
                        DBG((const char*)(i2c_state->rx_buf + 1));
                        DBG("\r\n");
                    }
                    DBG("ACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_GET_STATUS_WORD:
                {
                    /* prepare data to be sent to the master */
                    i2c_state->tx_buf[0] = i2c_state->status_word & 0x00FF;
                    i2c_state->tx_buf[1] = (i2c_state->status_word & 0xFF00) >> 8;
                    DBG("STS\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, TWO_BYTES_EXPECTED);
                } break;

                case CMD_GET_BRIGHTNESS:
                {
                    i2c_state->tx_buf[0] = led->brightness;
                    DBG("brig\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_GET_RESET:
                {
                    i2c_state->tx_buf[0] = i2c_state->reset_type;
                    DBG("RST\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_GET_WATCHDOG_STATE:
                {
                    i2c_state->tx_buf[0] = wdg->watchdog_state;
                    DBG("WDT GET\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                /* read fw version of the current application */
                case CMD_GET_FW_VERSION_APP:
                {
                    for (idx = 0; idx < MAX_TX_BUFFER_SIZE; idx++)
                    {
                        i2c_state->tx_buf[idx] = version[idx];
                    }
                    DBG("FWA\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, TWENTY_BYTES_EXPECTED);
                } break;

                /* read fw version of the bootloader */
                case CMD_GET_FW_VERSION_BOOT:
                {
                    read_bootloader_version(i2c_state->tx_buf);

                    DBG("FWB\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, TWENTY_BYTES_EXPECTED);
                } break;

                case CMD_EXT_CONTROL:
                {
                    if((i2c_state->rx_data_ctr -1) == FOUR_BYTES_EXPECTED)
                    {
                        slave_i2c_ext_control(i2c_state->rx_buf[1] |
                                              (i2c_state->rx_buf[2] << 8),
                                              i2c_state->rx_buf[3] |
                                              (i2c_state->rx_buf[4] << 8));
                    }
                    DBG("EXT_CTRL\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    /* release SCL line */
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;

                case CMD_GET_EXT_CONTROL_STATUS:
                {
                    uint16_t ext_ctrl = slave_i2c_get_ext_control_status();
                    i2c_state->tx_buf[0] = ext_ctrl & 0x00FF;
                    i2c_state->tx_buf[1] = (ext_ctrl & 0xFF00) >> 8;
                    DBG("GET_EXT_CTRL\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, TWO_BYTES_EXPECTED);
                } break;

                case CMD_GET_FEATURES:
                {
                    /* prepare data to be sent to the master */
                    i2c_state->tx_buf[0] = slave_features_supported & 0x00FF;
                    i2c_state->tx_buf[1] = (slave_features_supported & 0xFF00) >> 8;
                    DBG("FEAT\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, TWO_BYTES_EXPECTED);
                } break;

                case CMD_GET_EXT_STATUS_DWORD:
                {
                    /* prepare data to be sent to the master */
                    i2c_state->tx_buf[0] = i2c_state->ext_status_dword & 0xFF;
                    i2c_state->tx_buf[1] = (i2c_state->ext_status_dword >> 8) & 0xFF;
                    i2c_state->tx_buf[2] = (i2c_state->ext_status_dword >> 16) & 0xFF;
                    i2c_state->tx_buf[3] = (i2c_state->ext_status_dword >> 24) & 0xFF;
                    DBG("EXT_STS\r\n");

                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, FOUR_BYTES_EXPECTED);
                } break;

                default: /* command doesnt exist - send NACK */
                {
                    DBG("NACK\r\n");
                    I2C_AcknowledgeConfig(I2C_PERIPH_NAME, DISABLE);
                    I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                } break;
            }
        }
        else
        {
            if (direction == I2C_DIR_RECEIVER_EMULATOR)
            {
                 i2c_state->rx_buf[i2c_state->rx_data_ctr++] = I2C_ReceiveData(I2C_PERIPH_NAME);

                 switch(i2c_state->rx_buf[CMD_INDEX])
                 {
                    case CMD_LED_MODE:
                    {
                        if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                        {
                            led_driver_set_led_mode(i2c_state->rx_buf[1] & 0x0F, \
                            (i2c_state->rx_buf[1] & 0x10) >> 4);

                            DBG("set LED mode - LED index : ");
                            DBG((const char*)(i2c_state->rx_buf[1] & 0x0F));
                            DBG("\r\nLED mode: ");
                            DBG((const char*)((i2c_state->rx_buf[1] & 0x0F) >> 4));
                            DBG("\r\n");
                        }
                        DBG("ACK\r\n");
                        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                        /* release SCL line */
                        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                    } break;

                    case CMD_LED_STATE:
                    {
                        if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                        {
                            led_driver_set_led_state_user(i2c_state->rx_buf[1] & 0x0F, \
                            (i2c_state->rx_buf[1] & 0x10) >> 4);

                            DBG("set LED state - LED index : ");
                            DBG((const char*)(i2c_state->rx_buf[1] & 0x0F));
                            DBG("\r\nLED state: ");
                            DBG((const char*)((i2c_state->rx_buf[1] & 0x0F) >> 4));
                            DBG("\r\n");
                        }
                        DBG("ACK\r\n");
                        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                        /* release SCL line */
                        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                    } break;

                    case CMD_LED_COLOUR:
                    {
                        if((i2c_state->rx_data_ctr -1) == FOUR_BYTES_EXPECTED)
                        {
                            led_index = i2c_state->rx_buf[1] & 0x0F;
                            /* colour = Red + Green + Blue */
                            colour = (i2c_state->rx_buf[2] << 16) | \
                            (i2c_state->rx_buf[3] << 8) | i2c_state->rx_buf[4];

                            led_driver_set_colour(led_index, colour);

                            DBG("set LED colour - LED index : ")
                            DBG((const char*)&led_index);
                            DBG("\r\nRED: ");
                            DBG((const char*)(i2c_state->rx_buf + 2));
                            DBG("\r\n");
                        }
                        DBG("ACK\r\n");
                        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                        /* release SCL line */
                        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                    } break;

                    case CMD_SET_BRIGHTNESS:
                    {
                        if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                        {
                            led_driver_pwm_set_brightness(i2c_state->rx_buf[1]);

                            DBG("brightness: ");
                            DBG((const char*)(i2c_state->rx_buf + 1));
                            DBG("\r\n");
                        }
                        DBG("ACK\r\n");
                        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                        /* release SCL line */
                        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                    } break;

                    case CMD_GET_BRIGHTNESS:
                    {
                        i2c_state->tx_buf[0] = led->brightness;
                        DBG("brig\r\n");

                        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
                        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                    } break;

                    default: /* command doesnt exist - send NACK */
                    {
                        DBG("EMU_NACK\r\n");
                        I2C_AcknowledgeConfig(I2C_PERIPH_NAME, DISABLE);
                        I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                    } break;
                 }
            }
            else /* I2C_Direction_Transmitter - MCU & EMULATOR */
            {
                DBG("ACKtx\r\n");
                I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
                i2c_state->data_tx_complete = 1;
            }
        }
    }

    /* stop flag */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_STOPF) == SET)
    {
        I2C_ClearITPendingBit(I2C_PERIPH_NAME, I2C_IT_STOPF);

        if (i2c_state->data_tx_complete) /* data have been sent to master */
        {
            i2c_state->data_tx_complete = 0;

            /* delete button status and counter bit from status_word */
            if (i2c_state->rx_buf[CMD_INDEX] == CMD_GET_STATUS_WORD)
            {
                i2c_state->status_word &= ~BUTTON_PRESSED_STSBIT;
                /* decrease button counter by the value has been sent */
                button_counter_decrease((i2c_state->status_word & BUTTON_COUNTER_VALBITS) >> 13);
            }
        }

        DBG("STOP\r\n");

        i2c_state->tx_data_ctr = 0;
        i2c_state->rx_data_ctr = 0;
    }

   __enable_irq();
}
