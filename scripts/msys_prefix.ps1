# MSYS2 root and bin directories.
# Prefer the environment already selected by the MSYS2 shell, then the
# install location recorded for MSYS2. Nothing here assumes a drive letter.

function Get-AqemuMsysCandidates {
    $cands = @()
    if ($env:MSYSTEM_PREFIX -and (Test-Path -LiteralPath $env:MSYSTEM_PREFIX)) {
        $cands += (Split-Path -Parent $env:MSYSTEM_PREFIX)
    }
    if ($env:MSYS2_ROOT) {
        $cands += $env:MSYS2_ROOT
    }
    $hives = @(
        'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*',
        'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*'
    )
    foreach ($hive in $hives) {
        Get-ItemProperty $hive -ErrorAction SilentlyContinue |
            Where-Object { $_.DisplayName -eq 'MSYS2' -and $_.InstallLocation } |
            ForEach-Object { $cands += $_.InstallLocation.TrimEnd('\') }
    }
    $bash = Get-Command bash.exe -ErrorAction SilentlyContinue
    if ($bash -and $bash.Source -match '\\usr\\bin\\bash\.exe$') {
        $cands += (Split-Path -Parent (Split-Path -Parent $bash.Source))
    }
    $seen = @{}
    foreach ($cand in $cands) {
        if (-not $cand) { continue }
        $root = $cand.TrimEnd('\')
        if ($seen.ContainsKey($root)) { continue }
        if (Test-Path -LiteralPath (Join-Path $root 'usr\bin\bash.exe')) {
            $seen[$root] = $true
            $root
        }
    }
}

function Get-AqemuMsysRoot {
    param(
        # MSYS2 environment directory name, for example clangarm64 or ucrt64.
        [string] $Environment = ""
    )
    $roots = @(Get-AqemuMsysCandidates)
    if ($Environment) {
        foreach ($root in $roots) {
            $bin = Join-Path $root (Join-Path $Environment 'bin')
            if (Test-Path -LiteralPath $bin) { return $root }
        }
        foreach ($root in $roots) {
            if (Test-Path -LiteralPath (Join-Path $root $Environment)) { return $root }
        }
    }
    if ($roots.Count -gt 0) { return $roots[0] }
    return $null
}

function Get-AqemuMsysBinDirs {
    $dirs = @()
    foreach ($prefix in @($env:MSYSTEM_PREFIX, $env:MINGW_PREFIX)) {
        if (-not $prefix) { continue }
        $bin = Join-Path $prefix "bin"
        if ((Test-Path -LiteralPath $bin) -and ($dirs -notcontains $bin)) {
            $dirs += $bin
        }
    }
    $root = Get-AqemuMsysRoot
    if ($root) {
        Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            $bin = Join-Path $_.FullName 'bin'
            $cc = Join-Path $bin 'cc.exe'
            if ((Test-Path -LiteralPath $cc) -and ($dirs -notcontains $bin)) {
                $dirs += $bin
            }
        }
    }
    return $dirs
}

function Get-AqemuMsysShareDirs {
    $dirs = @()
    $root = Get-AqemuMsysRoot
    if (-not $root) { return $dirs }
    Get-ChildItem -LiteralPath $root -Directory -ErrorAction SilentlyContinue | ForEach-Object {
        $cand = Join-Path $_.FullName 'share\qemu'
        if ((Test-Path -LiteralPath $cand) -and ($dirs -notcontains $cand)) {
            $dirs += $cand
        }
    }
    return $dirs
}
