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

echo "=== Deploying Windows x86_64 Runtime Libraries ==="
# Try windeployqt or windeployqt-qt5 if available
WDQ="$(which windeployqt-qt5 2>/dev/null || which windeployqt 2>/dev/null || true)"
if [[ -n "${WDQ}" && -x "${WDQ}" ]]; then
  echo "Running ${WDQ} for Qt5 DLLs and plugins..."
  "${WDQ}" --no-translations --compiler-runtime "${BUILD_DIR}/aqemu.exe" || true
fi

# Ensure essential Qt5 and LibVNCServer runtime DLLs are copied
echo "Copying Qt5 and runtime DLLs into ${BUILD_DIR}..."
cp -f /ucrt64/bin/Qt5Core.dll "${BUILD_DIR}/" 2>/dev/null || true
cp -f /ucrt64/bin/Qt5Gui.dll "${BUILD_DIR}/" 2>/dev/null || true
cp -f /ucrt64/bin/Qt5Widgets.dll "${BUILD_DIR}/" 2>/dev/null || true
cp -f /ucrt64/bin/Qt5Network.dll "${BUILD_DIR}/" 2>/dev/null || true
cp -f /ucrt64/bin/Qt5PrintSupport.dll "${BUILD_DIR}/" 2>/dev/null || true
cp -f /ucrt64/bin/libvncclient*.dll "${BUILD_DIR}/" 2>/dev/null || true
cp -f /ucrt64/bin/libvncserver*.dll "${BUILD_DIR}/" 2>/dev/null || true

# Deploy Qt platform plugin (required for GUI window display on Windows)
mkdir -p "${BUILD_DIR}/platforms"
for plat in /ucrt64/share/qt5/plugins/platforms/qwindows.dll /ucrt64/lib/qt5/plugins/platforms/qwindows.dll /ucrt64/plugins/platforms/qwindows.dll; do
  if [[ -f "${plat}" ]]; then
    cp -f "${plat}" "${BUILD_DIR}/platforms/"
    echo "Deployed platforms/qwindows.dll from ${plat}"
    break
  fi
done

# Deploy QEMU binaries and runtime DLLs if built under third_party/qemu-install
if [[ -d "${ROOT}/third_party/qemu-install/bin" ]]; then
  echo "Deploying QEMU executables and runtime dependencies from third_party/qemu-install..."
  cp -f "${ROOT}/third_party/qemu-install/bin"/*.exe "${BUILD_DIR}/" 2>/dev/null || true
  cp -f "${ROOT}/third_party/qemu-install/bin"/*.dll "${BUILD_DIR}/" 2>/dev/null || true
  if [[ -d "${ROOT}/third_party/qemu-install/share" ]]; then
    mkdir -p "${BUILD_DIR}/share"
    cp -rf "${ROOT}/third_party/qemu-install/share"/* "${BUILD_DIR}/share/" 2>/dev/null || true
  fi
fi

# Recursively resolve and copy all transitive DLL dependencies from /ucrt64/bin
echo "Resolving all transitive runtime DLL dependencies with ldd..."
for pass in 1 2 3 4; do
  NEW_COPIED=0
  for bin in "${BUILD_DIR}"/*.dll "${BUILD_DIR}"/*.exe "${BUILD_DIR}"/platforms/*.dll; do
    [[ -f "$bin" ]] || continue
    while read -r dep; do
      if [[ -f "$dep" && ! -f "${BUILD_DIR}/$(basename "$dep")" ]]; then
        cp -f "$dep" "${BUILD_DIR}/"
        NEW_COPIED=$((NEW_COPIED + 1))
      fi
    done < <(ldd "$bin" 2>/dev/null | grep -iE '/(ucrt64|mingw64)/bin/' | awk '{print $3}' | sort -u)
  done
  if [[ $NEW_COPIED -eq 0 ]]; then
    break
  fi
  echo "Pass $pass: copied $NEW_COPIED additional runtime dependencies"
done

# Ensure third_party/qemu-install/bin also has all the resolved runtime DLLs so QEMU runs standalone
if [[ -d "${ROOT}/third_party/qemu-install/bin" ]]; then
  cp -f "${BUILD_DIR}"/*.dll "${ROOT}/third_party/qemu-install/bin/" 2>/dev/null || true
fi

echo "=== Build Succeeded! ==="
echo "Executable: ${BUILD_DIR}/aqemu.exe"
