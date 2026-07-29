#Requires -Version 5.1
<#
.SYNOPSIS
  Stage build_win payload and build AQEMU-*-win64.msix (Desktop Bridge / runFullTrust).

.NOTES
  Use this package for Microsoft Store paid submissions (Store commerce / price tiers).
  Keep the MSI for website / Stripe sales.
#>
[CmdletBinding()]
param(
    [string] $RepoRoot = "",
    [string] $BuildDir = "",
    [string] $Version = "1.1.0.0",
    [string] $OutDir = "",
    # Must match Partner Center Product identity Publisher (CN=...)
    [string] $Publisher = "CN=16318CB3-C262-4B44-BCCF-310B0DDA3950",
    # Must match Partner Center Product identity Package/Identity name
    [string] $IdentityName = "30932PhilipSempers.AQEMUVMManager",
    # Must match Partner Center publisher display name (account name)
    [string] $PublisherDisplayName = "Philip Sempers",
    [switch] $SkipSign,
    # Optional: plaintext for local test signing only. Prefer env AQEMU_MSIX_PFX_PASSWORD.
    [string] $PfxPassword = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($scriptDir)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
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

# MSIX version must be four-part
if ($Version -notmatch '^\d+\.\d+\.\d+\.\d+$') {
    if ($Version -match '^(\d+)\.(\d+)\.(\d+)$') {
        $Version = "$Version.0"
    } else {
        Write-Error "Version must be like 1.0.0.0 (got '$Version')"
    }
}

$msixSrc = Join-Path $RepoRoot "installer\msix"
$layoutDir = Join-Path $RepoRoot "installer\msix_layout"
$msixName = "AQEMU-$Version-win64.msix"
if ($Version -match '^(\d+\.\d+\.\d+)\.0$') {
    $msixName = "AQEMU-$($Matches[1])-win64.msix"
}
$msixPath = Join-Path $OutDir $msixName
$certDir = Join-Path $RepoRoot "installer\certs"
$pfxPath = Join-Path $certDir "aqemu-msix-test.pfx"
$cerPath = Join-Path $certDir "aqemu-msix-test.cer"
if ([string]::IsNullOrWhiteSpace($PfxPassword)) {
    $PfxPassword = $env:AQEMU_MSIX_PFX_PASSWORD
}
if ([string]::IsNullOrWhiteSpace($PfxPassword)) {
    # Dev-only fallback for local self-signed test certs (never echoed)
    $PfxPassword = "aqemu-msix-dev"
}

function Find-SdkTool([string] $name) {
    $patterns = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\$name",
        "${env:ProgramFiles}\Windows Kits\10\bin\*\x64\$name"
    )
    foreach ($p in $patterns) {
        $hit = Get-Item $p -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
        if ($hit) { return $hit.FullName }
    }
    return $null
}

$makeappx = Find-SdkTool "makeappx.exe"
$signtool = Find-SdkTool "signtool.exe"
if (-not $makeappx) {
    Write-Error "makeappx.exe not found. Install Windows 10/11 SDK."
}

$aqemu = Join-Path $BuildDir "aqemu.exe"
if (-not (Test-Path $aqemu)) {
    Write-Error "Missing $aqemu - build AQEMU into build_win first."
}

$splashSrc = Join-Path $msixSrc "Assets\SplashScreen.png"
if (-not (Test-Path $splashSrc)) {
    Add-Type -AssemblyName System.Drawing
    $logo = [System.Drawing.Image]::FromFile((Join-Path $RepoRoot "resources\icons\aqemu_logo.png"))
    $bmp = New-Object System.Drawing.Bitmap 620, 300, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::FromArgb(255, 24, 24, 28))
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $side = 220
    $g.DrawImage($logo, [float]((620 - $side) / 2), [float]((300 - $side) / 2), [float]$side, [float]$side)
    $g.Dispose(); $logo.Dispose()
    $bmp.Save($splashSrc, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

Write-Host "MakeAppx: $makeappx"
Write-Host "Payload:  $BuildDir"
Write-Host "Layout:   $layoutDir"
Write-Host "Identity: $IdentityName"
Write-Host "Publisher:$Publisher"
Write-Host "PublisherDisplayName: $PublisherDisplayName"
Write-Host "Version:  $Version"
Write-Host "Output:   $msixPath"

if (Test-Path $layoutDir) {
    Remove-Item $layoutDir -Recurse -Force
}
New-Item -ItemType Directory -Path $layoutDir | Out-Null

Write-Host "Staging files into MSIX layout..."
$roboArgs = @(
    $BuildDir, $layoutDir, "/E", "/NFL", "/NDL", "/NJH", "/NJS", "/nc", "/ns", "/np",
    "/XD", "CMakeFiles", "aqemu_autogen", "Testing", "CMakeTmp",
    "/XF", "*.cmake", "CMakeCache.txt", "Makefile", "*.log",
    "aqemu_err*.txt", "aqemu_out.txt", "qerr.txt", "qout.txt",
    "*.obj", "*.o", "*.a", "*.rsp", "*.d"
)
& robocopy @roboArgs | Out-Null
if ($LASTEXITCODE -ge 8) {
    Write-Error "robocopy failed with exit code $LASTEXITCODE"
}

# Ensure QEMU firmware share/ is present (required for Store embedded sessions).
$shareBios = Join-Path $layoutDir "share\bios-256k.bin"
if (-not (Test-Path $shareBios)) {
    $qemuPrefix = Join-Path $RepoRoot "third_party\qemu-install"
    $shareSrc = $null
    foreach ($cand in @(
        (Join-Path $qemuPrefix "share"),
        (Join-Path $qemuPrefix "share\qemu"),
        (Join-Path $BuildDir "share")
    )) {
        if (Test-Path (Join-Path $cand "bios-256k.bin")) {
            $shareSrc = $cand
            break
        }
    }
    if (-not $shareSrc) {
        Write-Error "MSIX layout is missing share\bios-256k.bin. Rebuild/bundle QEMU with firmware (scripts/build_qemu_windows_msys.sh + -DAQEMU_BUNDLE_QEMU=ON)."
    }
    Write-Host "Copying QEMU firmware share from $shareSrc ..."
    New-Item -ItemType Directory -Path (Join-Path $layoutDir "share") -Force | Out-Null
    & robocopy $shareSrc (Join-Path $layoutDir "share") /E /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
    if ($LASTEXITCODE -ge 8 -or -not (Test-Path $shareBios)) {
        Write-Error "Failed to stage QEMU share/ firmware into MSIX layout."
    }
}

$qemuSystems = @(Get-ChildItem (Join-Path $layoutDir "qemu-system-*.exe") -ErrorAction SilentlyContinue)
Write-Host ("Staged qemu-system-* count: {0}" -f $qemuSystems.Count)
if ($qemuSystems.Count -lt 10) {
    Write-Warning ("Only {0} qemu-system-* binaries staged. Store packages should include EVERY softmmu target - rebuild with scripts/build_qemu_windows_msys.sh (all targets)." -f $qemuSystems.Count)
}

if (-not (Test-Path (Join-Path $layoutDir "aqemu.exe"))) {
    Write-Error "Staging failed - aqemu.exe not in layout."
}
$fileCount = (Get-ChildItem $layoutDir -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count
Write-Host "Staged $fileCount files."

Copy-Item (Join-Path $msixSrc "Assets") (Join-Path $layoutDir "Assets") -Recurse -Force

$template = Get-Content (Join-Path $msixSrc "AppxManifest.xml.template") -Raw
$manifest = $template.
    Replace("__IDENTITY_NAME__", $IdentityName).
    Replace("__PUBLISHER__", $Publisher).
    Replace("__PUBLISHER_DISPLAY_NAME__", $PublisherDisplayName).
    Replace("__VERSION__", $Version)
$manifestPath = Join-Path $layoutDir "AppxManifest.xml"
$utf8NoBom = New-Object System.Text.UTF8Encoding $false
[System.IO.File]::WriteAllText($manifestPath, $manifest, $utf8NoBom)

New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
if (Test-Path $msixPath) { Remove-Item $msixPath -Force }

Write-Host "Packing MSIX (this can take a few minutes)..."
& $makeappx pack /d $layoutDir /p $msixPath /o
if ($LASTEXITCODE -ne 0) {
    Write-Error "makeappx failed with exit code $LASTEXITCODE"
}

if (-not $SkipSign) {
    if (-not $signtool) {
        Write-Warning "signtool.exe not found - leaving package unsigned."
    }
    else {
        New-Item -ItemType Directory -Path $certDir -Force | Out-Null
        if (-not (Test-Path $pfxPath)) {
            Write-Host "Creating self-signed test certificate ($Publisher)..."
            # Avoid {text} brace parsing issues: build extension strings with format
            $ekuCodeSigning = '2.5.29.37=' + '{text}' + '1.3.6.1.5.5.7.3.3'
            $basicConstraints = '2.5.29.19=' + '{text}'
            $cert = New-SelfSignedCertificate `
                -Type Custom `
                -Subject $Publisher `
                -KeyUsage DigitalSignature `
                -FriendlyName "AQEMU MSIX Test" `
                -CertStoreLocation "Cert:\CurrentUser\My" `
                -TextExtension @($ekuCodeSigning, $basicConstraints)
            $secure = ConvertTo-SecureString -String $PfxPassword -Force -AsPlainText
            Export-PfxCertificate -Cert $cert -FilePath $pfxPath -Password $secure | Out-Null
            Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null
            Write-Host "Test PFX: $pfxPath (password not printed - use -PfxPassword or AQEMU_MSIX_PFX_PASSWORD)"
            Write-Host "Install $cerPath into Trusted People for local sideload testing."
        }

        Write-Host "Signing..."
        & $signtool sign /fd SHA256 /a /f $pfxPath /p $PfxPassword $msixPath
        if ($LASTEXITCODE -ne 0) {
            Write-Error "signtool failed with exit code $LASTEXITCODE"
        }
    }
}

$item = Get-Item $msixPath
$mb = [math]::Round($item.Length / 1MB, 1)
$okMsg = 'OK: ' + $item.FullName + ' (' + $mb.ToString() + ' MB)'
Write-Host $okMsg
Write-Host ''
Write-Host 'Store submission: upload this MSIX package to Microsoft Partner Center.'
