#ifndef __KEY_H
#define __KEY_H

#include "stm32f10x.h"

typedef enum {
    KEY_NONE = 0,
    KEY_SW6,
    KEY_SW4,
    KEY_SW3
} KeyCode;

void Key_Init(void);
uint16_t Key_ReadAdc(void);
KeyCode Key_Scan(void);
KeyCode Key_GetEvent(void);

#endif
