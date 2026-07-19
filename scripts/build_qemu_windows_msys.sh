#!/usr/bin/env bash
# Pure MSYS2 MinGW64 build of bundled QEMU (same toolchain for compiler + glib/spice).
# Prerequisite: scripts/fix_msys2_gcc_admin.ps1 run as Administrator once.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
PREFIX="${1:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build-win"

export MSYSTEM=MINGW64
# /usr/bin first so Meson can find msys `diff` (via C:\msys64\usr\bin)
export PATH="/mingw64/bin:/usr/bin:${PATH}"
# Meson runs under Win32 Python — must use C:/ paths (not /mingw64/...)
export PKG_CONFIG="C:/msys64/mingw64/bin/pkg-config.exe"
export PKG_CONFIG_PATH="C:/msys64/mingw64/lib/pkgconfig"
unset PKG_CONFIG_LIBDIR || true

echo "Using $(which gcc)"
echo "Using PKG_CONFIG=$PKG_CONFIG"
gcc --version | head -1
if ! printf '%s\n' 'int main(void){return 0;}' | gcc -x c - -c -o /tmp/aqemu_cc_probe.o; then
  echo "ERROR: MSYS2 gcc cannot compile. Run as Admin:"
  echo "  powershell -ExecutionPolicy Bypass -File scripts/fix_msys2_gcc_admin.ps1"
  exit 1
fi

"$PKG_CONFIG" --modversion glib-2.0

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

EXTRA=(--enable-vnc --disable-docs --disable-guest-agent --disable-werror)
if "$PKG_CONFIG" --exists spice-server 2>/dev/null; then
  EXTRA+=(--enable-spice)
  echo "SPICE enabled"
fi

# Win32 Meson looks on PATH for `diff`; put MSYS tools on a Windows-style PATH
export PATH="C:/msys64/mingw64/bin:C:/msys64/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
# Ensure `diff` is next to other mingw tools
if [[ -x /usr/bin/diff.exe && ! -e /mingw64/bin/diff.exe ]]; then
  cp -f /usr/bin/diff.exe /mingw64/bin/diff.exe
fi
command -v diff
diff --version | head -1

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --cc=gcc \
  --cxx=g++ \
  --target-list=x86_64-softmmu,i386-softmmu,aarch64-softmmu \
  "${EXTRA[@]}"

echo "=== configure OK, building ==="
ninja -j"$(nproc 2>/dev/null || echo 4)"
ninja install
echo "Installed to ${PREFIX}"
ls -la "${PREFIX}/bin"/qemu-system-* "${PREFIX}/bin"/qemu-img* 2>/dev/null || true
