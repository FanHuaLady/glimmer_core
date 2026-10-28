# GlimmerCore

STM32F103C8T6 board project with a VS Code/CMake firmware workflow, a
20-KB BootLoader, and an application partition starting at `0x08005000`.

## Project layout

- [`Code/`](Code/) — complete firmware project: BootLoader and application,
  SWD flashing, and Cortex-Debug configuration.
- [`CodeApp/`](CodeApp/) — application-only project for later experiments and
  UART/XMODEM update testing. It does not replace the BootLoader.
- [`PCB/`](PCB/) — schematics, PCB source/export files, and pin notes.
- [`Docs/`](Docs/) — board reference documents.

## Build and flash

Install CMake, Ninja, the GNU Arm Embedded Toolchain, OpenOCD, and the VS Code
extensions CMake Tools, C/C++, and Cortex-Debug. The scripts add the project's
expected local tool directories to `PATH`; edit `scripts/env.ps1`, the CMake
presets, or the Cortex-Debug launch configuration if your tools are installed
elsewhere.

From `Code/`, build and flash both images:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\check_env.ps1
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
powershell -ExecutionPolicy Bypass -File scripts\flash.ps1
```

See [`Code/README_VSCODE_CMAKE.md`](Code/README_VSCODE_CMAKE.md) for memory
layout, OLED, UART, BootLoader, and debugging details. For the application-only
workflow, see [`CodeApp/README.md`](CodeApp/README.md).

## Hardware notes

The BootLoader uses USART2 on PA2/PA3 and W25Q128 for staged updates. AT24C02
is optional for the current firmware. The OLED uses PB6/PB7; its orientation
commands are set in `Code/Hardware/OLED.c`.
