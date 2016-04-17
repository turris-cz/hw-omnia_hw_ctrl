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
#include "wan_lan_pci_status.h"
#include "power_control.h"
#include "delay.h"
#include "debounce.h"
#include "eeprom.h"
#include "msata_pci.h"
#include "pca9538_emu.h"

#include "flash.h"
#include "boot_i2c.h"
#include <string.h>

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

#define I2C_SLAVE_ADDRESS               0x58  /* address in linux: 0x2C */

#define LOW_ADDR_BYTE_IDX             1
#define HIGH_ADDR_BYTE_IDX            0
#define DATA_START_BYTE_IDX           2

enum i2c_commands {
    CMD_UPGRADE_FW                      = 0x24
};


enum expected_bytes_in_cmd {
    ONE_BYTE_EXPECTED                   = 1,
    TWO_BYTES_EXPECTED                  = 2,
    FOUR_BYTES_EXPECTED                 = 4,
    TWENTY_BYTES_EXPECTED               = 20
};

struct st_i2c_status i2c_status;

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
  * @function   slave_i2c_handler
  * @brief      Interrupt handler for I2C communication.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void slave_i2c_handler(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    static uint16_t direction;
    static uint32_t flash_address = APPLICATION_ADDRESS;
    static uint8_t data;


   // __disable_irq();

    /* address match interrupt */
    if(I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_ADDR) == SET)
    {
        /* Clear IT pending bit */
        I2C_ClearITPendingBit(I2C_PERIPH_NAME, I2C_IT_ADDR);

        /* Check if transfer direction is read (slave transmitter) */
        if ((I2C_PERIPH_NAME->ISR & I2C_ISR_DIR) == I2C_ISR_DIR)
        {
            direction = I2C_Direction_Transmitter;

            flash_read(&flash_address, &data);

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
        I2C_SendData(I2C_PERIPH_NAME, data);
        i2c_state->tx_data_ctr++;

        if (i2c_state->tx_data_ctr < I2C_DATA_PACKET_SIZE)
        {
            flash_read(&flash_address, &data);
        }
        else
        {
            i2c_state->tx_data_ctr = 0; //TODO: jeste vynulovat po skonceni flashovani - jak to poznat?
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
            i2c_state->data_tx_complete = 1;
        }
    }

    /* stop flag */
    else if (I2C_GetITStatus(I2C_PERIPH_NAME, I2C_IT_STOPF) == SET)
    {
        I2C_ClearITPendingBit(I2C_PERIPH_NAME, I2C_IT_STOPF);

        i2c_state->data_rx_complete = 1;

//        if (i2c_state->data_tx_complete) /* data have been sent to master */
//        {
//            i2c_state->data_tx_complete = 0;
//            i2c_state->tx_data_ctr = 0;
//        }

        if (i2c_state->data_rx_complete)
        {
            I2C_ITConfig(I2C_PERIPH_NAME, I2C_IT_ADDRI | I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_TXI, DISABLE);
        }
        DBG("STOP\r\n");
    }

  // __enable_irq();
}

static void clear_rxbuf(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    uint16_t idx;

    for (idx = 0; idx < I2C_DATA_PACKET_SIZE; idx++)
    {
        i2c_state->rx_buf[idx + DATA_START_BYTE_IDX] = 0;
    }
}

uint32_t slave_i2c_process_data(void)
{
    struct st_i2c_status *i2c_state = &i2c_status;
    uint16_t address, idx, data_length = I2C_DATA_PACKET_SIZE / 4;
    uint8_t data[I2C_DATA_PACKET_SIZE];
    uint32_t flash_status = 0;
    static uint32_t flash_address = APPLICATION_ADDRESS;
    static uint16_t flash_erase_sts;

    //TODO: nastavit spravnou adresu flashovani po pozadavku

    if (i2c_state->data_rx_complete)
    {
        memset(data, 0, sizeof(data)); //TODO - check sizeof

        address = (i2c_state->rx_buf[HIGH_ADDR_BYTE_IDX] << 8) | \
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
        }

        //flash_status = flash_new_data((uint32_t*)data, data_length);
        flash_status = flash_write(&flash_address, (uint32_t*)data, data_length);

        clear_rxbuf();

        i2c_state->data_rx_complete = 0;
        i2c_state->rx_data_ctr = 0;

        I2C_ITConfig(I2C_PERIPH_NAME, I2C_IT_ADDRI | I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_TXI, ENABLE);


    }

    return flash_status;
}