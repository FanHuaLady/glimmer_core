#include "stm32f10x.h"
#include "Delay.h"
#include "Key.h"
#include "OLED.h"

int main(void)
{
	OLED_Init();
	Key_Init();

	while (1) {
		Key_Scan();
		Delay_ms(10);
	}
}

