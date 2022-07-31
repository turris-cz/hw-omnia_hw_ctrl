/**
 ******************************************************************************
 * @file    bootloader.h
 * @author  CZ.NIC, z.s.p.o.
 * @date    17-April-2016
 * @brief   Header for bootloader.
 ******************************************************************************
 ******************************************************************************
 **/

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

enum eeprom_flags {
    BOOTLOADER_REQ                      = 0xAA,
    FLASH_NOT_CONFIRMED                 = 0x55,
    FLASH_CONFIRMED                     = 0x88
};

#endif /* BOOTLOADER_H */
