#include "AT24C02.h"
#include "Delay.h"

#define AT24C02_SCL_PORT GPIOB
#define AT24C02_SCL_PIN GPIO_Pin_10
#define AT24C02_SDA_PORT GPIOB
#define AT24C02_SDA_PIN GPIO_Pin_11

static void AT24C02_WriteScl(uint8_t value)
{
    GPIO_WriteBit(AT24C02_SCL_PORT, AT24C02_SCL_PIN,
                  value ? Bit_SET : Bit_RESET);
}

static void AT24C02_WriteSda(uint8_t value)
{
    GPIO_WriteBit(AT24C02_SDA_PORT, AT24C02_SDA_PIN,
                  value ? Bit_SET : Bit_RESET);
}

static uint8_t AT24C02_ReadSda(void)
{
    return GPIO_ReadInputDataBit(AT24C02_SDA_PORT, AT24C02_SDA_PIN);
}

static void AT24C02_Start(void)
{
    AT24C02_WriteSda(1);
    AT24C02_WriteScl(1);
    AT24C02_WriteSda(0);
    AT24C02_WriteScl(0);
}

static void AT24C02_Stop(void)
{
    AT24C02_WriteSda(0);
    AT24C02_WriteScl(1);
    AT24C02_WriteSda(1);
}

static void AT24C02_SendByte(uint8_t value)
{
    uint8_t bit;
    for (bit = 0; bit < 8; ++bit) {
        AT24C02_WriteSda((value & 0x80U) != 0);
        AT24C02_WriteScl(1);
        AT24C02_WriteScl(0);
        value <<= 1;
    }
    AT24C02_WriteSda(1);
}

static uint8_t AT24C02_WaitAck(void)
{
    uint16_t timeout = 1000;
    AT24C02_WriteSda(1);
    while (AT24C02_ReadSda() && timeout != 0) {
        --timeout;
        Delay_us(1);
    }
    if (timeout == 0) return 1;
    AT24C02_WriteScl(1);
    AT24C02_WriteScl(0);
    return 0;
}

static uint8_t AT24C02_ReadByte(uint8_t acknowledge)
{
    uint8_t bit;
    uint8_t value = 0;
    AT24C02_WriteSda(1);
    for (bit = 0; bit < 8; ++bit) {
        value <<= 1;
        AT24C02_WriteScl(1);
        if (AT24C02_ReadSda()) value |= 1;
        AT24C02_WriteScl(0);
    }
    AT24C02_WriteSda(acknowledge ? 0 : 1);
    AT24C02_WriteScl(1);
    AT24C02_WriteScl(0);
    return value;
}

void AT24C02_Init(void)
{
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    gpio.GPIO_Pin = AT24C02_SCL_PIN | AT24C02_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    AT24C02_WriteScl(1);
    AT24C02_WriteSda(1);
}

uint8_t AT24C02_WriteByte(uint8_t address, uint8_t data)
{
    if (address >= AT24C02_CAPACITY_BYTES) return 1;
    AT24C02_Start();
    AT24C02_SendByte(AT24C02_WADDR);
    if (AT24C02_WaitAck()) goto fail;
    AT24C02_SendByte(address);
    if (AT24C02_WaitAck()) goto fail;
    AT24C02_SendByte(data);
    if (AT24C02_WaitAck()) goto fail;
    AT24C02_Stop();
    Delay_ms(5);
    return 0;
fail:
    AT24C02_Stop();
    return 1;
}

uint8_t AT24C02_WritePage(uint8_t address, const uint8_t *data)
{
    uint8_t i;
    if ((address & 0x0FU) != 0 || data == 0 ||
        address > AT24C02_CAPACITY_BYTES - AT24C02_PAGE_SIZE) return 1;
    AT24C02_Start();
    AT24C02_SendByte(AT24C02_WADDR);
    if (AT24C02_WaitAck()) goto fail;
    AT24C02_SendByte(address);
    if (AT24C02_WaitAck()) goto fail;
    for (i = 0; i < 16; ++i) {
        AT24C02_SendByte(data[i]);
        if (AT24C02_WaitAck()) goto fail;
    }
    AT24C02_Stop();
    Delay_ms(5);
    return 0;
fail:
    AT24C02_Stop();
    return 1;
}

uint8_t AT24C02_Read(uint8_t address, uint8_t *data, uint16_t length)
{
    uint16_t i;
    if (data == 0 || length == 0 || address >= AT24C02_CAPACITY_BYTES ||
        length > AT24C02_CAPACITY_BYTES - address) return 1;
    AT24C02_Start();
    AT24C02_SendByte(AT24C02_WADDR);
    if (AT24C02_WaitAck()) goto fail;
    AT24C02_SendByte(address);
    if (AT24C02_WaitAck()) goto fail;
    AT24C02_Start();
    AT24C02_SendByte(AT24C02_RADDR);
    if (AT24C02_WaitAck()) goto fail;
    for (i = 0; i < length; ++i) data[i] = AT24C02_ReadByte(i + 1 < length);
    AT24C02_Stop();
    return 0;
fail:
    AT24C02_Stop();
    return 1;
}
