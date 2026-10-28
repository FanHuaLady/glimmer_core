#include "stm32f10x.h"
#include "Delay.h"
#include "Key.h"
#include "OLED.h"

int main(void)
{
	OLED_Init();
	OLED_ShowString(0, 0, "APP RUNNING", OLED_8X16);
	OLED_ShowString(0, 20, "Boot jump OK", OLED_8X16);
	OLED_ShowString(0, 40, "GlimmerCore", OLED_8X16);
	OLED_Update();
	Key_Init();

	while (1) {
		Key_Scan();
		Delay_ms(10);
	}
}
