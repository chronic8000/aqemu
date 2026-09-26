#Requires -Version 5.1
<#
.SYNOPSIS
  Build bundled QEMU on Windows on ARM (WoA) / Qualcomm Snapdragon laptops.

.DESCRIPTION
  Launches MSYS2 CLANGARM64 to build native ARM64 QEMU binaries.

.PARAMETER Target
  Target architecture to build (default: "all", or e.g. "aarch64", "x86_64").
.PARAMETER MsysRoot
  Path to MSYS2 installation (default: C:\msys64).
.PARAMETER Prefix
  Installation output directory.
#>
[CmdletBinding()]
param(
    [string] $Target = "all",
    [string] $MsysRoot = "C:\msys64",
    [string] $Prefix = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
$RepoRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path

if ([string]::IsNullOrWhiteSpace($Prefix)) {
    $Prefix = Join-Path $RepoRoot "third_party\qemu-install"
}

$bash = Join-Path $MsysRoot "usr\bin\bash.exe"
if (-not (Test-Path $bash)) {
    Write-Error "MSYS2 bash not found at $bash. Please install MSYS2 from https://www.msys2.org/"
}

$unixScript = "/$(($RepoRoot -replace '\\', '/').TrimStart('/') -replace ':', '')/scripts/build_qemu_win_arm64.sh"
$unixPrefix = "/$(($Prefix -replace '\\', '/').TrimStart('/') -replace ':', '')"

Write-Host "=== Launching QEMU Windows on ARM Build ===" -ForegroundColor Cyan
Write-Host "Target:  $Target"
Write-Host "Prefix:  $Prefix"

$cmd = "& `"$bash`" -lc `"MSYSTEM=CLANGARM64 bash '$unixScript' '$Target' '$unixPrefix'`""
Invoke-Expression $cmd
