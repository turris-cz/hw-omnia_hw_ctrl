/**
 ******************************************************************************
 * @file    boot_i2c.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    15-April-2016
 * @brief   Header for I2C driver.
 ******************************************************************************
 ******************************************************************************
 **/

#include "stm32f0xx.h"

#ifndef BOOT_I2C_H
#define BOOT_I2C_H

typedef enum {
	FLASH_CMD_RECEIVED,
	FLASH_CMD_NOT_RECEIVED,
	FLASH_WRITE_OK,
	FLASH_WRITE_ERROR
} flash_i2c_state_t;

/*******************************************************************************
  * @function   boot_i2c_config
  * @brief      Configuration of I2C peripheral as a slave.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
void boot_i2c_config(void);

/*******************************************************************************
  * @function   boot_i2c_flash_data
  * @brief      Flash received data.
  * @param      None.
  * @retval     None.
  *****************************************************************************/
flash_i2c_state_t boot_i2c_flash_data(void);

#endif /* BOOT_I2C_H */

