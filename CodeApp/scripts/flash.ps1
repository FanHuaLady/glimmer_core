param([switch]$NoBuild)

$ErrorActionPreference = "Stop"
. "$PSScriptRoot\env.ps1"
if (-not $NoBuild) {
    & "$PSScriptRoot\build.ps1"
    if ($LASTEXITCODE -ne 0) { throw "Application build failed; refusing to flash." }
}

$elf = Join-Path $PSScriptRoot "..\build\debug\GlimmerCore.elf"
$config = Join-Path $PSScriptRoot "..\debug\openocd_stm32f103c8_cmsis_dap.cfg"
if (-not (Test-Path $elf)) { throw "Application ELF not found: $elf" }
$elfOpenOcd = (Resolve-Path $elf).Path -replace "\\", "/"
openocd -f $config -c "init" -c "reset halt" `
    -c "program {$elfOpenOcd} verify" -c "reset run" -c "shutdown"
if ($LASTEXITCODE -ne 0) { throw "OpenOCD failed with exit code $LASTEXITCODE" }
