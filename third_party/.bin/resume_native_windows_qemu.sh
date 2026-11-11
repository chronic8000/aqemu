#!/usr/bin/env bash
# Resume QEMU ninja after a failed compile; assumes reims dist + meson setup exist.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
if [[ "${MSYSTEM:-}" != "CLANGARM64" ]]; then
  echo "ERROR: run from MSYS2 CLANGARM64" >&2
  exit 1
fi
export PATH="/clangarm64/bin:/usr/bin:${PATH}"
export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

BUILD="${ROOT}/third_party/reims-build-aarch64"
STAGE="${ROOT}/third_party/reims-stage-aarch64"
PREFIX="${ROOT}/third_party/qemu-install-arm64"
DIST="${ROOT}/third_party/reims-vgpu/target/dist-vulkan-aarch64"
JOBS="$(nproc 2>/dev/null || echo 4)"

if [[ ! -f "${BUILD}/build.ninja" ]]; then
  echo "ERROR: missing ${BUILD}/build.ninja — run full aqemu_build_reims first" >&2
  exit 1
fi
if [[ ! -f "${DIST}/lib/pkgconfig/reims-vgpu.pc" ]]; then
  echo "ERROR: missing reims dist pc" >&2
  exit 1
fi

# Confirm nettle4 fix is present.
if ! grep -q 'NETTLE_VERSION_MAJOR >= 4' \
  "${ROOT}/third_party/reims-vgpu/vendor/qemu/crypto/cipher-nettle.c.inc"; then
  echo "ERROR: nettle4 gcm_digest patch missing from vendor qemu" >&2
  exit 1
fi

export PKG_CONFIG_PATH="$(cygpath -m "${DIST}/lib/pkgconfig");${PKG_CONFIG_PATH:-}"
export CC="$(cygpath -m /clangarm64/bin/clang.exe)"
export CXX="$(cygpath -m /clangarm64/bin/clang++.exe)"

echo "=== ninja -C ${BUILD} -j${JOBS} ==="
ninja -C "${BUILD}" -j"${JOBS}"

echo "=== ninja install ==="
DESTDIR= ninja -C "${BUILD}" install

mkdir -p "${PREFIX}/bin" "${ROOT}/build_woa"
if [[ -f "${STAGE}/bin/qemu-system-x86_64.exe" ]]; then
  cp -f "${STAGE}/bin/qemu-system-x86_64.exe" "${PREFIX}/bin/qemu-system-reims3d.exe"
  cp -f "${PREFIX}/bin/qemu-system-reims3d.exe" "${ROOT}/build_woa/qemu-system-reims3d.exe"
elif [[ -f "${BUILD}/qemu-system-x86_64.exe" ]]; then
  cp -f "${BUILD}/qemu-system-x86_64.exe" "${PREFIX}/bin/qemu-system-reims3d.exe"
  cp -f "${PREFIX}/bin/qemu-system-reims3d.exe" "${ROOT}/build_woa/qemu-system-reims3d.exe"
fi

ROM=""
for cand in \
  "${STAGE}/share/qemu/ReimsVgpu.rom" \
  "${PREFIX}/share/qemu/ReimsVgpu.rom" \
  "${DIST}/share/reims-vgpu/ReimsVgpu.rom"; do
  [[ -f "${cand}" ]] && ROM="${cand}" && break
done
if [[ -n "${ROM}" ]]; then
  mkdir -p "${ROOT}/build_woa/share"
  cp -f "${ROM}" "${ROOT}/build_woa/share/ReimsVgpu.rom"
fi

BIN="${ROOT}/build_woa/qemu-system-reims3d.exe"
file "${BIN}" || true
"${BIN}" -device help 2>/dev/null | grep -E 'reims-vgpu-pci|apple-gfx-pci' || {
  echo "ERROR: ${BIN} missing Reims GPU device" >&2
  exit 1
}
echo "=== done: ${BIN} ==="
ls -la "${BIN}"
