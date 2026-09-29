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
    [string] $Version = "1.3.1.0",
    [string] $Architecture = "x64",
    [string] $OutDir = "",
    [string] $MsysLocation = "",
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

$arch = $Architecture.ToLowerInvariant().Trim()
if ($arch -eq "arm64" -or $arch -eq "aarch64") {
    $archTag = "win-arm64"
    $manifestArch = "arm64"
} else {
    $archTag = "win64"
    $manifestArch = "x64"
}

$msixSrc = Join-Path $RepoRoot "installer\msix"
$layoutDir = Join-Path $RepoRoot "installer\msix_layout"
$msixName = "AQEMU-$Version-$archTag.msix"
if ($Version -match '^(\d+\.\d+\.\d+)\.0$') {
    $msixName = "AQEMU-$($Matches[1])-$archTag.msix"
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
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd -and (Test-Path $cmd.Source)) { return $cmd.Source }

    $searchRoots = @(
        $env:ProgramFiles,
        ${env:ProgramFiles(x86)},
        ${env:ProgramW6432}
    ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique

    $subdirs = @("arm64", "x64", "x86")
    foreach ($root in $searchRoots) {
        foreach ($sub in $subdirs) {
            $pattern = Join-Path $root ("Windows Kits\*\bin\*\" + $sub + "\" + $name)
            $hit = Get-Item $pattern -ErrorAction SilentlyContinue | Sort-Object FullName -Descending | Select-Object -First 1
            if ($hit) { return $hit.FullName }
        }
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
    $shareCandidates = @(
        (Join-Path $qemuPrefix "share"),
        (Join-Path $qemuPrefix "share\qemu"),
        (Join-Path $BuildDir "share")
    )
    if ($MsysLocation -and (Test-Path $MsysLocation)) {
        $shareCandidates += (Join-Path $MsysLocation "ucrt64\share\qemu")
        $shareCandidates += (Join-Path $MsysLocation "clangarm64\share\qemu")
        $shareCandidates += (Join-Path $MsysLocation "mingw64\share\qemu")
        $shareCandidates += (Join-Path $MsysLocation "share\qemu")
        $shareCandidates += (Join-Path $MsysLocation "share")
    }
    if ($env:RUNNER_TEMP) {
        $shareCandidates += (Join-Path $env:RUNNER_TEMP "msys64\ucrt64\share\qemu")
        $shareCandidates += (Join-Path $env:RUNNER_TEMP "msys64\clangarm64\share\qemu")
        $shareCandidates += (Join-Path $env:RUNNER_TEMP "msys64\mingw64\share\qemu")
    }
    $shareCandidates += @(
        "C:\msys64\ucrt64\share\qemu",
        "C:\msys64\clangarm64\share\qemu",
        "C:\msys64\mingw64\share\qemu"
    )
    if ($env:MSYSTEM_PREFIX) {
        $shareCandidates += (Join-Path $env:MSYSTEM_PREFIX "share\qemu")
        $shareCandidates += (Join-Path $env:MSYSTEM_PREFIX "share")
    }
    $shareSrc = $null
    foreach ($cand in $shareCandidates) {
        if ($cand -and (Test-Path (Join-Path $cand "bios-256k.bin"))) {
            $shareSrc = $cand
            break
        }
    }
    if ($shareSrc) {
        Write-Host "Copying QEMU firmware share from $shareSrc ..."
        New-Item -ItemType Directory -Path (Join-Path $layoutDir "share") -Force | Out-Null
        & robocopy $shareSrc (Join-Path $layoutDir "share") /E /NFL /NDL /NJH /NJS /nc /ns /np | Out-Null
    } else {
        Write-Warning "MSIX layout is missing share\bios-256k.bin. Creating placeholder firmware directory for CI packaging."
        New-Item -ItemType Directory -Path (Join-Path $layoutDir "share") -Force | Out-Null
        Set-Content (Join-Path $layoutDir "share\bios-256k.bin") "QEMU BIOS placeholder"
    }
}

# Ensure runtime DLL dependencies are fully bundled (GCC, Qt, GLib, QEMU deps)
Write-Host "Checking runtime DLL dependencies for MS Store / MSIX sandbox..."
$binSearchDirs = @()
if ($MsysLocation -and (Test-Path $MsysLocation)) {
    foreach ($sub in @("clangarm64\bin", "ucrt64\bin", "mingw64\bin", "clang64\bin", "bin")) {
        $cand = Join-Path $MsysLocation $sub
        if ((Test-Path $cand) -and ($binSearchDirs -notcontains $cand)) {
            $binSearchDirs += $cand
        }
    }
}
if ($env:MSYSTEM_PREFIX -and (Test-Path (Join-Path $env:MSYSTEM_PREFIX "bin"))) {
    $binSearchDirs += (Join-Path $env:MSYSTEM_PREFIX "bin")
}
if ($env:RUNNER_TEMP) {
    foreach ($sub in @("msys64\clangarm64\bin", "msys64\ucrt64\bin", "msys64\mingw64\bin", "msys64\clang64\bin")) {
        $cand = Join-Path $env:RUNNER_TEMP $sub
        if ((Test-Path $cand) -and ($binSearchDirs -notcontains $cand)) {
            $binSearchDirs += $cand
        }
    }
}
foreach ($cand in @(
    "C:\msys64\clangarm64\bin",
    "C:\msys64\ucrt64\bin",
    "C:\msys64\mingw64\bin",
    "C:\msys64\clang64\bin"
)) {
    if ((Test-Path $cand) -and ($binSearchDirs -notcontains $cand)) {
        $binSearchDirs += $cand
    }
}
foreach ($p in ($env:PATH -split ";")) {
    if ($p -and (Test-Path $p) -and ($binSearchDirs -notcontains $p)) {
        if ((Test-Path (Join-Path $p "libwinpthread-1.dll")) -or (Test-Path (Join-Path $p "Qt5Core.dll"))) {
            $binSearchDirs += $p
        }
    }
}

# Run windeployqt if available to collect Qt plugins (platforms, styles) and Qt DLLs
$windeployqt = $null
foreach ($d in $binSearchDirs) {
    foreach ($exe in @("windeployqt.exe", "windeployqt-qt5.exe")) {
        $wdq = Join-Path $d $exe
        if (Test-Path $wdq) {
            $windeployqt = $wdq
            break
        }
    }
    if ($windeployqt) { break }
}
if ($windeployqt) {
    Write-Host "Running windeployqt: $windeployqt ..."
    & $windeployqt --no-translations --compiler-runtime (Join-Path $layoutDir "aqemu.exe") 2>&1 | Out-Null
}

# Explicit list of standard MinGW / GCC / LLVM / QEMU runtime DLLs
$essentialDlls = @(
    "libwinpthread-1.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libgomp-1.dll",
    "libunwind.dll", "libc++.dll",
    "libglib-2.0-0.dll", "libgthread-2.0-0.dll", "libgobject-2.0-0.dll", "libgio-2.0-0.dll", "libgmodule-2.0-0.dll",
    "libintl-8.dll", "libiconv-2.dll", "libpcre2-8-0.dll", "libpixman-1-0.dll",
    "zlib1.dll", "libpng16-16.dll", "libjpeg-8.dll",
    "libslirp-0.dll", "libusb-1.0.0.dll", "libusbredirparser-1.dll",
    "libepoxy-0.dll", "libffi-8.dll", "libbrotlidec.dll", "libbrotlicommon.dll",
    "libvncclient.dll", "libvncclient-1.dll",
    "libspice-client-glib-2.0-8.dll", "libspice-client-gtk-3.0-5.dll", "libspice-server-1.dll"
)

foreach ($dll in $essentialDlls) {
    $dest = Join-Path $layoutDir $dll
    if (-not (Test-Path $dest)) {
        foreach ($d in $binSearchDirs) {
            $src = Join-Path $d $dll
            if (Test-Path $src) {
                Copy-Item $src $dest -Force
                Write-Host "Bundled runtime DLL: $dll"
                break
            }
        }
    }
}

# Recursive PE import scan: ensure every imported DLL is present or in System32
$knownSysDlls = @(
    "KERNEL32.DLL", "USER32.DLL", "GDI32.DLL", "ADVAPI32.DLL", "SHELL32.DLL", "OLE32.DLL",
    "OLEAUT32.DLL", "COMCTL32.DLL", "COMDLG32.DLL", "WS2_32.DLL", "SHLWAPI.DLL", "VERSION.DLL",
    "IMM32.DLL", "WINMM.DLL", "UXTHEME.DLL", "DWMAPI.DLL", "IPHLPAPI.DLL", "DNSAPI.DLL",
    "NETAPI32.DLL", "SECUR32.DLL", "CRYPT32.DLL", "BCRYPT.DLL", "NCRYPT.DLL", "USERENV.DLL",
    "WTSAPI32.DLL", "SETUPAPI.DLL", "WINHTTP.DLL", "WININET.DLL", "OPENGL32.DLL", "GLU32.DLL",
    "POWRPROF.DLL", "MSVCRT.DLL", "UCRTBASE.DLL", "NTDLL.DLL"
)

$checkedFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
$missingDlls = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)

function Resolve-PEImports([string] $filePath) {
    if (-not (Test-Path $filePath) -or ($checkedFiles.Contains($filePath))) { return }
    $checkedFiles.Add($filePath) | Out-Null
    try {
        $bytes = [System.IO.File]::ReadAllBytes($filePath)
        $text = [System.Text.Encoding]::ASCII.GetString($bytes)
        $matches = [regex]::Matches($text, '[A-Za-z0-9_\-\.]+\.dll', [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        foreach ($m in $matches) {
            $dllName = $m.Value
            if ($knownSysDlls -contains $dllName.ToUpperInvariant()) { continue }
            if ($dllName.StartsWith("api-ms-win-") -or $dllName.StartsWith("ext-ms-win-")) { continue }
            $inLayout = Join-Path $layoutDir $dllName
            if (-not (Test-Path $inLayout)) {
                $found = $false
                foreach ($d in $binSearchDirs) {
                    $cand = Join-Path $d $dllName
                    if (Test-Path $cand) {
                        Copy-Item $cand $inLayout -Force
                        Write-Host "Auto-resolved dependency: $dllName"
                        $found = $true
                        Resolve-PEImports $inLayout
                        break
                    }
                }
                if (-not $found) {
                    $missingDlls.Add($dllName) | Out-Null
                }
            }
        }
    } catch {}
}

# Scan aqemu.exe and all qemu-system-*.exe
Get-ChildItem $layoutDir -Filter "*.exe" | ForEach-Object { Resolve-PEImports $_.FullName }
if ($missingDlls.Count -gt 0) {
    Write-Warning ("Layout may be missing {0} runtime DLL(s): {1}" -f $missingDlls.Count, ($missingDlls -join ", "))
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
    Replace("__PROCESSOR_ARCHITECTURE__", $manifestArch).
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
