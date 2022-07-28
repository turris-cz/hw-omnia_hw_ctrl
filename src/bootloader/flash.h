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

#define USER_FLASH_LAST_PAGE_ADDRESS  0x0800EC00
#define USER_FLASH_END_ADDRESS        0x0800EFFF /* 64-4-20 = 40 KBytes */
#define FLASH_PAGE_SIZE               0x400      /* 1 Kbytes */

/* define the address from where user application will be loaded,
   the application address should be a start sector address */
#define APPLICATION_ADDRESS     (uint32_t)0x08005000
#define APPLICATION_END         (uint32_t)(USER_FLASH_LAST_PAGE_ADDRESS + FLASH_PAGE_SIZE)
#define APPLICATION_MAX_SIZE    (APPLICATION_END - APPLICATION_ADDRESS)

/* Get the number of Sector from where the user program will be loaded */
#define  FLASH_PAGE_NUMBER      (uint32_t)((APPLICATION_ADDRESS - 0x08000000) >> 12)

/* Compute the mask to test if the Flash memory, where the user program will be
   loaded, is write protected */
#define  FLASH_PROTECTED_PAGES   ((uint32_t)~((1 << FLASH_PAGE_NUMBER) - 1))

/* define the user application size */
#define USER_FLASH_SIZE   (USER_FLASH_END_ADDRESS - APPLICATION_ADDRESS + 1)

/*******************************************************************************
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  *****************************************************************************/
static __force_inline void flash_config(void)
{
	/* Unlock the Program memory */
	FLASH_Unlock();

	/* Clear all FLASH flags */
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR |
			FLASH_FLAG_BSY);
}

/*******************************************************************************
  * @brief  This function does an erase of all user flash area
  * @param  start_sector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         -1: error occurred
  *****************************************************************************/
static __force_inline int flash_erase(uint32_t start_page)
{
	for (uint32_t page = start_page;
	     page <= USER_FLASH_LAST_PAGE_ADDRESS;
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
