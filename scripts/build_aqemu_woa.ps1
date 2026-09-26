#Requires -Version 5.1
<#
.SYNOPSIS
  Build AQEMU for Windows on ARM (WoA) / Snapdragon X Elite & Snapdragon 8cx laptops.

.DESCRIPTION
  Builds native ARM64 Windows binaries using MSYS2 CLANGARM64 or LLVM-MinGW.
  Supports creating native Store MSIX packages via -BuildMsix.

.PARAMETER MsysRoot
  Path to MSYS2 installation (default: C:\msys64).
.PARAMETER BuildDir
  Output build directory (default: build_woa).
.PARAMETER BuildMsix
  If set, packages the built ARM64 payload into an MSIX for Windows on ARM / Store.
.PARAMETER SkipSpice
  Disable embedded SPICE display client.
#>
[CmdletBinding()]
param(
    [string] $MsysRoot = "C:\msys64",
    [string] $BuildDir = "build_woa",
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
Write-Host "  AQEMU - Windows on ARM (WoA / Snapdragon) Build Script    " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Host Architecture: $env:PROCESSOR_ARCHITECTURE"
Write-Host "Repository Root:   $RepoRoot"
Write-Host "Build Directory:   $FullBuildDir"

# Verify MSYS2 CLANGARM64 environment
$clangarm64Bin = Join-Path $MsysRoot "clangarm64\bin"
$clangarm64Pkg = Join-Path $MsysRoot "clangarm64\lib\pkgconfig"

if (-not (Test-Path $clangarm64Bin)) {
    Write-Warning "Could not find MSYS2 CLANGARM64 environment at $clangarm64Bin"
    Write-Host "To set up native Windows on ARM builds in MSYS2 on your Snapdragon laptop:" -ForegroundColor Yellow
    Write-Host "  1. Install MSYS2 from https://www.msys2.org/"
    Write-Host "  2. Open the 'MSYS2 CLANGARM64' terminal"
    Write-Host "  3. Run:"
    Write-Host "     pacman -S --needed mingw-w64-clang-aarch64-toolchain \"
    Write-Host "       mingw-w64-clang-aarch64-qt5-base mingw-w64-clang-aarch64-cmake \"
    Write-Host "       mingw-w64-clang-aarch64-ninja mingw-w64-clang-aarch64-pkgconf \"
    Write-Host "       mingw-w64-clang-aarch64-spice-gtk mingw-w64-clang-aarch64-libvncserver \"
    Write-Host "       mingw-w64-clang-aarch64-libslirp mingw-w64-clang-aarch64-libusb"
    Write-Host ""
}

# Add CLANGARM64 tools to PATH for this session
if (Test-Path $clangarm64Bin) {
    $env:PATH = "$clangarm64Bin;$MsysRoot\usr\bin;$env:PATH"
    $env:PKG_CONFIG_PATH = $clangarm64Pkg
    Write-Host "Using CLANGARM64 toolchain from $clangarm64Bin" -ForegroundColor Green
}

# Check for cmake and ninja
$cmakeCmd = Get-Command "cmake" -ErrorAction SilentlyContinue
$ninjaCmd = Get-Command "ninja" -ErrorAction SilentlyContinue

if (-not $cmakeCmd) {
    Write-Error "cmake is not found on PATH. Ensure MSYS2 CLANGARM64 or CMake ARM64 is installed."
}
if (-not $ninjaCmd) {
    Write-Error "ninja is not found on PATH. Install ninja in MSYS2 (pacman -S mingw-w64-clang-aarch64-ninja)."
}

if (-not (Test-Path $FullBuildDir)) {
    New-Item -ItemType Directory -Path $FullBuildDir -Force | Out-Null
}

$cmakeArgs = @(
    "-G", "Ninja",
    "-B", $FullBuildDir,
    "-S", $RepoRoot,
    "-DCMAKE_BUILD_TYPE=Release",
    "-DWIN_ARM64_OPTIMIZATIONS=ON"
)

if (-not $SkipSpice) {
    $cmakeArgs += "-DAQEMU_WITH_SPICE_GTK=ON"
} else {
    Write-Host "SPICE client disabled by -SkipSpice"
}

# If QEMU ARM64 bundle prefix exists, enable bundling
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

Write-Host "`nCompiling native ARM64 AQEMU executable..." -ForegroundColor Cyan
& ninja.exe -C $FullBuildDir
if ($LASTEXITCODE -ne 0) {
    Write-Error "Ninja compilation failed."
}

$targetExe = Join-Path $FullBuildDir "aqemu.exe"
if (-not (Test-Path $targetExe)) {
    Write-Error "Build finished but aqemu.exe was not found at $targetExe."
}

Write-Host "`n=== WoA Build Succeeded! ===" -ForegroundColor Green
Write-Host "Executable: $targetExe"

# Automatically run windeployqt if available to gather Qt ARM64 plugins/DLLs
$wdq = Get-Command "windeployqt" -ErrorAction SilentlyContinue
if ($wdq) {
    Write-Host "Deploying Qt runtime libraries with windeployqt..." -ForegroundColor Cyan
    & $wdq.Path --no-translations --compiler-runtime $targetExe
}

# Build MSIX package if requested
if ($BuildMsix) {
    Write-Host "`n=== Packaging into Windows on ARM MSIX ===" -ForegroundColor Cyan
    $msixScript = Join-Path $RepoRoot "installer\build-msix.ps1"
    if (Test-Path $msixScript) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $msixScript -Architecture "arm64" -BuildDir $FullBuildDir
    } else {
        Write-Warning "Could not find $msixScript"
    }
}

Write-Host "`nTo run AQEMU on your Snapdragon PC:" -ForegroundColor Green
Write-Host "  & `"$targetExe`""
