#include "stm32f10x.h"
#include "W25Q128.h"
#include "OLED.h"

#include <stdint.h>

#define BOOT_UART_BAUDRATE       115200UL
#define BOOT_UART_TIMEOUT_MS     1000U
#define BOOT_ENTRY_WINDOW_MS     1500U

#define APP_BASE_ADDRESS         0x08005000UL
#define APP_FLASH_SIZE           (44UL * 1024UL)
#define FLASH_PAGE_SIZE          1024UL

#define STAGING_ADDRESS          0x000000UL
#define METADATA_ADDRESS         0x00F000UL
#define METADATA_MAGIC           0x474C4D52UL
#define METADATA_COMMIT          0xA55A3CC3UL

#define XMODEM_SOH               0x01U
#define XMODEM_EOT               0x04U
#define XMODEM_ACK               0x06U
#define XMODEM_NAK               0x15U
#define XMODEM_CAN               0x18U
#define XMODEM_CRC_REQUEST       0x43U
#define XMODEM_PACKET_SIZE       128U
#define XMODEM_MAX_ERRORS        10U

typedef struct {
    uint32_t magic;
    uint32_t length;
    uint32_t crc16;
    uint32_t length_xor;
    uint32_t commit;
} BootMetadata;

static uint8_t g_packet[XMODEM_PACKET_SIZE];
static uint8_t g_flash_buffer[XMODEM_PACKET_SIZE];

static void Boot_UartInit(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = BOOT_UART_BAUDRATE;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &usart);
    USART_Cmd(USART2, ENABLE);
}

static void Boot_UartPut(uint8_t value)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) { }
    USART_SendData(USART2, value);
}

static void Boot_UartPuts(const char *text)
{
    while (*text != '\0') Boot_UartPut((uint8_t)*text++);
}

static void Boot_UartLine(const char *text)
{
    Boot_UartPuts(text);
    Boot_UartPuts("\r\n");
}

static int Boot_UartGet(uint8_t *value, uint32_t timeout_ms)
{
    while (timeout_ms != 0U) {
        SysTick->LOAD = (SystemCoreClock / 1000UL) - 1UL;
        SysTick->VAL = 0;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;

        while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {
            if (USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET) {
                *value = (uint8_t)USART_ReceiveData(USART2);
                SysTick->CTRL = 0;
                return 1;
            }
        }
        --timeout_ms;
    }
    SysTick->CTRL = 0;
    return 0;
}

static uint16_t Boot_Crc16Update(uint16_t crc, uint8_t value)
{
    uint8_t bit;
    crc ^= (uint16_t)value << 8;
    for (bit = 0; bit < 8U; ++bit) {
        crc = (crc & 0x8000U) ? (uint16_t)((crc << 1) ^ 0x1021U)
                              : (uint16_t)(crc << 1);
    }
    return crc;
}

static uint16_t Boot_Crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0;
    uint32_t i;
    for (i = 0; i < length; ++i) crc = Boot_Crc16Update(crc, data[i]);
    return crc;
}

static uint16_t Boot_Crc16External(uint32_t address, uint32_t length)
{
    uint16_t crc = 0;
    while (length != 0U) {
        uint32_t count = (length > sizeof(g_flash_buffer))
            ? sizeof(g_flash_buffer) : length;
        uint32_t i;
        if (W25Q128_Read(address, g_flash_buffer, count) != 0U) return 0xFFFFU;
        for (i = 0; i < count; ++i)
            crc = Boot_Crc16Update(crc, g_flash_buffer[i]);
        address += count;
        length -= count;
    }
    return crc;
}

static uint16_t Boot_Crc16Internal(uint32_t address, uint32_t length)
{
    uint16_t crc = 0;
    const uint8_t *data = (const uint8_t *)address;
    uint32_t i;
    for (i = 0; i < length; ++i) crc = Boot_Crc16Update(crc, data[i]);
    return crc;
}

static int Boot_VectorValid(uint32_t stack, uint32_t reset, uint32_t length)
{
    uint32_t app_limit = APP_BASE_ADDRESS + length;
    if (length < 8U || length > APP_FLASH_SIZE) return 0;
    if (stack < 0x20000000UL || stack > 0x20005000UL || (stack & 3U) != 0U)
        return 0;
    if ((reset & 1U) == 0U) return 0;
    reset &= ~1UL;
    return reset >= APP_BASE_ADDRESS && reset < app_limit;
}

static int Boot_ReadStageVectors(uint32_t length, uint32_t *stack, uint32_t *reset)
{
    uint8_t vectors[8];
    if (W25Q128_Read(STAGING_ADDRESS, vectors, sizeof(vectors)) != 0U) return 0;
    *stack = (uint32_t)vectors[0] | ((uint32_t)vectors[1] << 8) |
             ((uint32_t)vectors[2] << 16) | ((uint32_t)vectors[3] << 24);
    *reset = (uint32_t)vectors[4] | ((uint32_t)vectors[5] << 8) |
             ((uint32_t)vectors[6] << 16) | ((uint32_t)vectors[7] << 24);
    return Boot_VectorValid(*stack, *reset, length);
}

static int Boot_MetadataRead(BootMetadata *metadata)
{
    if (W25Q128_Read(METADATA_ADDRESS, (uint8_t *)metadata,
                     sizeof(*metadata)) != 0U)
        return 0;
    return metadata->magic == METADATA_MAGIC &&
           metadata->commit == METADATA_COMMIT &&
           metadata->length_xor == ~metadata->length &&
           metadata->length >= 8U && metadata->length <= APP_FLASH_SIZE &&
           metadata->crc16 <= 0xFFFFU;
}

static int Boot_MetadataWrite(uint32_t length, uint16_t crc)
{
    BootMetadata metadata;
    uint32_t commit = METADATA_COMMIT;
    metadata.magic = METADATA_MAGIC;
    metadata.length = length;
    metadata.crc16 = crc;
    metadata.length_xor = ~length;
    metadata.commit = 0xFFFFFFFFUL;
    if (W25Q128_Write(METADATA_ADDRESS, (const uint8_t *)&metadata,
                      sizeof(metadata) - sizeof(metadata.commit)) != 0U)
        return 0;
    return W25Q128_Write(METADATA_ADDRESS + 16U, (const uint8_t *)&commit,
                         sizeof(commit)) == 0U;
}

static int Boot_AppValid(void)
{
    uint32_t stack = *(const uint32_t *)APP_BASE_ADDRESS;
    uint32_t reset = *(const uint32_t *)(APP_BASE_ADDRESS + 4U);
    return Boot_VectorValid(stack, reset, APP_FLASH_SIZE);
}

static void Boot_JumpToApp(void)
{
    uint32_t stack = *(const uint32_t *)APP_BASE_ADDRESS;
    uint32_t reset = *(const uint32_t *)(APP_BASE_ADDRESS + 4U);
    void (*entry)(void) = (void (*)(void))reset;

    __disable_irq();
    USART_Cmd(USART2, DISABLE);
    USART_DeInit(USART2);
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    NVIC->ICER[0] = 0xFFFFFFFFUL;
    NVIC->ICER[1] = 0xFFFFFFFFUL;
    NVIC->ICPR[0] = 0xFFFFFFFFUL;
    NVIC->ICPR[1] = 0xFFFFFFFFUL;
    SCB->VTOR = APP_BASE_ADDRESS;
    __DSB();
    __ISB();
    __asm volatile("msr msp, %0\n cpsie i\n bx %1" : : "r"(stack), "r"(entry) : "memory");
    for (;;) { }
}

static int Boot_InstallStagedImage(const BootMetadata *metadata)
{
    uint32_t stack;
    uint32_t reset;
    uint32_t pages = (metadata->length + FLASH_PAGE_SIZE - 1U) / FLASH_PAGE_SIZE;
    uint32_t page;
    uint32_t offset;

    if (!Boot_ReadStageVectors(metadata->length, &stack, &reset) ||
        Boot_Crc16External(STAGING_ADDRESS, metadata->length) != metadata->crc16) {
        Boot_UartLine("Staged image invalid; keeping current application.");
        return 0;
    }

    FLASH_Unlock();
    for (page = 0; page < pages; ++page) {
        if (FLASH_ErasePage(APP_BASE_ADDRESS + page * FLASH_PAGE_SIZE) != FLASH_COMPLETE) {
            FLASH_Lock();
            Boot_UartLine("Internal Flash erase failed.");
            return 0;
        }
    }

    for (offset = 0; offset < metadata->length; offset += sizeof(g_flash_buffer)) {
        uint32_t count = metadata->length - offset;
        uint32_t i;
        if (count > sizeof(g_flash_buffer)) count = sizeof(g_flash_buffer);
        if (W25Q128_Read(STAGING_ADDRESS + offset, g_flash_buffer, count) != 0U) {
            FLASH_Lock();
            Boot_UartLine("External Flash read failed.");
            return 0;
        }
        for (i = 0; i < count; i += 2U) {
            uint16_t halfword = g_flash_buffer[i];
            if (i + 1U < count) halfword |= (uint16_t)g_flash_buffer[i + 1U] << 8;
            else halfword |= 0xFF00U;
            if (FLASH_ProgramHalfWord(APP_BASE_ADDRESS + offset + i,
                                      halfword) != FLASH_COMPLETE) {
                FLASH_Lock();
                Boot_UartLine("Internal Flash write failed.");
                return 0;
            }
        }
    }
    FLASH_Lock();

    if (Boot_Crc16Internal(APP_BASE_ADDRESS, metadata->length) != metadata->crc16 ||
        !Boot_AppValid()) {
        Boot_UartLine("Installed image verification failed.");
        return 0;
    }
    if (W25Q128_Erase4K(METADATA_ADDRESS) != 0U) {
        Boot_UartLine("Could not clear update marker; install will be retried.");
        return 0;
    }
    Boot_UartLine("Application update installed.");
    return 1;
}

static int Boot_ReceiveXmodem(void)
{
    uint32_t total = 0;
    uint32_t erased_until = 0;
    uint8_t expected = 1;
    uint8_t started = 0;
    uint8_t errors = 0;
    uint8_t value;
    uint16_t image_crc = 0;

    if (W25Q128_Erase4K(METADATA_ADDRESS) != 0U ||
        W25Q128_Erase4K(STAGING_ADDRESS) != 0U) {
        Boot_UartLine("W25Q128 erase failed.");
        return 0;
    }
    erased_until = W25Q128_SECTOR_SIZE;
    Boot_UartLine("Send XMODEM-CRC 128-byte SOH packets now.");
    Boot_UartPut(XMODEM_CRC_REQUEST);

    for (;;) {
        uint8_t block;
        uint8_t inverse;
        uint8_t crc_hi;
        uint8_t crc_lo;
        uint16_t received_crc;
        uint16_t calculated_crc;
        uint16_t i;

        if (!Boot_UartGet(&value, BOOT_UART_TIMEOUT_MS)) {
            if (!started) {
                Boot_UartPut(XMODEM_CRC_REQUEST);
                continue;
            }
            if (++errors > XMODEM_MAX_ERRORS) {
                Boot_UartLine("XMODEM timed out.");
                return 0;
            }
            Boot_UartPut(XMODEM_NAK);
            continue;
        }

        if (value == XMODEM_EOT) {
            Boot_UartPut(XMODEM_ACK);
            break;
        }
        if (value == XMODEM_CAN) {
            Boot_UartLine("Transfer cancelled.");
            return 0;
        }
        if (value != XMODEM_SOH) continue;

        if (!Boot_UartGet(&block, BOOT_UART_TIMEOUT_MS) ||
            !Boot_UartGet(&inverse, BOOT_UART_TIMEOUT_MS)) {
            Boot_UartPut(XMODEM_NAK);
            if (++errors > XMODEM_MAX_ERRORS) return 0;
            continue;
        }
        for (i = 0; i < XMODEM_PACKET_SIZE; ++i) {
            if (!Boot_UartGet(&g_packet[i], BOOT_UART_TIMEOUT_MS)) break;
        }
        if (i != XMODEM_PACKET_SIZE ||
            !Boot_UartGet(&crc_hi, BOOT_UART_TIMEOUT_MS) ||
            !Boot_UartGet(&crc_lo, BOOT_UART_TIMEOUT_MS)) {
            Boot_UartPut(XMODEM_NAK);
            if (++errors > XMODEM_MAX_ERRORS) return 0;
            continue;
        }

        received_crc = ((uint16_t)crc_hi << 8) | crc_lo;
        calculated_crc = Boot_Crc16(g_packet, XMODEM_PACKET_SIZE);
        if ((uint8_t)(block + inverse) != 0xFFU ||
            calculated_crc != received_crc) {
            Boot_UartPut(XMODEM_NAK);
            if (++errors > XMODEM_MAX_ERRORS) return 0;
            continue;
        }

        if (block == (uint8_t)(expected - 1U)) {
            Boot_UartPut(XMODEM_ACK);
            continue;
        }
        if (block != expected || total + XMODEM_PACKET_SIZE > APP_FLASH_SIZE) {
            Boot_UartPut(XMODEM_NAK);
            if (++errors > XMODEM_MAX_ERRORS) return 0;
            continue;
        }

        if (total + XMODEM_PACKET_SIZE > erased_until) {
            if (W25Q128_Erase4K(STAGING_ADDRESS + erased_until) != 0U) {
                Boot_UartLine("W25Q128 staging erase failed.");
                return 0;
            }
            erased_until += W25Q128_SECTOR_SIZE;
        }
        if (W25Q128_Write(STAGING_ADDRESS + total, g_packet,
                          XMODEM_PACKET_SIZE) != 0U) {
            Boot_UartLine("W25Q128 staging write failed.");
            return 0;
        }
        for (i = 0; i < XMODEM_PACKET_SIZE; ++i)
            image_crc = Boot_Crc16Update(image_crc, g_packet[i]);
        total += XMODEM_PACKET_SIZE;
        ++expected;
        started = 1;
        errors = 0;
        Boot_UartPut(XMODEM_ACK);
    }

    if (total < 8U) {
        Boot_UartLine("No complete image received.");
        return 0;
    }
    {
        uint32_t stack;
        uint32_t reset;
        if (!Boot_ReadStageVectors(total, &stack, &reset)) {
            Boot_UartLine("Image vector table is invalid.");
            return 0;
        }
    }
    if (Boot_Crc16External(STAGING_ADDRESS, total) != image_crc ||
        !Boot_MetadataWrite(total, image_crc)) {
        Boot_UartLine("Staged image verification or metadata write failed.");
        return 0;
    }
    Boot_UartLine("Image received; installing.");
    {
        BootMetadata metadata;
        if (!Boot_MetadataRead(&metadata) || !Boot_InstallStagedImage(&metadata))
            return 0;
    }
    return 1;
}

int main(void)
{
    BootMetadata metadata;
    uint8_t value;
    uint32_t elapsed;
    uint32_t flash_id;

    OLED_Init();
    OLED_ShowString(0, 0, "BOOTLOADER", OLED_8X16);
    OLED_ShowString(0, 20, "Starting app", OLED_8X16);
    OLED_ShowString(0, 40, "in 2 seconds", OLED_8X16);
    OLED_Update();

    Boot_UartInit();
    W25Q128_Init();
    flash_id = W25Q128_ReadJedecId();

    if (flash_id == 0xEF4018UL && Boot_MetadataRead(&metadata)) {
        Boot_UartLine("Recovering pending application update.");
        (void)Boot_InstallStagedImage(&metadata);
    }

    Boot_UartLine("GlimmerCore bootloader - press w within 1.5 s for menu.");
    for (elapsed = 0; elapsed < BOOT_ENTRY_WINDOW_MS; ++elapsed) {
        if (elapsed == 500U) {
            OLED_Clear();
            OLED_ShowString(0, 0, "BOOTLOADER", OLED_8X16);
            OLED_ShowString(0, 20, "Starting app", OLED_8X16);
            OLED_ShowString(0, 40, "in 1 second", OLED_8X16);
            OLED_Update();
        }
        if (Boot_UartGet(&value, 1U) && value == (uint8_t)'w') break;
    }
    if (elapsed >= BOOT_ENTRY_WINDOW_MS && Boot_AppValid()) {
        OLED_Clear();
        OLED_ShowString(0, 20, "Starting app", OLED_8X16);
        OLED_Update();
        Boot_JumpToApp();
    }

    OLED_Clear();
    OLED_ShowString(0, 0, "BOOTLOADER MENU", OLED_8X16);
    OLED_ShowString(0, 20, "2: UART update", OLED_8X16);
    OLED_ShowString(0, 40, "r: run app", OLED_8X16);
    OLED_Update();

    Boot_UartLine("2: UART firmware update (XMODEM-CRC)");
    Boot_UartLine("r: run application");
    Boot_UartLine("7: reboot");
    if (flash_id != 0xEF4018UL) Boot_UartLine("ERROR: W25Q128 not detected; update disabled.");

    for (;;) {
        if (!Boot_UartGet(&value, 0xFFFFFFFFUL)) continue;
        if (value == (uint8_t)'2') {
            if (flash_id == 0xEF4018UL && Boot_ReceiveXmodem()) {
                Boot_UartLine("Update complete; rebooting.");
                NVIC_SystemReset();
            }
            Boot_UartLine("Update failed; application remains available when valid.");
        } else if (value == (uint8_t)'r') {
            if (Boot_AppValid()) Boot_JumpToApp();
            Boot_UartLine("No valid application image.");
        } else if (value == (uint8_t)'7') {
            NVIC_SystemReset();
        }
    }
}
