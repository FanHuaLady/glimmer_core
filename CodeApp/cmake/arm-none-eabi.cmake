set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_NONE_EABI_BIN "$ENV{ARM_NONE_EABI_BIN}" CACHE PATH
    "Optional directory containing the GNU Arm Embedded executables")

find_program(ARM_NONE_EABI_GCC
    NAMES arm-none-eabi-gcc
    HINTS "${ARM_NONE_EABI_BIN}"
    REQUIRED)

get_filename_component(ARM_NONE_EABI_BIN "${ARM_NONE_EABI_GCC}" DIRECTORY)

set(CMAKE_C_COMPILER "${ARM_NONE_EABI_GCC}" CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER "${ARM_NONE_EABI_GCC}" CACHE FILEPATH "" FORCE)

foreach(tool OBJCOPY SIZE GDB)
    string(TOLOWER "${tool}" tool_lower)
    find_program(ARM_NONE_EABI_${tool}
        NAMES "arm-none-eabi-${tool_lower}"
        HINTS "${ARM_NONE_EABI_BIN}"
        REQUIRED)
endforeach()

set(CMAKE_OBJCOPY "${ARM_NONE_EABI_OBJCOPY}" CACHE FILEPATH "" FORCE)
set(CMAKE_SIZE "${ARM_NONE_EABI_SIZE}" CACHE FILEPATH "" FORCE)
set(CMAKE_GDB "${ARM_NONE_EABI_GDB}" CACHE FILEPATH "" FORCE)
