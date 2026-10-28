cmake_minimum_required(VERSION 3.21)

project(GlimmerCore C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(APP_TARGET GlimmerCore)
set(BOOTLOADER_TARGET Bootloader)
set(APP_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F103C8T6_FLASH.ld")
set(BOOTLOADER_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F103C8T6_BOOT.ld")

file(GLOB STM32_STDPERIPH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Library/*.c")

set(APP_SOURCES
    Start/startup_stm32f103c8_gcc.S
    Start/core_cm3.c
    Start/system_stm32f10x.c
    System/Delay.c
    Hardware/AT24C02.c
    Hardware/Key.c
    Hardware/OLED.c
    Hardware/OLED_Data.c
    Hardware/Serial.c
    Hardware/W25Q128.c
    App/W25Q128_Test.c
    User/main.c
    User/stm32f10x_it.c
    ${STM32_STDPERIPH_SOURCES}
)

set(BOOTLOADER_SOURCES
    Start/startup_stm32f103c8_gcc.S
    Start/core_cm3.c
    Start/system_stm32f10x.c
    System/Delay.c
    Hardware/OLED.c
    Hardware/OLED_Data.c
    Hardware/W25Q128.c
    Bootloader/Bootloader.c
    ${STM32_STDPERIPH_SOURCES}
)

function(configure_firmware_target target linker_script vect_offset)
target_include_directories(${target} PRIVATE
    Start
    Library
    User
    System
    Hardware
    App
    Bootloader
)

target_compile_definitions(${target} PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
    VECT_TAB_OFFSET=${vect_offset}
)

target_compile_options(${target} PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    $<$<COMPILE_LANGUAGE:C>:-Og>
    $<$<COMPILE_LANGUAGE:C>:-g3>
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>
)

if("${target}" STREQUAL "${APP_TARGET}" OR "${target}" STREQUAL "${BOOTLOADER_TARGET}")
    set_source_files_properties(Hardware/OLED_Data.c PROPERTIES
        COMPILE_OPTIONS "-Wno-missing-braces")
endif()

target_link_options(${target} PRIVATE
    -mcpu=cortex-m3
    -mthumb
    "-T${linker_script}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map"
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs
)

set_target_properties(${target} PROPERTIES
    OUTPUT_NAME ${target}
    SUFFIX ".elf"
)

add_custom_command(TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:${target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${target}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:${target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:${target}>"
    BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/${target}.hex"
        "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin"
    COMMENT "Generating HEX/BIN firmware images"
)
endfunction()

add_executable(${APP_TARGET} ${APP_SOURCES})
configure_firmware_target(${APP_TARGET} "${APP_LINKER_SCRIPT}" 0x5000)

add_executable(${BOOTLOADER_TARGET} ${BOOTLOADER_SOURCES})
configure_firmware_target(${BOOTLOADER_TARGET} "${BOOTLOADER_LINKER_SCRIPT}" 0x0)
target_compile_options(${BOOTLOADER_TARGET} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Os>
)
