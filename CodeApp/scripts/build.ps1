$ErrorActionPreference = "Stop"
. "$PSScriptRoot\env.ps1"
Push-Location (Join-Path $PSScriptRoot "..")
try {
    cmake --preset stm32f103-debug
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    cmake --build --preset stm32f103-debug
    if ($LASTEXITCODE -ne 0) { throw "Application build failed." }
} finally {
    Pop-Location
}
