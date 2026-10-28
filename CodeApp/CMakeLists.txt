cmake_minimum_required(VERSION 3.21)

project(GlimmerCoreApp C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_executable(GlimmerCore ${APP_SOURCES})
target_include_directories(GlimmerCore PRIVATE
    Start Library User System Hardware App)
target_compile_definitions(GlimmerCore PRIVATE
    STM32F10X_MD USE_STDPERIPH_DRIVER VECT_TAB_OFFSET=0x5000)
target_compile_options(GlimmerCore PRIVATE
    -mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections
    $<$<COMPILE_LANGUAGE:C>:-Og;-g3;-Wall;-Wextra;-Wno-unused-parameter>)
set_source_files_properties(Hardware/OLED_Data.c PROPERTIES
    COMPILE_OPTIONS "-Wno-missing-braces")
target_link_options(GlimmerCore PRIVATE
    -mcpu=cortex-m3 -mthumb
    "-T${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F103C8T6_FLASH.ld"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/GlimmerCore.map"
    -Wl,--gc-sections -Wl,--print-memory-usage
    --specs=nano.specs --specs=nosys.specs)
set_target_properties(GlimmerCore PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET GlimmerCore POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex "$<TARGET_FILE:GlimmerCore>"
            "${CMAKE_CURRENT_BINARY_DIR}/GlimmerCore.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary "$<TARGET_FILE:GlimmerCore>"
            "${CMAKE_CURRENT_BINARY_DIR}/GlimmerCore.bin"
    COMMAND "${CMAKE_SIZE}" "$<TARGET_FILE:GlimmerCore>"
    BYPRODUCTS
        "${CMAKE_CURRENT_BINARY_DIR}/GlimmerCore.hex"
        "${CMAKE_CURRENT_BINARY_DIR}/GlimmerCore.bin"
    COMMENT "Generating application HEX/BIN images")
