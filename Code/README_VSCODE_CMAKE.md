# GlimmerCore VS Code / CMake project

This folder is a GNU Arm/CMake project for VS Code. It builds two images for the
STM32F103C8's 64 KB Flash and 20 KB SRAM: a 20 KB bootloader at `0x08000000` and
the application at `0x08005000` (44 KB maximum).

## Required tools

The scripts add the existing local installations under `D:\MyCmake`,
`D:\MyNinja`, `D:\MyArmGunToolchainArmNoneEabi`, and `D:\MyOpenocd` when those
directories exist. If your tools are installed elsewhere, update
`scripts/env.ps1`, `cmake/arm-none-eabi.cmake`, `CMakePresets.json`, and
`.vscode/launch.json`.

Install the VS Code extensions **CMake Tools**, **C/C++**, and
**Cortex-Debug**. Check the command-line toolchain from this folder with:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\check_env.ps1
```

## Build

Open this `Code` folder in VS Code. CMake Tools uses the `stm32f103-debug`
preset. From a PowerShell terminal, the same build is:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

Build outputs are written under `build/debug/`:

```text
Bootloader.elf / .hex / .bin / .map
GlimmerCore.elf
GlimmerCore.hex
GlimmerCore.bin
GlimmerCore.map
```

## Flash and debug

Connect a CMSIS-DAP probe to the board's SWDIO, SWCLK, GND, and target-voltage
reference pins. Then run **Terminal > Run Task > Flash firmware (CMSIS-DAP)**,
or execute:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\flash.ps1
```

The flash script builds the current source first, then asks OpenOCD to program
and verify both images. This works for the first setup and later full flashes.
In VS Code, choose **Run and Debug > GlimmerCore STM32F103C8 (CMSIS-DAP)** to
build and debug the application ELF.

The OpenOCD configuration uses CMSIS-DAP over SWD and the OpenOCD
`target/stm32f1x.cfg` target. It requests a Cortex-M system reset, so the probe
does not need a separate NRST connection.

## UART firmware update

The bootloader uses USART2 on PA2 (TX) and PA3 (RX), 115200 baud, 8 data bits,
no parity, and one stop bit. Cross TX/RX when connecting the ESP32-S3 and share
ground; use compatible 3.3 V logic levels.

At reset, the bootloader waits 1.5 seconds for lowercase `w`. Without it, a
valid application starts automatically. In the menu, send `2` and transfer a
raw application `.bin` using XMODEM-CRC with 128-byte SOH packets. The firmware
is checked and staged in W25Q128 before it is copied into internal Flash. Send
`r` to run the application or `7` to reset. XMODEM 1K STX packets are not
supported.

AT24C02 is not required. The W25Q128 stores the staged image and recovery
metadata, so it must be present and working. On a blank board, use **Flash
firmware (CMSIS-DAP)** once over SWD to install both images at their linker
addresses.
