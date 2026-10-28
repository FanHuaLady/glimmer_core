#ifndef __AT24C02_H
#define __AT24C02_H

#include "stm32f10x.h"

#define AT24C02_WADDR 0xA0U
#define AT24C02_RADDR 0xA1U
#define AT24C02_CAPACITY_BYTES 256U
#define AT24C02_PAGE_SIZE 16U

void AT24C02_Init(void);
uint8_t AT24C02_WriteByte(uint8_t address, uint8_t data);
uint8_t AT24C02_WritePage(uint8_t address, const uint8_t *data);
uint8_t AT24C02_Read(uint8_t address, uint8_t *data, uint16_t length);

#endif
