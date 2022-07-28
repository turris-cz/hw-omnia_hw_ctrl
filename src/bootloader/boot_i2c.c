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
#include "debug.h"
#include "led_driver.h"
#include "power_control.h"
#include "delay.h"
#include "eeprom.h"
#include "flash.h"
#include "i2c_slave.h"
#include "boot_i2c.h"
#include "bootloader.h"
#include "gpio.h"
#include <string.h>

__attribute__((section(".boot_version"))) uint8_t version[20] = VERSION;

#define PKT_SIZE		128

#define I2C_SLAVE_ADDRESS	0x2c

#define FILE_CMP_OK		0xBB
#define FILE_CMP_ERROR		0xDD
#define ADDR_CMP		0xFFFF

typedef struct {
	uint8_t cmd[2 + PKT_SIZE];
	uint8_t cmd_len;
	uint8_t tx_idx;
	uint16_t tx_addr;
	bool flash_erased;
	bool rx_complete;
	bool receiving;
	bool cmd_valid;
} boot_i2c_state_t;

static uint16_t get_addr(const boot_i2c_state_t *state)
{
	return (state->cmd[0] << 8) | state->cmd[1];
}

static bool has_valid_addr(const boot_i2c_state_t *state)
{
	uint16_t addr = get_addr(state);

	return (addr & (PKT_SIZE - 1)) == 0 &&
	       ((uint32_t)addr + PKT_SIZE) <= APPLICATION_MAX_SIZE;
}

static int boot_i2c_event_cb(void *priv, uint8_t addr,
			     i2c_slave_event_t event, uint8_t *val)
{
	boot_i2c_state_t *state = priv;

	if (addr != I2C_SLAVE_ADDRESS)
		return -1;

	switch (event) {
	case I2C_SLAVE_READ_REQUESTED:
		state->receiving = 0;
		state->tx_idx = 0;
		if (state->cmd_len == 2 && has_valid_addr(state)) {
			state->tx_addr = get_addr(state);
			state->cmd_len = 0;
		}
		fallthrough;

	case I2C_SLAVE_READ_PROCESSED:
		if (state->tx_idx >= PKT_SIZE ||
		    state->tx_addr >= APPLICATION_MAX_SIZE)
			return -1;

		*val = *(uint8_t *)(APPLICATION_ADDRESS + state->tx_addr);
		state->tx_addr++;
		state->tx_idx++;

		break;

	case I2C_SLAVE_WRITE_REQUESTED:
		state->cmd_len = 0;
		state->cmd_valid = 0;
		state->receiving = 1;
		break;

	case I2C_SLAVE_WRITE_RECEIVED:
		state->cmd_valid = 1;

		if (state->cmd_len < sizeof(state->cmd))
			state->cmd[state->cmd_len++] = *val;
		else
			state->cmd_valid = 0;

		/* check address validity */
		if (state->cmd_len == 2 && get_addr(state) != ADDR_CMP &&
		    !has_valid_addr(state))
			state->cmd_valid = 0;

		/* ADDR_CMP command may contain only one data byte */
		if (state->cmd_len > 3 && get_addr(state) == ADDR_CMP)
			state->cmd_valid = 0;

		if (!state->cmd_valid)
			return -1;

		break;

	case I2C_SLAVE_STOP:
		/* trigger flashing/comparing in boot_i2c_flash_data() */
		if (state->receiving && state->cmd_valid &&
		    state->cmd_len > 2) {
			state->rx_complete = 1;
			i2c_slave_pause(SLAVE_I2C);
		} else if (!state->receiving) {
			state->cmd_len = 0;
			state->cmd_valid = 0;
		}
		break;
	}

	return 0;
}

static boot_i2c_state_t boot_i2c_state;

static i2c_slave_t i2c_slave = {
	.cb = boot_i2c_event_cb,
	.priv = &boot_i2c_state,
};

/*******************************************************************************
  * @function   boot_i2c_config
  * @brief      Configuration of I2C peripheral and its timeout.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void boot_i2c_config(void)
{
	i2c_slave_init(SLAVE_I2C, &i2c_slave, I2C_SLAVE_ADDRESS, 0, 1);
}

/*******************************************************************************
  * @function   boot_i2c_flash_data
  * @brief      Flash received data.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
flash_i2c_state_t boot_i2c_flash_data(void)
{
	boot_i2c_state_t *state = &boot_i2c_state;
	flash_i2c_state_t ret;

	if (!state->rx_complete)
		return FLASH_CMD_NOT_RECEIVED;

	if (!state->flash_erased) {
		flash_erase(APPLICATION_ADDRESS);
		state->flash_erased = 1;
	}

	if (get_addr(state) == ADDR_CMP) {
		if (state->cmd[2] == FILE_CMP_OK)
			ret = FLASH_WRITE_OK;
		else
			ret = FLASH_WRITE_ERROR;

		state->flash_erased = 0;
		state->tx_addr = 0;
	} else {
		flash_write(APPLICATION_ADDRESS + get_addr(state),
			    &state->cmd[2], PKT_SIZE);
		ret = FLASH_CMD_RECEIVED;
	}

	state->cmd_len = 0;
	state->cmd_valid = 0;
	state->rx_complete = 0;

	i2c_slave_resume(SLAVE_I2C);

	return ret;
}
