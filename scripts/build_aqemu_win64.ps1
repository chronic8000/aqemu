#Requires -Version 5.1
<#
.SYNOPSIS
  Build x86_64 AQEMU from the MSYS2 UCRT64 environment.

.PARAMETER MsysRoot
  MSYS2 install root. Empty uses the active shell, then the registered MSYS2 install.
.PARAMETER BuildMsix
  Package the x64 build with installer/build-msix.ps1.
.PARAMETER SkipSpice
  Do not enable the SPICE client even when the library is installed.
.PARAMETER InstallDeps
  Install the UCRT64 toolchain and Qt packages before building.
#>
[CmdletBinding()]
param(
    [string] $MsysRoot = "",
    [switch] $BuildMsix,
    [switch] $SkipSpice,
    [switch] $InstallDeps
)

$ErrorActionPreference = "Stop"
$scriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
$RepoRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path
. (Join-Path $scriptDir "msys_prefix.ps1")
if ([string]::IsNullOrWhiteSpace($MsysRoot)) {
    $MsysRoot = Get-AqemuMsysRoot -Environment "ucrt64"
}
if (-not $MsysRoot) {
    Write-Error "MSYS2 was not found. Install it, or set MSYS2_ROOT to the install directory."
}
$bash = Join-Path $MsysRoot "usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $bash)) {
    Write-Error "MSYS2 bash is not at $bash"
}

$prefixBin = Join-Path $MsysRoot "ucrt64\bin"
if (-not $InstallDeps) {
    $haveCc = Test-Path -LiteralPath (Join-Path $prefixBin "cc.exe")
    $haveQt = Test-Path -LiteralPath (Join-Path $prefixBin "Qt5Core.dll")
    if (-not ($haveCc -and $haveQt)) { $InstallDeps = $true }
}

$env:MSYSTEM = "UCRT64"
$env:CHERE_INVOKING = "1"
if ($SkipSpice) { $env:AQEMU_SKIP_SPICE = "1" } else { Remove-Item Env:AQEMU_SKIP_SPICE -ErrorAction SilentlyContinue }

$unixRoot = (& $bash -lc "cygpath -u '$($RepoRoot.Replace('\','/'))'").Trim()
$depArg = ""
if ($InstallDeps) { $depArg = " --deps" }
Write-Host "x86_64 build using MSYSTEM=$env:MSYSTEM root=$MsysRoot"
& $bash -lc "cd '$unixRoot' && ./scripts/build_aqemu_win64.sh$depArg"
if ($LASTEXITCODE -ne 0) { Write-Error "x86_64 build failed with exit code $LASTEXITCODE" }

if ($BuildMsix) {
    $msixScript = Join-Path $RepoRoot "installer\build-msix.ps1"
    $buildDir = Join-Path $RepoRoot "build_win"
    & powershell -NoProfile -ExecutionPolicy Bypass -File $msixScript -Architecture "x64" -BuildDir $buildDir
}
