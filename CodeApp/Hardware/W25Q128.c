#include "W25Q128.h"

#define W25Q128_CS_PIN GPIO_Pin_4
#define W25Q128_WAIT_LIMIT 0x00FFFFFFUL

static void W25Q128_Select(uint8_t selected)
{
    GPIO_WriteBit(GPIOA, W25Q128_CS_PIN, selected ? Bit_RESET : Bit_SET);
}

static uint8_t W25Q128_Transfer(uint8_t value)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET) {}
    SPI_I2S_SendData(SPI1, value);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {}
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

static void W25Q128_SendAddress(uint32_t address)
{
    W25Q128_Transfer((uint8_t)(address >> 16));
    W25Q128_Transfer((uint8_t)(address >> 8));
    W25Q128_Transfer((uint8_t)address);
}

static uint8_t W25Q128_WriteEnable(void)
{
    if (W25Q128_WaitBusy() != 0) return 1;
    W25Q128_Select(1);
    W25Q128_Transfer(0x06);
    W25Q128_Select(0);
    return 0;
}

void W25Q128_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);
    gpio.GPIO_Pin = W25Q128_CS_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    W25Q128_Select(0);

    gpio.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);

    SPI_I2S_DeInit(SPI1);
    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);
}

uint32_t W25Q128_ReadJedecId(void)
{
    uint32_t id;
    W25Q128_Select(1);
    W25Q128_Transfer(0x9F);
    id = (uint32_t)W25Q128_Transfer(0xFF) << 16;
    id |= (uint32_t)W25Q128_Transfer(0xFF) << 8;
    id |= W25Q128_Transfer(0xFF);
    W25Q128_Select(0);
    return id;
}

uint8_t W25Q128_WaitBusy(void)
{
    uint32_t timeout = W25Q128_WAIT_LIMIT;
    do {
        uint8_t status;
        W25Q128_Select(1);
        W25Q128_Transfer(0x05);
        status = W25Q128_Transfer(0xFF);
        W25Q128_Select(0);
        if ((status & 0x01U) == 0) return 0;
    } while (--timeout != 0);
    return 1;
}

uint8_t W25Q128_Erase64K(uint32_t address)
{
    if (address >= W25Q128_CAPACITY_BYTES ||
        (address % W25Q128_BLOCK_SIZE) != 0 || W25Q128_WriteEnable() != 0)
        return 1;
    W25Q128_Select(1);
    W25Q128_Transfer(0xD8);
    W25Q128_SendAddress(address);
    W25Q128_Select(0);
    return W25Q128_WaitBusy();
}

uint8_t W25Q128_Erase4K(uint32_t address)
{
    if (address >= W25Q128_CAPACITY_BYTES ||
        (address % W25Q128_SECTOR_SIZE) != 0 || W25Q128_WriteEnable() != 0)
        return 1;
    W25Q128_Select(1);
    W25Q128_Transfer(0x20);
    W25Q128_SendAddress(address);
    W25Q128_Select(0);
    return W25Q128_WaitBusy();
}

uint8_t W25Q128_Write(uint32_t address, const uint8_t *data, uint32_t length)
{
    if ((data == 0 && length != 0) || address > W25Q128_CAPACITY_BYTES ||
        length > W25Q128_CAPACITY_BYTES - address) return 1;

    while (length != 0) {
        uint16_t offset = (uint16_t)(address % W25Q128_PAGE_SIZE);
        uint16_t chunk = (uint16_t)(W25Q128_PAGE_SIZE - offset);
        uint16_t i;
        if (chunk > length) chunk = (uint16_t)length;
        if (W25Q128_WriteEnable() != 0) return 2;
        W25Q128_Select(1);
        W25Q128_Transfer(0x02);
        W25Q128_SendAddress(address);
        for (i = 0; i < chunk; ++i) W25Q128_Transfer(data[i]);
        W25Q128_Select(0);
        if (W25Q128_WaitBusy() != 0) return 3;
        address += chunk;
        data += chunk;
        length -= chunk;
    }
    return 0;
}

uint8_t W25Q128_Read(uint32_t address, uint8_t *data, uint32_t length)
{
    uint32_t i;
    if ((data == 0 && length != 0) || address > W25Q128_CAPACITY_BYTES ||
        length > W25Q128_CAPACITY_BYTES - address || W25Q128_WaitBusy() != 0)
        return 1;
    W25Q128_Select(1);
    W25Q128_Transfer(0x03);
    W25Q128_SendAddress(address);
    for (i = 0; i < length; ++i) data[i] = W25Q128_Transfer(0xFF);
    W25Q128_Select(0);
    return 0;
}
