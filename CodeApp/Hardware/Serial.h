#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

#define U0_RX_SIZE 2048
#define U0_RX_MAX 256
#define SERIAL_PACKET_COUNT 10

typedef struct {
    uint8_t *start;
    uint8_t *end;
} UCB_URxBuffptr;

typedef struct {
    uint16_t URxCounter;
    UCB_URxBuffptr URxDataPtr[SERIAL_PACKET_COUNT];
    UCB_URxBuffptr *URxDataIN;
    UCB_URxBuffptr *URxDataOUT;
    UCB_URxBuffptr *URxDataEND;
    uint8_t packetValid[SERIAL_PACKET_COUNT];
    uint8_t nextPacketIndex;
} UCB_CB;

extern UCB_CB U0CB;
extern uint8_t U0_RxBuff[U0_RX_SIZE];

void Serial_Init(void);
void MarkPacketProcessed(uint8_t index);

#endif
