#Requires -Version 5.1
<#
.SYNOPSIS
  Stage build_win payload and build AQEMU-*-win64.msi with WiX CLI.

.NOTES
  InstallShield is not used (commercial / not installed). This uses WiX Toolset.
#>
[CmdletBinding()]
param(
    [string] $RepoRoot = "",
    [string] $BuildDir = "",
    [string] $Version = "1.3.0",
    [string] $OutDir = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Get-Location | Select-Object -ExpandProperty Path
}

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path $scriptDir "..")).Path
}
if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build_win"
}
if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "installer\out"
}

$wixDir = Join-Path $RepoRoot "installer\wix"
$stageDir = Join-Path $RepoRoot "installer\payload"
$msiName = "AQEMU-$Version-win64.msi"
$msiPath = Join-Path $OutDir $msiName

function Find-Wix {
    $cmd = Get-Command wix -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        "$env:LOCALAPPDATA\Microsoft\WinGet\Links\wix.exe",
        "$env:ProgramFiles\WiX Toolset *\bin\wix.exe",
        "${env:ProgramFiles(x86)}\WiX Toolset *\bin\wix.exe"
    )
    foreach ($pattern in $candidates) {
        $hit = Get-Item $pattern -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    return $null
}

$wix = Find-Wix
if (-not $wix) {
    Write-Error @"
WiX CLI ('wix') not found.
Install with:  winget install --id WiXToolset.WiXCLI -e
Then re-open the terminal and run this script again.
"@
}

$aqemu = Join-Path $BuildDir "aqemu.exe"
if (-not (Test-Path $aqemu)) {
    Write-Error "Missing $aqemu — build AQEMU into build_win first."
}

Write-Host "WiX:      $wix"
Write-Host "Payload:  $BuildDir"
Write-Host "Staging:  $stageDir"
Write-Host "Output:   $msiPath"

# Clean / stage payload (skip CMake junk)
if (Test-Path $stageDir) {
    Remove-Item $stageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $stageDir | Out-Null

$excludeDirNames = @(
    "CMakeFiles", "aqemu_autogen", "Testing", "CMakeTmp"
)
$excludeFilePatterns = @(
    "*.cmake", "CMakeCache.txt", "Makefile", "*.log",
    "aqemu_err*.txt", "aqemu_out.txt", "qerr.txt", "qout.txt"
)

Write-Host "Staging files..."
Get-ChildItem $BuildDir -Force | ForEach-Object {
    if ($_.PSIsContainer) {
        if ($excludeDirNames -contains $_.Name) { return }
        Copy-Item $_.FullName (Join-Path $stageDir $_.Name) -Recurse -Force
    }
    else {
        foreach ($pat in $excludeFilePatterns) {
            if ($_.Name -like $pat) { return }
        }
        Copy-Item $_.FullName (Join-Path $stageDir $_.Name) -Force
    }
}

if (-not (Test-Path (Join-Path $stageDir "aqemu.exe"))) {
    Write-Error "Staging failed — aqemu.exe not in payload."
}

New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

# WiX v7 requires OSMF EULA acceptance (one-time per user, or -acceptEula per build)
$eulaMarker = Join-Path $env:USERPROFILE ".wix\wix7-osmf-eula.txt"
if (-not (Test-Path $eulaMarker)) {
    Write-Host "Accepting WiX OSMF EULA (wix7)..."
    & $wix eula accept wix7
    if ($LASTEXITCODE -ne 0) {
        Write-Error "WiX EULA accept failed. See https://wixtoolset.org/osmf/"
    }
}

# Ensure UI extension is available (match installed WiX major)
Push-Location $wixDir
& $wix extension add WixToolset.UI.wixext 2>&1 | Out-Null
Pop-Location

Write-Host "Building MSI..."
$pkg = Join-Path $wixDir "Package.wxs"
$license = Join-Path $wixDir "License.rtf"

Push-Location $wixDir
try {
    & $wix build `
        -acceptEula wix7 `
        -o $msiPath `
        -arch x64 `
        -ext WixToolset.UI.wixext `
        -b "Payload=$stageDir" `
        -d "ProductVersion=$Version" `
        -d "LicenseRtf=$license" `
        $pkg
}
finally {
    Pop-Location
}

if ($LASTEXITCODE -ne 0) {
    Write-Error "wix build failed with exit code $LASTEXITCODE"
}

$item = Get-Item $msiPath
Write-Host ("OK: {0} ({1:N1} MB)" -f $item.FullName, ($item.Length / 1MB))
Write-Host "Silent install: msiexec /i `"$($item.FullName)`" /qn"
