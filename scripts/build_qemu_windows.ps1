# Build bundled QEMU for Windows (MSYS2 / MinGW)
# Run from an MSYS2 MinGW64 shell after: pacman -S mingw-w64-x86_64-spice ...
# Usage: bash scripts/build_qemu_windows.sh [PREFIX]

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$QemuSrc = Join-Path $Root "third_party\qemu"
$Prefix = if ($args.Count -ge 1) { $args[0] } else { Join-Path $Root "third_party\qemu-install" }
$BuildDir = Join-Path $Root "third_party\qemu-build-win"

Write-Host "This script documents the Windows QEMU build."
Write-Host "Preferred: use MSYS2 MinGW64 and run scripts/build_qemu_windows.sh (bash)."
Write-Host ""
Write-Host "MSYS2 steps:"
Write-Host "  pacman -S --needed git python ninja mingw-w64-x86_64-toolchain mingw-w64-x86_64-glib2 mingw-w64-x86_64-pixman mingw-w64-x86_64-spice"
Write-Host "  cd $(Join-Path $Root 'third_party')"
Write-Host "  mkdir -p qemu-build-win && cd qemu-build-win"
Write-Host "  ../qemu/configure --prefix=$Prefix --target-list=x86_64-softmmu,i386-softmmu,aarch64-softmmu --enable-spice --enable-vnc --disable-docs"
Write-Host "  ninja && ninja install"
Write-Host ""
Write-Host "Then configure AQEMU with -DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=$Prefix"

# If bash + qemu configure exist under MSYS path, try nothing automatically here —
# Windows MSVC cannot build upstream QEMU easily; MinGW is required.
if (-not (Test-Path (Join-Path $QemuSrc "meson.build"))) {
  Write-Error "Missing third_party/qemu. Run: git submodule update --init --depth 1 third_party/qemu"
}
