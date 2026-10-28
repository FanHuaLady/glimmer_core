$ErrorActionPreference = "Stop"
. "$PSScriptRoot\env.ps1"

Push-Location (Join-Path $PSScriptRoot "..")
try {
    cmake --preset stm32f103-debug
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configure failed with exit code $LASTEXITCODE"
    }

    cmake --build --preset stm32f103-debug
    if ($LASTEXITCODE -ne 0) {
        throw "Firmware build failed with exit code $LASTEXITCODE"
    }
} finally {
    Pop-Location
}
