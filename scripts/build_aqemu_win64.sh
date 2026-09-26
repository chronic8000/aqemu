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

  # Disable pacman's strict 10-second download timeout if configured in /etc/pacman.conf
  if [[ -f /etc/pacman.conf ]] && grep -q '^#DisableDownloadTimeout' /etc/pacman.conf 2>/dev/null; then
    echo "Configuring MSYS2 /etc/pacman.conf to disable download timeouts on slow mirrors..."
    sed -i 's/^#DisableDownloadTimeout/DisableDownloadTimeout/' /etc/pacman.conf 2>/dev/null || true
  fi

  DEPS=(
    mingw-w64-ucrt-x86_64-toolchain
    mingw-w64-ucrt-x86_64-qt5-base
    mingw-w64-ucrt-x86_64-cmake
    mingw-w64-ucrt-x86_64-ninja
    mingw-w64-ucrt-x86_64-pkgconf
    mingw-w64-ucrt-x86_64-spice-gtk
    mingw-w64-ucrt-x86_64-libvncserver
    mingw-w64-ucrt-x86_64-libslirp
    mingw-w64-ucrt-x86_64-libusb \
    mingw-w64-ucrt-x86_64-gobject-introspection
  )

  # Retry up to 3 times to gracefully recover from transient MSYS2 mirror dropouts
  MAX_RETRIES=3
  SUCCESS=0
  for ((attempt=1; attempt<=MAX_RETRIES; attempt++)); do
    echo "Running pacman (attempt ${attempt}/${MAX_RETRIES})..."
    if pacman -S --needed --noconfirm "${DEPS[@]}"; then
      SUCCESS=1
      echo "All dependencies installed successfully!"
      break
    fi
    if [[ $attempt -lt $MAX_RETRIES ]]; then
      echo "Mirror connection timed out or interrupted. Retrying in 5s (cached packages will not be re-downloaded)..."
      sleep 5
    fi
  done

  if [[ $SUCCESS -ne 1 ]]; then
    echo "ERROR: pacman dependency installation failed after ${MAX_RETRIES} attempts."
    echo "If mirror.msys2.org is timing out, try refreshing mirrors with:"
    echo "  pacman -Sy"
    echo "or edit /etc/pacman.conf and ensure 'DisableDownloadTimeout' is enabled."
    exit 1
  fi
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
