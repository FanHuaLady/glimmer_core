#include "stm32f10x.h"
#include "Delay.h"

static void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	/* Enable GPIOC clock. */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	/* Configure PC15 as a 2 MHz push-pull output. */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Start with the LED output low. */
	GPIO_ResetBits(GPIOC, GPIO_Pin_15);
}

int main(void)
{
	LED_Init();

	while (1)
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_15);
		Delay_ms(500);

		GPIO_ResetBits(GPIOC, GPIO_Pin_15);
		Delay_ms(500);
	}
}
