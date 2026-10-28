#ifndef __W25Q128_H
#define __W25Q128_H

#include "stm32f10x.h"

#define W25Q128_CAPACITY_BYTES 0x01000000UL
#define W25Q128_PAGE_SIZE 256U
#define W25Q128_BLOCK_SIZE 65536U
#define W25Q128_SECTOR_SIZE 4096U

void W25Q128_Init(void);
uint32_t W25Q128_ReadJedecId(void);
uint8_t W25Q128_WaitBusy(void);
uint8_t W25Q128_Erase64K(uint32_t address);
uint8_t W25Q128_Erase4K(uint32_t address);
uint8_t W25Q128_Write(uint32_t address, const uint8_t *data, uint32_t length);
uint8_t W25Q128_Read(uint32_t address, uint8_t *data, uint32_t length);

#endif
