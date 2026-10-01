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
    [string] $Version = "",
    [string] $OutDir = "",
    [string] $MsysLocation = ""
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

# Auto-detect version from single-source-of-truth VERSION file if not passed
if (-not $Version) {
    $versionFile = Join-Path $RepoRoot "VERSION.txt"
    if (Test-Path $versionFile) {
        $Version = (Get-Content $versionFile -Raw).Trim()
    } else {
        $Version = "1.5.0"
    }
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

# Resolve MSYS2 and runtime directories for transitive DLL bundling
$binSearchDirs = @()
if ($MsysLocation -and (Test-Path $MsysLocation)) {
    foreach ($sub in @("ucrt64\bin", "clangarm64\bin", "mingw64\bin", "bin")) {
        $cand = Join-Path $MsysLocation $sub
        if ((Test-Path $cand) -and ($binSearchDirs -notcontains $cand)) { $binSearchDirs += $cand }
    }
}
if ($env:MSYSTEM_PREFIX -and (Test-Path (Join-Path $env:MSYSTEM_PREFIX "bin"))) {
    $binSearchDirs += (Join-Path $env:MSYSTEM_PREFIX "bin")
}
foreach ($cand in @("C:\msys64\ucrt64\bin", "C:\msys64\clangarm64\bin", "C:\msys64\mingw64\bin")) {
    if ((Test-Path $cand) -and ($binSearchDirs -notcontains $cand)) { $binSearchDirs += $cand }
}
foreach ($p in ($env:PATH -split ";")) {
    if ($p -and (Test-Path $p) -and ($binSearchDirs -notcontains $p)) {
        if ((Test-Path (Join-Path $p "libwinpthread-1.dll")) -or (Test-Path (Join-Path $p "Qt5Core.dll"))) {
            $binSearchDirs += $p
        }
    }
}

# Explicit list of standard MinGW / GCC / LLVM / Qt / QEMU runtime DLLs
$essentialDlls = @(
    "libwinpthread-1.dll", "libgcc_s_seh-1.dll", "libgcc_s_dw2-1.dll", "libgcc_s_sjlj-1.dll",
    "libstdc++-6.dll", "libgomp-1.dll", "libunwind.dll", "libc++.dll",
    "Qt5Core.dll", "Qt5Gui.dll", "Qt5Widgets.dll", "Qt5Network.dll", "Qt5PrintSupport.dll", "Qt5Test.dll", "Qt5DBus.dll",
    "libdouble-conversion.dll", "libmd4c.dll", "libharfbuzz-0.dll",
    "libfreetype-6.dll", "libgraphite2.dll", "libpng16-16.dll", "libjpeg-8.dll",
    "zlib1.dll", "libzstd.dll", "libbz2-1.dll", "libpcre2-16-0.dll", "libpcre2-8-0.dll",
    "libvncclient.dll", "libvncclient-1.dll", "libvncserver.dll", "libvncserver-1.dll",
    "libgcrypt-20.dll", "libgpg-error-0.dll", "libgnutls-30.dll", "libnettle-8.dll", "libhogweed-6.dll", "libgmp-10.dll",
    "libtasn1-6.dll", "libidn2-0.dll", "libunistring-5.dll", "libunistring-2.dll",
    "libbrotlidec.dll", "libbrotlicommon.dll", "libbrotlienc.dll",
    "libglib-2.0-0.dll", "libgthread-2.0-0.dll", "libgobject-2.0-0.dll", "libgio-2.0-0.dll", "libgmodule-2.0-0.dll",
    "libintl-8.dll", "libiconv-2.dll", "libffi-8.dll", "libslirp-0.dll", "libusb-1.0.0.dll", "libusbredirparser-1.dll",
    "libpixman-1-0.dll", "libepoxy-0.dll", "libcurl-4.dll", "libnghttp2-14.dll", "libssh2-1.dll", "libfdt-1.dll"
)

foreach ($dll in $essentialDlls) {
    $dest = Join-Path $stageDir $dll
    if (-not (Test-Path $dest)) {
        foreach ($d in $binSearchDirs) {
            $src = Join-Path $d $dll
            if (Test-Path $src) {
                Copy-Item $src $dest -Force
                break
            }
        }
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
