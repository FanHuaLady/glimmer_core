$ErrorActionPreference = "Stop"
. "$PSScriptRoot\env.ps1"

$requiredCommands = @("cmake", "ninja", "arm-none-eabi-gcc", "arm-none-eabi-gdb", "openocd")
$missing = @()

foreach ($name in $requiredCommands) {
    $command = Get-Command $name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        $missing += $name
    } else {
        Write-Host ("{0}: {1}" -f $name, $command.Source)
    }
}

if ($missing.Count -gt 0) {
    throw ("Missing required tools: " + ($missing -join ", "))
}

$openOcd = Get-Command openocd -ErrorAction Stop
$openOcdRoot = Split-Path (Split-Path $openOcd.Source -Parent) -Parent
$openOcdTarget = Join-Path $openOcdRoot "share\openocd\scripts\target\stm32f1x.cfg"
if (-not (Test-Path $openOcdTarget)) { throw "OpenOCD target configuration not found: $openOcdTarget" }

Write-Host "Toolchain check passed. Install the VS Code extensions CMake Tools, C/C++, and Cortex-Debug for the integrated workflow."
