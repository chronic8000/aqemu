#!/usr/bin/env bash
# ==============================================================================
# Build AQEMU for Windows x86_64 via MSYS2 UCRT64 / MINGW64 Shell
# ==============================================================================
# Usage: ./scripts/build_aqemu_win64.sh [--deps]
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build_win"

# Optional: install all necessary dependencies
if [[ "${1:-}" == "--deps" ]]; then
  echo "Installing UCRT64 dependencies..."
  pacman -S --needed --noconfirm \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-qt5-base \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-spice-gtk \
    mingw-w64-ucrt-x86_64-libvncserver \
    mingw-w64-ucrt-x86_64-libslirp \
    mingw-w64-ucrt-x86_64-libusb
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
export PKG_CONFIG_PATH="/ucrt64/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

CMAKE_FLAGS=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DAQEMU_WITH_SPICE_GTK=ON
)

if [[ -d "${ROOT}/third_party/qemu-install/bin" ]]; then
  CMAKE_FLAGS+=(-DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX="${ROOT}/third_party/qemu-install")
fi

echo "Configuring CMake for Windows x86_64..."
cmake "${CMAKE_FLAGS[@]}" "${ROOT}"

echo "Building AQEMU..."
ninja -j"$(nproc 2>/dev/null || echo 4)"

echo "=== Build Succeeded! ==="
echo "Executable: ${BUILD_DIR}/aqemu.exe"
