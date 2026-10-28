#include "Key.h"

#define KEY_ADC ADC1
#define KEY_ADC_CHANNEL ADC_Channel_9
#define KEY_ADC_PORT GPIOB
#define KEY_ADC_PIN GPIO_Pin_1
#define KEY_DEBOUNCE_SCANS 4U

static KeyCode candidateKey;
static KeyCode stableKey;
static KeyCode pendingEvent;
static uint8_t candidateCount;

void Key_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_ADC1,
                           ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    gpio.GPIO_Pin = KEY_ADC_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_ADC_PORT, &gpio);

    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = DISABLE;
    adc.ADC_ContinuousConvMode = DISABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 1;
    ADC_Init(KEY_ADC, &adc);
    ADC_RegularChannelConfig(KEY_ADC, KEY_ADC_CHANNEL, 1,
                             ADC_SampleTime_239Cycles5);
    ADC_Cmd(KEY_ADC, ENABLE);
    ADC_ResetCalibration(KEY_ADC);
    while (ADC_GetResetCalibrationStatus(KEY_ADC) != RESET) {}
    ADC_StartCalibration(KEY_ADC);
    while (ADC_GetCalibrationStatus(KEY_ADC) != RESET) {}
    candidateKey = KEY_NONE;
    stableKey = KEY_NONE;
    pendingEvent = KEY_NONE;
    candidateCount = 0;
}

uint16_t Key_ReadAdc(void)
{
    uint32_t total = 0;
    uint8_t i;

    for (i = 0; i < 4; ++i) {
        ADC_SoftwareStartConvCmd(KEY_ADC, ENABLE);
        while (ADC_GetFlagStatus(KEY_ADC, ADC_FLAG_EOC) == RESET) {}
        total += ADC_GetConversionValue(KEY_ADC);
    }
    return (uint16_t)(total / 4U);
}

KeyCode Key_Scan(void)
{
    uint16_t value = Key_ReadAdc();
    KeyCode sampledKey;

    /* R9=10 kOhm and R11/R13=2.49 kOhm form three voltage levels. */
    if (value < 400U) sampledKey = KEY_SW3;
    else if (value < 1100U) sampledKey = KEY_SW4;
    else if (value < 2600U) sampledKey = KEY_SW6;
    else sampledKey = KEY_NONE;

    if (sampledKey == candidateKey) {
        if (candidateCount < KEY_DEBOUNCE_SCANS) ++candidateCount;
    } else {
        candidateKey = sampledKey;
        candidateCount = 1;
    }

    if (candidateCount >= KEY_DEBOUNCE_SCANS && stableKey != candidateKey) {
        stableKey = candidateKey;
        if (stableKey != KEY_NONE) pendingEvent = stableKey;
    }
    return stableKey;
}

KeyCode Key_GetEvent(void)
{
    KeyCode event = pendingEvent;
    pendingEvent = KEY_NONE;
    return event;
}
