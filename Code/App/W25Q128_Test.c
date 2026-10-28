#include "W25Q128_Test.h"
#include "W25Q128.h"
#include "OLED.h"

#define TEST_ADDRESS 0x00FF00F8UL
#define TEST_BLOCK_ADDRESS 0x00FF0000UL

static uint8_t TestBuffer[16];
static uint8_t ReadBuffer[16];

static uint8_t BuffersEqual(const uint8_t *left, const uint8_t *right,
                            uint16_t length)
{
    uint16_t i;
    for (i = 0; i < length; ++i) {
        if (left[i] != right[i]) return 0;
    }
    return 1;
}

void W25Q128_Test_Run(void)
{
    uint32_t id;
    uint8_t eraseOk;
    uint8_t writeOk;
    uint8_t readOk;
    uint8_t idOk;
    uint8_t i;

    for (i = 0; i < sizeof(TestBuffer); ++i) {
        TestBuffer[i] = (uint8_t)(0x30U + i * 7U);
        ReadBuffer[i] = 0;
    }

    OLED_Clear();
    OLED_ShowString(0, 0, "W25Q128 TEST", OLED_8X16);
    OLED_ShowString(0, 20, "INIT...", OLED_6X8);
    OLED_Update();

    W25Q128_Init();
    id = W25Q128_ReadJedecId();
    idOk = (uint8_t)(id == 0xEF4018UL);
    eraseOk = (uint8_t)(W25Q128_Erase64K(TEST_BLOCK_ADDRESS) == 0);
    writeOk = (uint8_t)(eraseOk && W25Q128_Write(TEST_ADDRESS, TestBuffer,
                                                  sizeof(TestBuffer)) == 0);
    readOk = (uint8_t)(writeOk && W25Q128_Read(TEST_ADDRESS, ReadBuffer,
                                                sizeof(ReadBuffer)) == 0 &&
                       BuffersEqual(TestBuffer, ReadBuffer,
                                    sizeof(TestBuffer)));

    OLED_Clear();
    OLED_ShowString(0, 0, "W25Q128 TEST", OLED_8X16);
    OLED_ShowString(0, 18, "JEDEC:", OLED_6X8);
    OLED_ShowHexNum(48, 18, id, 6, OLED_6X8);
    OLED_ShowString(0, 28, idOk ? "ID: OK" : "ID: FAIL", OLED_6X8);
    OLED_ShowString(0, 37, eraseOk ? "ERASE: OK" : "ERASE: FAIL", OLED_6X8);
    OLED_ShowString(0, 46, writeOk ? "WRITE: OK" : "WRITE: FAIL", OLED_6X8);
    OLED_ShowString(0, 55, readOk ? "READ: OK" : "READ: FAIL", OLED_6X8);
    OLED_Update();
}
