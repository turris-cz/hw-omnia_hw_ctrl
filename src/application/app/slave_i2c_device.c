/**
 ******************************************************************************
 * @file    slave_i2c_device.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    26-October-2021
 * @brief   Driver for IC2 communication with master device (main CPU).
 ******************************************************************************
 ******************************************************************************
 **/
/* Includes ------------------------------------------------------------------*/
#include "gd32f1x0_libopt.h"
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

#define I2C_GPIO_CLOCK                  RCU_GPIOF
#define I2C_PERIPH_NAME                 I2C1
#define I2C_PERIPH_CLOCK                RCU_I2C1
#define I2C_DATA_PIN                    GPIO_PIN_7 /* I2C1_SDA - GPIOF */
#define I2C_CLK_PIN                     GPIO_PIN_6 /* I2C1_SCL - GPIOF */
#define I2C_GPIO_PORT                   GPIOF

#define I2C_SLAVE_ADDRESS               0x55  /* address in linux: 0x2A */
#define I2C_SLAVE_ADDRESS_EMULATOR      0x56  /* address in linux: 0x2B */

#define CMD_INDEX                       0
#define NUMBER_OF_BYTES_VERSION         MAX_TX_BUFFER_SIZE
#define BOOTLOADER_VERSION_ADDR         0x08000110

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
};

enum i2c_control_byte_mask {
    LIGHT_RST_MASK                      = 0x01,
    HARD_RST_MASK                       = 0x02,
    SFP_DIS_MASK                        = 0x04,
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
    /* I2C Peripheral Disable */
    rcu_periph_clock_disable(I2C_PERIPH_CLOCK);

    /* I2C Periph clock enable */
    rcu_periph_clock_enable(I2C_PERIPH_CLOCK);

    rcu_periph_clock_enable(I2C_GPIO_CLOCK);

    /* Configure I2C pins: SCL */
    gpio_mode_set(I2C_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C_CLK_PIN);
    gpio_output_options_set(I2C_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, I2C_CLK_PIN);

    gpio_mode_set(I2C_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C_DATA_PIN);
    gpio_output_options_set(I2C_GPIO_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, I2C_DATA_PIN);
}

/*******************************************************************************
  * @function   slave_i2c_periph_config
  * @brief      Configuration of I2C peripheral as a slave.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void slave_i2c_periph_config(void)
{
    i2c_deinit(I2C_PERIPH_NAME);
    i2c_disable(I2C_PERIPH_NAME);

    /* I2C clock configure */
    i2c_clock_config(I2C_PERIPH_NAME, 100000, I2C_DTCY_2);

    /* I2C address configure */
    i2c_mode_addr_config(I2C_PERIPH_NAME, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, I2C_SLAVE_ADDRESS);
    i2c_dualaddr_enable(I2C_PERIPH_NAME, I2C_SLAVE_ADDRESS_EMULATOR);

    i2c_interrupt_enable(I2C1, I2C_INT_EV);

    i2c_stretch_scl_low_config(I2C_PERIPH_NAME, I2C_SCLSTRETCH_ENABLE);

    /* enable I2C */
    i2c_enable(I2C_PERIPH_NAME);
    /* enable acknowledge */
    i2c_ack_config(I2C_PERIPH_NAME, I2C_ACK_ENABLE);

    nvic_irq_enable(I2C1_EV_IRQn, 0, 1);
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
        /* set CFG_CTRL pin to high state ASAP */
        gpio_bit_set(CFG_CTRL_PIN_PORT, CFG_CTRL_PIN);

        /* reset of CPU */
        gpio_bit_reset(MANRES_PIN_PORT, MANRES_PIN);
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
            gpio_bit_reset(ENABLE_4V5_PIN_PORT, ENABLE_4V5_PIN);
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
                case VAR_FLASH_COMPLETE:    DBG_UART("RST: OK\r\n"); break;
                case VAR_PAGE_FULL:         DBG_UART("RST: Pg full\r\n"); break;
                case VAR_NO_VALID_PAGE:     DBG_UART("RST: No Pg\r\n"); break;
                default:
                    break;
            }

            i2c_control->state = SLAVE_I2C_GO_TO_BOOTLOADER;
        }
    }
}

static void slave_i2c_handler_set_stopped(void)
{
    i2c_interrupt_disable(I2C_PERIPH_NAME, I2C_INT_ERR);
    i2c_interrupt_disable(I2C_PERIPH_NAME, I2C_INT_BUF);
    i2c_status.tx_data_ctr = 0;
    i2c_status.tx_data_len = 0;
    i2c_status.handler_state = STOPPED;
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
    struct led_rgb *led = leds;
    uint16_t stat0;
    uint16_t idx;
    uint8_t led_index;
    uint32_t colour;
    eeprom_var_t ee_var;
    uint8_t address;

    __disable_irq();

    stat0 = I2C_STAT0(I2C_PERIPH_NAME);

    /********* error handling *********/

    /* over-run / under-run interrupt */
    if (stat0 & I2C_STAT0_OUERR) {
        DBG_UART("OUERR\r\n");

        /* clear */
        I2C_STAT0(I2C_PERIPH_NAME) &= ~I2C_STAT0_OUERR;
    }

    /* bus error interrupt */
    if (stat0 & I2C_STAT0_BERR) {
        DBG_UART("BERR\r\n");

        /* clear & stop */
        I2C_STAT0(I2C_PERIPH_NAME) &= ~I2C_STAT0_BERR;
        slave_i2c_handler_set_stopped();
    }



    /********* communication handling *********/

    /* acknowledge not received interrupt */
    if (stat0 & I2C_STAT0_AERR) {
        DBG_UART("AERR\r\n");

        /* clear AERR */
        I2C_STAT0(I2C_PERIPH_NAME) &= ~I2C_STAT0_AERR;

        slave_i2c_handler_set_stopped();
    }

    /* stop flag */
    else if (i2c_state->handler_state != STOPPED && (stat0 & I2C_STAT0_STPDET))
    {
        /* this also clears STPDET bit */
        i2c_enable(I2C_PERIPH_NAME);

        DBG_UART("STOP\r\n");
        slave_i2c_handler_set_stopped();
    }

    /* data not empty during receiving interrupt */
    else if (i2c_state->handler_state == RECEIVING && (stat0 & I2C_STAT0_RBNE))
    {
        uint8_t byte = i2c_data_receive(I2C_PERIPH_NAME);

        /* if more bytes than MAX_RX_BUFFER_SIZE received -> fail */
        if (i2c_state->rx_data_ctr >= MAX_RX_BUFFER_SIZE)
        {
            DBG_UART("RX OVERFLOW\r\n");
            goto end;
        }

        i2c_state->rx_buf[i2c_state->rx_data_ctr++] = byte;

        /* check if the command (register) exists and send ACK */
        switch(i2c_state->rx_buf[CMD_INDEX])
        {
            case CMD_GET_STATUS_WORD:
            {
                /* prepare data to be sent to the master */
                i2c_state->tx_buf[0] = i2c_state->status_word & 0x00FF;
                i2c_state->tx_buf[1] = (i2c_state->status_word & 0xFF00) >> 8;
                i2c_state->tx_data_len = 2;
                DBG_UART("STS\r\n");

                i2c_state->rx_data_ctr = 0;

                /* delete button status and counter bit from status_word */
                if (i2c_state->rx_buf[CMD_INDEX] == CMD_GET_STATUS_WORD)
                {
                    i2c_state->status_word &= ~BUTTON_PRESSED_STSBIT;
                    /* decrease button counter by the value has been sent */
                    button_counter_decrease((i2c_state->status_word & BUTTON_COUNTER_VALBITS) >> 13);
                }
            } break;

            case CMD_GENERAL_CONTROL:
            {
                if((i2c_state->rx_data_ctr -1) == TWO_BYTES_EXPECTED)
                {
                    slave_i2c_check_control_byte(i2c_state->rx_buf[2], \
                        i2c_state->rx_buf[1]);

                    i2c_state->rx_data_ctr = 0;
                }
            } break;

            case CMD_LED_MODE:
            {
                if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                {
                    led_driver_set_led_mode(i2c_state->rx_buf[1] & 0x0F, \
                         (i2c_state->rx_buf[1] & 0x10) >> 4);
                    DBG_UART("set LED mode - LED index : ");
                    DBG_UART((const char*)(i2c_state->rx_buf[1] & 0x0F));
                    DBG_UART("\r\nLED mode: ");
                    DBG_UART((const char*)((i2c_state->rx_buf[1] & 0x0F) >> 4));
                    DBG_UART("\r\n");

                    i2c_state->rx_data_ctr = 0;
                }
            } break;

            case CMD_LED_STATE:
            {
                if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                {
                    led_driver_set_led_state_user(i2c_state->rx_buf[1] &
                        0x0F, (i2c_state->rx_buf[1] & 0x10) >> 4);

                    DBG_UART("set LED state - LED index : ");
                    DBG_UART((const char*)(i2c_state->rx_buf[1] & 0x0F));
                    DBG_UART("\r\nLED state: ");
                    DBG_UART((const char*)((i2c_state->rx_buf[1] & 0x0F) >> 4));
                    DBG_UART("\r\n");

                    i2c_state->rx_data_ctr = 0;
                }
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

                    DBG_UART("set LED colour - LED index : ");
                    DBG_UART((const char*)&led_index);
                    DBG_UART("\r\nRED: ");
                    DBG_UART((const char*)(i2c_state->rx_buf + 2));
                    DBG_UART("\r\n");

                    i2c_state->rx_data_ctr = 0;
                }
             } break;

#if USER_REGULATOR_ENABLED
            case CMD_USER_VOLTAGE:
            {
                if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                {
                    power_control_set_voltage(i2c_state->rx_buf[1]);

                    DBG_UART("user voltage: ");
                    DBG_UART((const char*)(i2c_state->rx_buf + 1));
                    DBG_UART("\r\n");

                    i2c_state->rx_data_ctr = 0;
                }
            } break;
#endif

            case CMD_SET_BRIGHTNESS:
            {
                if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                {
                    led_driver_pwm_set_brightness(i2c_state->rx_buf[1]);

                    DBG_UART("brightness: ");
                    DBG_UART((const char*)(i2c_state->rx_buf + 1));
                    DBG_UART("\r\n");

                    i2c_state->rx_data_ctr = 0;
                }
            } break;

            case CMD_GET_BRIGHTNESS:
            {
                i2c_state->tx_buf[0] = led->brightness;
                i2c_state->tx_data_len = 1;
                DBG_UART("brig\r\n");

                i2c_state->rx_data_ctr = 0;
            } break;

            case CMD_GET_RESET:
            {
                i2c_state->tx_buf[0] = i2c_state->reset_type;
                i2c_state->tx_data_len = 1;
                DBG_UART("RST\r\n");
                i2c_state->rx_data_ctr = 0;
            } break;

            case CMD_WATCHDOG_STATE:
            {
                if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                {
                    wdg->watchdog_state = i2c_state->rx_buf[1];
                    DBG_UART("WDT STATE: ");
                    DBG_UART((const char*)(i2c_state->rx_buf + 1));
                    DBG_UART("\r\n");
                    i2c_state->rx_data_ctr = 0;
                }
            } break;

            case CMD_WATCHDOG_STATUS:
            {
                if((i2c_state->rx_data_ctr -1) == ONE_BYTE_EXPECTED)
                {
                    wdg->watchdog_sts = i2c_state->rx_buf[1];

                    ee_var = EE_WriteVariable(WDG_VIRT_ADDR, wdg->watchdog_sts);

                    switch(ee_var)
                    {
                        case VAR_FLASH_COMPLETE: DBG_UART("WDT: OK\r\n"); break;
                        case VAR_PAGE_FULL: DBG_UART("WDT: Pg full\r\n"); break;
                        case VAR_NO_VALID_PAGE: DBG_UART("WDT: No Pg\r\n"); break;
                        default:
                             break;
                    }

                    DBG_UART("WDT: ");
                    DBG_UART((const char*)(i2c_state->rx_buf + 1));
                    DBG_UART("\r\n");

                    i2c_state->rx_data_ctr = 0;
                }
            } break;

            case CMD_GET_WATCHDOG_STATE:
            {
                i2c_state->tx_buf[0] = wdg->watchdog_state;
                i2c_state->tx_data_len = 1;
                DBG_UART("WDT GET\r\n");

                i2c_state->rx_data_ctr = 0;
            } break;

            /* read fw version of the current application */
            case CMD_GET_FW_VERSION_APP:
            {
                i2c_state->tx_data_len = MAX_TX_BUFFER_SIZE;
                for (idx = 0; idx < MAX_TX_BUFFER_SIZE; idx++)
                {
                    i2c_state->tx_buf[idx] = version[idx];
                }
                DBG_UART("FWA\r\n");

                i2c_state->rx_data_ctr = 0;
            } break;

            /* read fw version of the bootloader */
            case CMD_GET_FW_VERSION_BOOT:
            {
                read_bootloader_version(i2c_state->tx_buf);
                i2c_state->tx_data_len = MAX_TX_BUFFER_SIZE;

                DBG_UART("FWB\r\n");

                i2c_state->rx_data_ctr = 0;
            } break;

            default:
            {
                DBG_UART("DEF\r\n");
                i2c_state->rx_data_ctr = 0;
            } break;
        }
    }

    /* data empty during transmitting interrupt */
    else if (i2c_state->handler_state == TRANSMITTING && (stat0 & I2C_STAT0_TBE))
    {
        if (i2c_state->tx_data_len > 0)
        {
            i2c_data_transmit(I2C_PERIPH_NAME, i2c_state->tx_buf[i2c_state->tx_data_ctr++]);
            i2c_state->tx_data_len--;
            DBG_UART("send\r\n");
        }

        if (i2c_state->tx_data_len == 0)
        {
            i2c_state->tx_data_ctr = 0;
            i2c_state->handler_state = TRANSMITTED;

            /* all bytes transmitted, disable TBE interrupt */
            DBG_UART("no more bytes to send, disabling TBE\r\n");
            i2c_interrupt_disable(I2C_PERIPH_NAME, I2C_INT_BUF);
        }
    }

    /* byte transmission completed interrupt */
    else if (i2c_state->handler_state == TRANSMITTED && (stat0 & I2C_STAT0_BTC))
    {
        /*
         * There is no sensible way on GD32 to make the master time out if
         * we have no more bytes to send, thus we send 0xff if master
         * does a wrong request.
         */
        i2c_data_transmit(I2C_PERIPH_NAME, 0xff);
        DBG_UART("no bytes to send, sending 0xff (BTC)\r\n");
    }

    /* address match interrupt */
    else if (stat0 & I2C_STAT0_ADDSEND)
    {
        uint16_t stat1;

        /* reading stat1 after stat0 clears the ADDSEND bit */
        stat1 = I2C_STAT1(I2C_PERIPH_NAME);

        if (stat1 & I2C_STAT1_TR) {
            DBG_UART("ADDR tx\r\n");
            i2c_state->handler_state = TRANSMITTING;

            /*
             * enable ERR interrupt to be able to determine when master stops
             * a receiving transaction
             */
            i2c_interrupt_enable(I2C_PERIPH_NAME, I2C_INT_ERR);
        } else {
            DBG_UART("ADDR rx\r\n");
            i2c_state->handler_state = RECEIVING;
        }

        /* enable RBNE/TBE interrupt */
        i2c_interrupt_enable(I2C_PERIPH_NAME, I2C_INT_BUF);

        i2c_state->rx_data_ctr = 0;
    }

end:
    __enable_irq();
}
