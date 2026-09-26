#!/usr/bin/env bash
# ==============================================================================
# Build AQEMU for Raspberry Pi 5 (64-bit ARM / Raspberry Pi OS / Debian Bookworm)
# ==============================================================================
# Usage: ./scripts/build_aqemu_pi5.sh [--install]
#
# Flags used:
#   -DPI5_OPTIMIZATIONS=ON        Cortex-A76 tuning + 64KB ELF segment alignment
#   -DAQEMU_WITH_SPICE_GTK=ON     Embedded SPICE display client
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build_pi5"
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

echo "=== Building AQEMU for Raspberry Pi 5 (ARM64) ==="

ARCH="$(uname -m)"
if [[ "${ARCH}" != "aarch64" && "${ARCH}" != "arm64" ]]; then
  echo "WARNING: Detected architecture '${ARCH}'. Pi 5 requires a 64-bit OS (aarch64)."
fi

# Detect Pi model if available
if [[ -f /proc/device-tree/model ]]; then
  MODEL="$(tr -d '\0' < /proc/device-tree/model 2>/dev/null || true)"
  echo "Detected Hardware: ${MODEL}"
fi

# Verify core dependencies
MISSING_PKGS=()
for cmd in cmake ninja pkg-config g++; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    MISSING_PKGS+=("$cmd")
  fi
done

if [[ ${#MISSING_PKGS[@]} -gt 0 ]]; then
  echo "Missing required build tools: ${MISSING_PKGS[*]}"
  echo "Install with:"
  echo "  sudo apt update && sudo apt install -y build-essential cmake ninja-build pkg-config \\"
  echo "    qtbase5-dev libqt5widgets5 libqt5network5 libqt5dbus5 libvncserver-dev \\"
  echo "    libspice-client-glib-2.0-dev libslirp-dev libusb-1.0-0-dev"
  exit 1
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

CMAKE_FLAGS=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DPI5_OPTIMIZATIONS=ON
  -DAQEMU_WITH_SPICE_GTK=ON
)

# If QEMU bundle prefix exists, enable bundling
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
echo "To run AQEMU on Raspberry Pi OS (Wayland default):"
echo "  QT_QPA_PLATFORM=wayland ${BUILD_DIR}/aqemu"
echo "Or on X11 / Labwc:"
echo "  ${BUILD_DIR}/aqemu"
