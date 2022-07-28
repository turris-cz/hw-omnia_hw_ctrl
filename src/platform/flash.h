/**
 ******************************************************************************
 * @file    flash.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    13-April-2016
 * @brief   Header for driver for writing onto flash memory (used by IAP).
 ******************************************************************************/

#ifndef __FLASH_H
#define __FLASH_H

#include "stm32f0xx.h"
#include "stm32f0xx_flash.h"
#include "compiler.h"

#define FLASH_PAGE_SIZE		0x400

/*******************************************************************************
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  *****************************************************************************/
static __force_inline void flash_init(void)
{
	/* Unlock the Program memory */
	FLASH_Unlock();

	/* Clear all FLASH flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR |
			FLASH_FLAG_BSY);
}

/*******************************************************************************
  * @brief  This function does an erase of all user flash area
  * @param  start_page: start of user flash area
  * @param  len: length in bytes
  * @retval 0: user flash area successfully erased
  *         -1: error occurred
  *****************************************************************************/
static __force_inline int flash_erase(uint32_t start_page, uint16_t len)
{
	for (uint32_t page = start_page;
	     page < start_page + len;
	     page += FLASH_PAGE_SIZE)
		if (FLASH_ErasePage(page) != FLASH_COMPLETE)
			return -1;

	return 0;
}

/*******************************************************************************
  * @brief  This function writes a data buffer in flash.
  * @note   After writing data buffer, the flash content is checked.
  * @param  address: start address for writing data buffer, 4-aligned
  * @param  data: pointer on data buffer
  * @param  length: length of data buffer (unit is 8-bit word), 4-aligned
  * @retval 0: Data successfully written to Flash memory
  *         -1: Error occurred while writing data in Flash memory
  *         -2: Written Data in flash memory is different from expected one
  *****************************************************************************/
static __force_inline int flash_write(uint32_t address, uint8_t *data, uint16_t length)
{
	uint32_t end = address + length;

	for (; address < end; address += 4, data += 4) {
		uint32_t word = (data[3] << 24) | (data[2] << 16) |
				(data[1] << 8) | data[0];

		if (FLASH_ProgramWord(address, word) != FLASH_COMPLETE)
			return -1; /* error writing */

		if (*(volatile uint32_t *)address != word)
			return -2; /* error comparing */
	}

	return 0;
}

#endif /* __FLASH_H */
