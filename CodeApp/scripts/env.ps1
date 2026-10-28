$toolBins = @(
    $env:ARM_NONE_EABI_BIN,
    $env:OPENOCD_BIN,
    $env:NINJA_BIN,
    "D:\MyCmake\bin",
    "D:\MyArmGunToolchainArmNoneEabi\bin",
    "D:\MyNinja",
    "D:\MyOpenocd\bin"
) | Where-Object { $_ }

foreach ($toolBin in $toolBins) {
    if (-not (Test-Path $toolBin)) { continue }
    if (-not (($env:Path -split ';') | Where-Object {
        $_.TrimEnd('\') -ieq $toolBin.TrimEnd('\')
    })) {
        $env:Path = "$toolBin;$env:Path"
    }
}
