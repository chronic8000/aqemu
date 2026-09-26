#Requires -Version 5.1
<#
.SYNOPSIS
  Build bundled QEMU on Windows x86_64 (MSYS2 UCRT64 / MinGW64).

.DESCRIPTION
  Launches MSYS2 to build native x86_64 QEMU binaries.

.PARAMETER Target
  Target architecture to build (default: "all", or e.g. "x86_64", "aarch64").
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

$unixScript = "/$(($RepoRoot -replace '\\', '/').TrimStart('/') -replace ':', '')/scripts/build_qemu_win_x86_64.sh"
$unixPrefix = "/$(($Prefix -replace '\\', '/').TrimStart('/') -replace ':', '')"

Write-Host "=== Launching QEMU Windows x86_64 Build ===" -ForegroundColor Cyan
Write-Host "Target:  $Target"
Write-Host "Prefix:  $Prefix"

$cmd = "& `"$bash`" -lc `"bash '$unixScript' '$Target' '$unixPrefix'`""
Invoke-Expression $cmd
