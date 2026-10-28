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
    if (-not (Test-Path $toolBin)) {
        continue
    }

    $alreadyOnPath = $false
    foreach ($pathPart in ($env:Path -split ';')) {
        if ($pathPart.TrimEnd('\') -ieq $toolBin.TrimEnd('\')) {
            $alreadyOnPath = $true
            break
        }
    }

    if (-not $alreadyOnPath) {
        $env:Path = "$toolBin;$env:Path"
    }
}
