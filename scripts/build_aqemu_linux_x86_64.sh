#!/usr/bin/env bash
# ==============================================================================
# Build AQEMU for Linux x86_64 (Ubuntu / Debian / Fedora / Arch / openSUSE)
# ==============================================================================
# Usage: ./scripts/build_aqemu_linux_x86_64.sh [--install]
#
# Flags used:
#   -DAQEMU_WITH_SPICE_GTK=ON     Embedded SPICE display client
#   -DCMAKE_BUILD_TYPE=Release    Release build with optimizations
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build_linux_x86_64"
INSTALL_PREFIX="${HOME}/.local"
DO_INSTALL=false

for arg in "$@"; do
  case "$arg" in
    --install)
      DO_INSTALL=true
      ;;
    *)
      ;;
  esac
done

echo "=== Building AQEMU for Linux (x86_64) ==="

ARCH="$(uname -m)"
if [[ "${ARCH}" != "x86_64" ]]; then
  echo "WARNING: Detected architecture '${ARCH}', not x86_64."
fi

# Verify core dependencies
MISSING_PKGS=()
for cmd in cmake ninja pkg-config g++; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    MISSING_PKGS+=("$cmd")
  fi
done

if [[ ${#MISSING_PKGS[@]} -gt 0 ]]; then
  echo "Missing build tools: ${MISSING_PKGS[*]}"
  echo "Debian/Ubuntu: sudo apt install -y build-essential cmake ninja-build pkg-config qtbase5-dev libqt5widgets5 libqt5network5 libqt5dbus5 libvncserver-dev libspice-client-glib-2.0-dev"
  echo "Fedora:        sudo dnf install -y gcc-c++ cmake ninja-build pkgconf qt5-qtbase-devel libvncserver-devel spice-glib-devel"
  echo "Arch Linux:    sudo pacman -S --needed base-devel cmake ninja pkgconf qt5-base libvncserver spice-gtk"
  exit 1
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

CMAKE_FLAGS=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DAQEMU_WITH_SPICE_GTK=ON
)

if [[ -d "${ROOT}/third_party/qemu-install/bin" ]]; then
  CMAKE_FLAGS+=(-DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX="${ROOT}/third_party/qemu-install")
fi

echo "Configuring CMake in ${BUILD_DIR}..."
cmake "${CMAKE_FLAGS[@]}" "${ROOT}"

echo "Compiling AQEMU with Ninja..."
ninja -j"$(nproc 2>/dev/null || echo 4)"

echo "=== Build Complete! ==="
echo "Binary: ${BUILD_DIR}/aqemu"

if [[ "${DO_INSTALL}" == "true" ]]; then
  echo "Installing to ${INSTALL_PREFIX}..."
  cmake --install . --prefix "${INSTALL_PREFIX}"
  echo "Installed successfully to ${INSTALL_PREFIX}/bin/aqemu"
fi

echo ""
echo "To run AQEMU:"
echo "  ${BUILD_DIR}/aqemu"
