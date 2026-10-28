param(
    [switch]$NoBuild
)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\env.ps1"

if (-not $NoBuild) {
    & "$PSScriptRoot\build.ps1"
    if ($LASTEXITCODE -ne 0) {
        throw "Firmware build failed; refusing to flash."
    }
}

$bootloaderElf = Join-Path $PSScriptRoot "..\build\debug\Bootloader.elf"
$appElf = Join-Path $PSScriptRoot "..\build\debug\GlimmerCore.elf"
foreach ($elf in @($bootloaderElf, $appElf)) {
    if (-not (Test-Path $elf)) {
        throw "Firmware ELF not found: $elf. Run scripts\build.ps1 first."
    }
}

$config = Join-Path $PSScriptRoot "..\debug\openocd_stm32f103c8_cmsis_dap.cfg"
$bootloaderOpenOcd = (Resolve-Path $bootloaderElf).Path -replace "\\", "/"
$appOpenOcd = (Resolve-Path $appElf).Path -replace "\\", "/"

openocd -f $config -c "init" -c "reset halt" `
    -c "program {$bootloaderOpenOcd} verify" `
    -c "program {$appOpenOcd} verify" `
    -c "reset run" -c "shutdown"
if ($LASTEXITCODE -ne 0) {
    throw "OpenOCD failed with exit code $LASTEXITCODE"
}
