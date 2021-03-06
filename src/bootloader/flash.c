/**
 ******************************************************************************
 * @file    flash.c
 * @author  CZ.NIC, z.s.p.o.
 * @date    13-April-2016
 * @brief   Driver for writing onto flash memory (used by IAP).
 ******************************************************************************
 ******************************************************************************
 * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
 */
#include "stm32f0xx_conf.h"
#include "flash.h"

/*******************************************************************************
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  *****************************************************************************/
void flash_config(void)
{
  /* Unlock the Program memory */
  FLASH_Unlock();

  /* Clear all FLASH flags */
  FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_WRPERR | FLASH_FLAG_PGERR | FLASH_FLAG_BSY);
}

/*******************************************************************************
  * @brief  This function does an erase of all user flash area
  * @param  start_sector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         1: error occurred
  *****************************************************************************/
uint32_t flash_erase(uint32_t start_sector)
{
  uint32_t flashaddress;

  flashaddress = start_sector;

  while (flashaddress <= (uint32_t) USER_FLASH_LAST_PAGE_ADDRESS)
  {
    if (FLASH_ErasePage(flashaddress) == FLASH_COMPLETE)
    {
      flashaddress += FLASH_PAGE_SIZE;
    }
    else
    {
      /* Error occurred while page erase */
      return (1);
    }
  }
  return (0);
}

/*******************************************************************************
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  FlashAddress: start address for writing data buffer
  * @param  Data: pointer on data buffer
  * @param  DataLength: length of data buffer (unit is 32-bit word)
  * @retval 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  *****************************************************************************/
uint32_t flash_write(volatile uint32_t* flash_address, uint32_t* data, uint16_t data_length)
{
  uint32_t i = 0;

  for (i = 0; (i < data_length) && (*flash_address <= (USER_FLASH_END_ADDRESS-4)); i++)
  {
    /* the operation will be done by word */
    if (FLASH_ProgramWord(*flash_address, *(uint32_t*)(data+i)) == FLASH_COMPLETE)
    {
     /* Check the written value */
      if (*(uint32_t*)*flash_address != *(uint32_t*)(data+i))
      {
        /* Flash content doesn't match SRAM content */
        return(2);
      }
      /* Increment FLASH destination address */
      *flash_address += 4;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      return (1);
    }
  }

  return (0);
}

/*******************************************************************************
  * @brief  This function reads data from flash, byte after byte
  * @param  flash_address: start of selected flash area to be read
  * @param  data: data from flash
  * @retval None.
  *****************************************************************************/
void flash_read(volatile uint32_t *flash_address, uint8_t *data)
{
   *data = *(uint8_t*)*flash_address;
    (*flash_address)++;
}
