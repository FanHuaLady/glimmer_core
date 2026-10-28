$ErrorActionPreference = "Stop"
. "$PSScriptRoot\env.ps1"

foreach ($tool in @("cmake", "ninja", "arm-none-eabi-gcc", "arm-none-eabi-objcopy", "openocd")) {
    $command = Get-Command $tool -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        throw "Required tool not found: $tool"
    }
    Write-Host ("{0}: {1}" -f $tool, $command.Source)
}
