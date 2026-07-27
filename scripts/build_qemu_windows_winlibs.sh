#!/usr/bin/env bash
# Build QEMU using WinLibs GCC + MSYS2 MinGW libraries (glib/spice/pixman).
# MSYS2's own gcc is often broken by a conflicting C:\WINDOWS\libwinpthread-1.dll.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
PREFIX="${1:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build-win"

WINLIBS_BIN_WIN="C:/Users/chron/AppData/Local/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin"
WINLIBS_BIN="/c/Users/chron/AppData/Local/Microsoft/WinGet/Packages/BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe/mingw64/bin"
MSYS_MINGW_WIN="C:/msys64/mingw64"
MSYS_MINGW="/mingw64"

if [[ ! -x "${WINLIBS_BIN}/gcc.exe" ]]; then
  echo "WinLibs gcc not found at ${WINLIBS_BIN}"
  echo "Install via: winget install BrechtSanders.WinLibs.POSIX.UCRT"
  exit 1
fi
if [[ ! -x "${MSYS_MINGW}/bin/pkg-config.exe" ]]; then
  echo "MSYS2 pkg-config missing. In MINGW64 shell:"
  echo "  pacman -S mingw-w64-x86_64-pkgconf mingw-w64-x86_64-glib2 mingw-w64-x86_64-pixman mingw-w64-x86_64-spice"
  exit 1
fi
if [[ ! -f "${MSYS_MINGW}/lib/pkgconfig/glib-2.0.pc" ]]; then
  echo "glib-2.0.pc missing. In MINGW64:"
  echo "  pacman -S mingw-w64-x86_64-glib2"
  exit 1
fi

# WinLibs compiler first; MSYS2 libs/tools second
export PATH="${WINLIBS_BIN}:${MSYS_MINGW}/bin:/usr/bin:${PATH}"

# Meson uses Win32 Python — ALL tool paths must be Windows-style (C:/...), not /c/...
export CC="${WINLIBS_BIN_WIN}/gcc.exe"
export CXX="${WINLIBS_BIN_WIN}/g++.exe"
export PKG_CONFIG="${MSYS_MINGW_WIN}/bin/pkg-config.exe"
export PKG_CONFIG_PATH="${MSYS_MINGW_WIN}/lib/pkgconfig"
unset PKG_CONFIG_LIBDIR || true

echo "Using CC=$CC"
echo "Using PKG_CONFIG=$PKG_CONFIG"
echo "Using PKG_CONFIG_PATH=$PKG_CONFIG_PATH"
"$CC" --version | head -1
"$PKG_CONFIG" --modversion glib-2.0
"$PKG_CONFIG" --exists spice-server && echo "spice-server: yes" || echo "spice-server: no"

# Prove Meson's preferred names resolve on PATH
command -v pkg-config
command -v pkgconf || true

mkdir -p "${BUILD_DIR}" "${PREFIX}"
# Fresh build dir (stale meson cache causes odd pkg-config failures)
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

EXTRA=(--disable-werror)
# shellcheck source=qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
# WinLibs build still uses MSYS2 pkg-config for libs
aqemu_qemu_feature_flags
# Drop --disable-werror duplicate if feature flags already set it
EXTRA=("${AQEMU_QEMU_EXTRA_CONFIGURE[@]}")

# Every softmmu target — Microsoft Store / portable packages must ship them all.
# shellcheck source=qemu_softmmu_targets.sh
source "${ROOT}/scripts/qemu_softmmu_targets.sh"
TARGETS="$(qemu_softmmu_target_list "${QEMU_SRC}")"
echo "Building ALL softmmu targets: ${TARGETS}"

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --cc="${CC}" \
  --cxx="${CXX}" \
  --target-list="${TARGETS}" \
  "${EXTRA[@]}"

# Do NOT add -I${MSYS_MINGW}/include globally: WinLibs stdlib.h clashes with
# MSYS2 headers under -Werror=redundant-decls. pkg-config supplies per-dep -I/-L.
echo "=== configure OK, building ==="
ninja -j"$(nproc 2>/dev/null || echo 4)"
ninja install
echo "Installed to ${PREFIX}"
ls -la "${PREFIX}/bin"/qemu-system-* "${PREFIX}/bin"/qemu-img* 2>/dev/null || true
aqemu_qemu_verify_install "${PREFIX}"
