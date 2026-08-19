# Build xerub/img4lib as img4.exe (MinGW + OpenSSL + in-tree lzfse).
# Upstream Makefile is Unix-style; this is the Windows port:
#   - clone img4lib + lzfse submodule
#   - overlay extras/Inferno/patches/img4lib_vfs_file_win.c (O_BINARY, _commit)
#   - make with gcc + -lcrypto
#
# Prerequisites: MSYS2 UCRT64 or MINGW64 gcc + OpenSSL
#   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-openssl
#
# Usage (PowerShell from repo root):
#   .\scripts\build_img4_windows.ps1
#
# Copies img4.exe (and libcrypto if needed) next to build_win\aqemu.exe.

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Src = Join-Path $Root "third_party\img4lib"
$Overlay = Join-Path $Root "extras\Inferno\patches\img4lib_vfs_file_win.c"
$MingwBin = "C:\msys64\ucrt64\bin"
if (-not (Test-Path "$MingwBin\gcc.exe")) {
	$MingwBin = "C:\msys64\mingw64\bin"
}
if (-not (Test-Path "$MingwBin\gcc.exe")) {
	throw "Need MSYS2 ucrt64 or mingw64 gcc (with OpenSSL)."
}
if (-not (Test-Path $Overlay)) {
	throw "Missing Windows vfs overlay: $Overlay"
}

if (-not (Test-Path (Join-Path $Src ".git"))) {
	git clone --recurse-submodules https://github.com/xerub/img4lib.git $Src
} else {
	Push-Location $Src
	git submodule update --init --recursive
	Pop-Location
}

Copy-Item $Overlay (Join-Path $Src "libvfs\vfs_file.c") -Force

$env:PATH = "$MingwBin;C:\msys64\usr\bin;" + $env:PATH
$Make = Join-Path $MingwBin "mingw32-make.exe"
if (-not (Test-Path $Make)) { $Make = "make" }

Push-Location (Join-Path $Src "lzfse")
& $Make CC=gcc
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "lzfse build failed" }
Pop-Location

$Cflags = "-Wall -O2 -I. -Ilzfse/src -DiOS10 -DDER_MULTIBYTE_TAGS=1 -DDER_TAG_SIZE=8 -D__unused=__attribute__((unused)) -Wno-deprecated-declarations -Wno-unused-parameter"
Push-Location $Src
& $Make CC=gcc LD=gcc "CFLAGS=$Cflags" "LDFLAGS=-g -Llzfse/build/bin" "LDLIBS=-llzfse -lcrypto"
if ($LASTEXITCODE -ne 0) { Pop-Location; throw "img4 build failed" }
Pop-Location

$Exe = Join-Path $Src "img4.exe"
if (-not (Test-Path $Exe)) { $Exe = Join-Path $Src "img4" }
if (-not (Test-Path $Exe)) { throw "img4 binary not produced" }

$DestDir = Join-Path $Root "build_win"
if (Test-Path $DestDir) {
	Copy-Item $Exe (Join-Path $DestDir "img4.exe") -Force
	foreach ($dll in @("libcrypto-3-x64.dll", "libssl-3-x64.dll", "libwinpthread-1.dll", "zlib1.dll", "libgcc_s_seh-1.dll")) {
		$from = Join-Path $MingwBin $dll
		if (Test-Path $from) {
			Copy-Item $from (Join-Path $DestDir $dll) -Force
		}
	}
	Write-Host "Installed: $DestDir\img4.exe"
} else {
	Write-Host "Built: $Exe (copy next to aqemu.exe)"
}
