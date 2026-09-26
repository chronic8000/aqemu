#Requires -Version 5.1
<#
.SYNOPSIS
  Build AQEMU for Windows x86_64 (Intel / AMD 64-bit).

.DESCRIPTION
  Builds native x64 Windows binaries using WinLibs UCRT or MSYS2 MinGW64/UCRT64.
  Supports creating MSIX packages via -BuildMsix.

.PARAMETER MsysRoot
  Path to MSYS2 installation (default: C:\msys64).
.PARAMETER QtDir
  Path to Qt 5 MinGW build (optional if using MSYS2 Qt).
.PARAMETER BuildDir
  Output build directory (default: build_win).
.PARAMETER BuildMsix
  If set, packages the built x64 payload into an MSIX for Windows / Store.
.PARAMETER SkipSpice
  Disable embedded SPICE display client.
#>
[CmdletBinding()]
param(
    [string] $MsysRoot = "C:\msys64",
    [string] $QtDir = "",
    [string] $BuildDir = "build_win",
    [switch] $BuildMsix,
    [switch] $SkipSpice
)

$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
$RepoRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path
$FullBuildDir = Join-Path $RepoRoot $BuildDir

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "  AQEMU - Windows x86_64 (64-bit) Build Script              " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Repository Root: $RepoRoot"
Write-Host "Build Directory: $FullBuildDir"

# Probe MSYS2 ucrt64 or mingw64
$msysUcrtBin = Join-Path $MsysRoot "ucrt64\bin"
$msysMingwBin = Join-Path $MsysRoot "mingw64\bin"

if (Test-Path $msysUcrtBin) {
    $env:PATH = "$msysUcrtBin;$MsysRoot\usr\bin;$env:PATH"
    $env:PKG_CONFIG_PATH = (Join-Path $MsysRoot "ucrt64\lib\pkgconfig")
    Write-Host "Using MSYS2 UCRT64 toolchain from $msysUcrtBin" -ForegroundColor Green
} elseif (Test-Path $msysMingwBin) {
    $env:PATH = "$msysMingwBin;$MsysRoot\usr\bin;$env:PATH"
    $env:PKG_CONFIG_PATH = (Join-Path $MsysRoot "mingw64\lib\pkgconfig")
    Write-Host "Using MSYS2 MinGW64 toolchain from $msysMingwBin" -ForegroundColor Green
}

# Check for cmake and ninja
$cmakeCmd = Get-Command "cmake" -ErrorAction SilentlyContinue
$ninjaCmd = Get-Command "ninja" -ErrorAction SilentlyContinue

if (-not $cmakeCmd) {
    Write-Error "cmake is not found on PATH."
}
if (-not $ninjaCmd) {
    Write-Error "ninja is not found on PATH."
}

if (-not (Test-Path $FullBuildDir)) {
    New-Item -ItemType Directory -Path $FullBuildDir -Force | Out-Null
}

$cmakeArgs = @(
    "-G", "Ninja",
    "-B", $FullBuildDir,
    "-S", $RepoRoot,
    "-DCMAKE_BUILD_TYPE=Release"
)

if ($QtDir -and (Test-Path $QtDir)) {
    $cmakeArgs += "-DCMAKE_PREFIX_PATH=$QtDir"
}

if (-not $SkipSpice) {
    $cmakeArgs += "-DAQEMU_WITH_SPICE_GTK=ON"
}

$qemuInstall = Join-Path $RepoRoot "third_party\qemu-install"
if (Test-Path (Join-Path $qemuInstall "bin")) {
    $cmakeArgs += "-DAQEMU_BUNDLE_QEMU=ON"
    $cmakeArgs += "-DAQEMU_QEMU_PREFIX=$qemuInstall"
    Write-Host "Bundling QEMU binaries from $qemuInstall" -ForegroundColor Green
}

Write-Host "`nRunning CMake configuration..." -ForegroundColor Cyan
& cmake.exe @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed."
}

Write-Host "`nCompiling AQEMU executable..." -ForegroundColor Cyan
& ninja.exe -C $FullBuildDir
if ($LASTEXITCODE -ne 0) {
    Write-Error "Ninja compilation failed."
}

$targetExe = Join-Path $FullBuildDir "aqemu.exe"
if (-not (Test-Path $targetExe)) {
    Write-Error "Build finished but aqemu.exe was not found at $targetExe."
}

Write-Host "`n=== Build Succeeded! ===" -ForegroundColor Green
Write-Host "Executable: $targetExe"

# Deploy Qt dependencies
$wdq = Get-Command "windeployqt" -ErrorAction SilentlyContinue
if ($wdq) {
    Write-Host "Deploying Qt runtime libraries with windeployqt..." -ForegroundColor Cyan
    & $wdq.Path --no-translations --compiler-runtime $targetExe
}

if ($BuildMsix) {
    Write-Host "`n=== Packaging into MSIX ===" -ForegroundColor Cyan
    $msixScript = Join-Path $RepoRoot "installer\build-msix.ps1"
    if (Test-Path $msixScript) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $msixScript -Architecture "x64" -BuildDir $FullBuildDir
    }
}

Write-Host "`nTo run AQEMU:" -ForegroundColor Green
Write-Host "  & `"$targetExe`""
