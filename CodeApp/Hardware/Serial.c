#include "Serial.h"
#include <stdio.h>

#define SERIAL_BAUDRATE 921600

UCB_CB U0CB;
uint8_t U0_RxBuff[U0_RX_SIZE];
static uint16_t lastDmaPosition;

static void Serial_RxStateInit(void)
{
    uint8_t i;
    U0CB.URxDataIN = &U0CB.URxDataPtr[0];
    U0CB.URxDataOUT = &U0CB.URxDataPtr[0];
    U0CB.URxDataEND = &U0CB.URxDataPtr[SERIAL_PACKET_COUNT - 1];
    U0CB.URxCounter = 0;
    U0CB.nextPacketIndex = 0;
    for (i = 0; i < SERIAL_PACKET_COUNT; ++i) {
        U0CB.URxDataPtr[i].start = 0;
        U0CB.URxDataPtr[i].end = 0;
        U0CB.packetValid[i] = 0;
    }
    U0CB.URxDataIN->start = U0_RxBuff;
    lastDmaPosition = 0;
}

void MarkPacketProcessed(uint8_t index)
{
    if (index >= SERIAL_PACKET_COUNT) return;
    U0CB.packetValid[index] = 0;
    if (index == (uint8_t)(U0CB.URxDataOUT - U0CB.URxDataPtr)) {
        if (++U0CB.URxDataOUT > U0CB.URxDataEND)
            U0CB.URxDataOUT = &U0CB.URxDataPtr[0];
    }
}

void Serial_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    DMA_InitTypeDef dma;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = SERIAL_BAUDRATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &usart);

    Serial_RxStateInit();
    DMA_DeInit(DMA1_Channel6);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)U0_RxBuff;
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma.DMA_BufferSize = U0_RX_SIZE;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Circular;
    dma.DMA_Priority = DMA_Priority_Medium;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel6, &dma);
    DMA_Cmd(DMA1_Channel6, ENABLE);
    USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic.NVIC_IRQChannel = USART2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

int fputc(int ch, FILE *stream)
{
    (void)stream;
    USART_SendData(USART2, (uint8_t)ch);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) {}
    return ch;
}

void USART2_IRQHandler(void)
{
    uint16_t remaining, currentPosition, received, endPosition;
    uint8_t currentIndex, nextIndex;
    volatile uint16_t clear;

    if (USART_GetITStatus(USART2, USART_IT_IDLE) == RESET) return;
    clear = USART2->SR;
    clear = USART2->DR;
    (void)clear;

    DMA_Cmd(DMA1_Channel6, DISABLE);
    remaining = DMA_GetCurrDataCounter(DMA1_Channel6);
    DMA_Cmd(DMA1_Channel6, ENABLE);
    currentPosition = (U0_RX_SIZE - remaining) % U0_RX_SIZE;
    received = (currentPosition >= lastDmaPosition)
        ? currentPosition - lastDmaPosition
        : U0_RX_SIZE - lastDmaPosition + currentPosition;
    if (received == 0) return;

    currentIndex = (uint8_t)(U0CB.URxDataIN - U0CB.URxDataPtr);
    U0CB.packetValid[currentIndex] = 1;
    U0CB.URxDataIN->start = &U0_RxBuff[lastDmaPosition];
    endPosition = (lastDmaPosition + received - 1) % U0_RX_SIZE;
    U0CB.URxDataIN->end = &U0_RxBuff[endPosition];
    U0CB.URxCounter = (U0CB.URxCounter + received) % U0_RX_SIZE;

    nextIndex = (currentIndex + 1) % SERIAL_PACKET_COUNT;
    if (U0CB.packetValid[nextIndex]) {
        U0CB.packetValid[nextIndex] = 0;
        if (nextIndex == (uint8_t)(U0CB.URxDataOUT - U0CB.URxDataPtr)) {
            if (++U0CB.URxDataOUT > U0CB.URxDataEND)
                U0CB.URxDataOUT = &U0CB.URxDataPtr[0];
        }
    }
    if (++U0CB.URxDataIN > U0CB.URxDataEND)
        U0CB.URxDataIN = &U0CB.URxDataPtr[0];
    U0CB.nextPacketIndex = (uint8_t)(U0CB.URxDataIN - U0CB.URxDataPtr);
    U0CB.URxDataIN->start = &U0_RxBuff[currentPosition];
    lastDmaPosition = currentPosition;
}
