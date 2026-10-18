# MSYS2 root and bin directories.
# MSYSTEM_PREFIX looks like D:\msys64\ucrt64. MSYS2_ROOT is the root itself.
# C:\msys64 is only the last resort, and only when that directory exists.

function Get-AqemuMsysRoot {
    if ($env:MSYSTEM_PREFIX -and (Test-Path $env:MSYSTEM_PREFIX)) {
        return (Split-Path -Parent $env:MSYSTEM_PREFIX)
    }
    if ($env:MSYS2_ROOT -and (Test-Path $env:MSYS2_ROOT)) {
        return $env:MSYS2_ROOT
    }
    if (Test-Path "C:\msys64") {
        return "C:\msys64"
    }
    return $null
}

function Get-AqemuMsysBinDirs {
    $dirs = @()
    foreach ($prefix in @($env:MSYSTEM_PREFIX, $env:MINGW_PREFIX)) {
        if (-not $prefix) { continue }
        $bin = Join-Path $prefix "bin"
        if ((Test-Path $bin) -and ($dirs -notcontains $bin)) {
            $dirs += $bin
        }
    }
    $root = Get-AqemuMsysRoot
    if ($root) {
        foreach ($sub in @("clangarm64\bin", "clang64\bin", "ucrt64\bin", "mingw64\bin")) {
            $cand = Join-Path $root $sub
            if ((Test-Path $cand) -and ($dirs -notcontains $cand)) {
                $dirs += $cand
            }
        }
    }
    return $dirs
}

function Get-AqemuMsysShareDirs {
    $dirs = @()
    $root = Get-AqemuMsysRoot
    if (-not $root) { return $dirs }
    foreach ($sub in @("ucrt64", "clang64", "clangarm64", "mingw64")) {
        $cand = Join-Path $root (Join-Path $sub "share\qemu")
        if ((Test-Path $cand) -and ($dirs -notcontains $cand)) {
            $dirs += $cand
        }
    }
    return $dirs
}
