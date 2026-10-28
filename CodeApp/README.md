# GlimmerCore application experiment project

`CodeApp` contains only the STM32F103C8 application partition. It does not
contain or build the BootLoader. The linker places the vector table at
`0x08005000`, leaving the first 20 KB for the existing BootLoader.

Build from this directory:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
```

The application image is written to `build/debug/GlimmerCore.bin`. This is the
binary to use for the BootLoader's UART update experiment. The `.hex` and `.elf`
files are also generated for SWD debugging.

The `Flash application only` task and `scripts\flash.ps1` program only this
application image at `0x08005000`; they do not erase or replace the BootLoader.
The board must already contain the compatible BootLoader.
