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
#include "stm32f0xx_conf.h"
#include "debug_serial.h"
#include "led_driver.h"
#include "power_control.h"
#include "delay.h"
#include "eeprom.h"
#include "flash.h"
#include "boot_i2c.h"
#include "bootloader.h"
#include "gpio.h"
#include <string.h>


__attribute__((section(".boot_version"))) uint8_t version[20] = VERSION;

#define I2C_PINS_ALT_FN		1
#define I2C_SCL_PIN		PIN(F, 6)
#define I2C_SDA_PIN		PIN(F, 7)

#define I2C_TIMING                      0x10800000 /* 100kHz for 48MHz system clock */

#define I2C_PERIPH_NAME                 I2C2
#define I2C_PERIPH_CLOCK                RCC_APB1Periph_I2C2

#define I2C_SLAVE_ADDRESS               0x58  /* address in linux: 0x2C */

#define LOW_ADDR_BYTE_IDX               1
#define HIGH_ADDR_BYTE_IDX              0
#define DATA_START_BYTE_IDX             2

#define FILE_CMP_OK                     0xBB
#define FILE_CMP_ERROR                  0xDD
#define ADDR_CMP                        0xFFFF

#define ONE_BYTE_EXPECTED               1

struct st_i2c_status i2c_status;

static uint8_t flash_erase_sts; /* indicates start of flashing */

/*******************************************************************************
  * @function   boot_i2c_config
  * @brief      Configuration of pins for I2C.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void boot_i2c_io_config(void)
{
    /* I2C Peripheral Disable */
    RCC_APB1PeriphClockCmd(I2C_PERIPH_CLOCK, DISABLE);

    /* I2C Periph clock enable */
    RCC_APB1PeriphClockCmd(I2C_PERIPH_CLOCK, ENABLE);

    gpio_init_alts(I2C_PINS_ALT_FN, pin_opendrain, pin_spd_1, pin_nopull,
                   I2C_SCL_PIN, I2C_SDA_PIN);
}

/*******************************************************************************
  * @function   boot_i2c_periph_config
  * @brief      Configuration of I2C peripheral as a slave.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void boot_i2c_periph_config(void)
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
  * @function   boot_i2c_config
  * @brief      Configuration of I2C peripheral and its timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void boot_i2c_config(void)
{
    boot_i2c_io_config();
    boot_i2c_periph_config();
}

/*******************************************************************************
  * @function   boot_i2c_handler
  * @brief      Interrupt handler for I2C communication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void boot_i2c_handler(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    static uint16_t direction;
    static uint32_t flash_address = APPLICATION_ADDRESS;
    static uint8_t data;

    if (!flash_erase_sts) /* we are at the beginning again */
    {
        flash_address = APPLICATION_ADDRESS;
    }

    /* address match interrupt */
    if(I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_ADDR) == SET)
    {
        /* Clear IT pending bit */
        I2C_ClearITPendingBit(I2C_PERIPH_NAME, I2C_IT_ADDR);

        /* Check if transfer direction is read (slave transmitter) */
        if ((I2C_PERIPH_NAME->ISR & I2C_ISR_DIR) == I2C_ISR_DIR)
        {
            direction = I2C_Direction_Transmitter;

            DBG("S.TX\r\n");
        }
        else
        {
            direction = I2C_Direction_Receiver;

            I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
            DBG("S.RX\r\n");
        }
    }
    /* transmit interrupt */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_TXIS) == SET)
    {
        flash_read(&flash_address, &data);
        I2C_SendData(I2C_PERIPH_NAME, data);
        i2c_state->tx_data_ctr++;

        if (i2c_state->tx_data_ctr >= I2C_DATA_PACKET_SIZE)
        {
            i2c_state->tx_data_ctr = 0;
        }
        DBG("send\r\n");
    }
    /* transfer complet interrupt (TX and RX) */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_TCR) == SET)
    {
        if(direction == I2C_Direction_Receiver)
        {
            i2c_state->rx_buf[i2c_state->rx_data_ctr++] = I2C_ReceiveData(I2C_PERIPH_NAME);

            DBG("ACK\r\n");
            I2C_AcknowledgeConfig(I2C_PERIPH_NAME, ENABLE);
            I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);

        }
        else /* I2C_Direction_Transmitter - MCU & EMULATOR */
        {
            DBG("ACKtx\r\n");
            I2C_NumberOfBytesConfig(I2C_PERIPH_NAME, ONE_BYTE_EXPECTED);
        }
    }

    /* stop flag */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_STOPF) == SET)
    {
        I2C_ClearITPendingBit(I2C_PERIPH_NAME, I2C_IT_STOPF);

        if (direction == I2C_Direction_Receiver)
        {
            i2c_state->data_rx_complete = 1;
            I2C_ITConfig(I2C_PERIPH_NAME, I2C_IT_ADDRI | I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_TXI, DISABLE);
        }

        DBG("STOP\r\n");
    }
}

/*******************************************************************************
  * @function   clear_rxbuf
  * @brief      Delete RX buffer.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
static void clear_rxbuf(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    uint16_t idx;

    for (idx = 0; idx < I2C_DATA_PACKET_SIZE; idx++)
    {
        i2c_state->rx_buf[idx + DATA_START_BYTE_IDX] = 0;
    }
}

/*******************************************************************************
  * @function   boot_i2c_flash_data
  * @brief      Flash received data.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
flash_i2c_states_t boot_i2c_flash_data(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    uint16_t rx_cmd, idx, data_length = I2C_DATA_PACKET_SIZE / 4;
    uint8_t data[I2C_DATA_PACKET_SIZE];
    static flash_i2c_states_t flash_status = FLASH_CMD_NOT_RECEIVED;
    static uint32_t flash_address = APPLICATION_ADDRESS;

    if (i2c_state->data_rx_complete)
    {
        memset(data, 0, sizeof(data));

        flash_status = FLASH_CMD_RECEIVED;

        rx_cmd = (i2c_state->rx_buf[HIGH_ADDR_BYTE_IDX] << 8) | \
                            (i2c_state->rx_buf[LOW_ADDR_BYTE_IDX]);

        /* copy data */
        for(idx = 0; idx < I2C_DATA_PACKET_SIZE; idx++)
        {
            data[idx] = i2c_state->rx_buf[idx + DATA_START_BYTE_IDX];
        }

        if (!flash_erase_sts) /* enter the flash sequence, erase pages */
        {
            flash_erase(flash_address);
            flash_erase_sts = 1;
            DBG("FL_NOT_CONF\r\n");
        }

        if (rx_cmd == ADDR_CMP) /* flashing is complete, linux send result of comparison */
        {
            if (data[0] == FILE_CMP_OK)
            {
                flash_status = FLASH_WRITE_OK;
                DBG("WRITE_OK\n\r");
            }
            else
            {
                flash_status = FLASH_WRITE_ERROR;
                DBG("WRITE ERR\n\r");
            }

            flash_address = APPLICATION_ADDRESS;
            flash_erase_sts = 0;
            i2c_state->tx_data_ctr = 0;
        }
        else /* write incoming data */
        {
            flash_write(&flash_address, (uint32_t*)data, data_length);
        }

        clear_rxbuf();

        i2c_state->data_rx_complete = 0;
        i2c_state->rx_data_ctr = 0;

        I2C_ITConfig(I2C_PERIPH_NAME, I2C_IT_ADDRI | I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_TXI, ENABLE);
    }

    return flash_status;
}
