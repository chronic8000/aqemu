#!/usr/bin/env bash
# Build native Windows ARM64 qemu-system-reims3d.exe from the already-patched
# third_party/reims-vgpu tree (no re-clone).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
if [[ "${MSYSTEM:-}" != "CLANGARM64" ]]; then
  echo "ERROR: run from MSYS2 CLANGARM64 (got MSYSTEM=${MSYSTEM:-none})" >&2
  exit 1
fi

export PATH="/clangarm64/bin:/usr/bin:${PATH}"
export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
cd "${ROOT}"

# shellcheck source=../../scripts/qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags

# shellcheck source=../../scripts/qemu_special_targets.sh
source "${ROOT}/scripts/qemu_special_targets.sh"

PREFIX="${ROOT}/third_party/qemu-install-arm64"
JOBS="$(nproc 2>/dev/null || echo 4)"
echo "=== aqemu_build_reims (native Windows, no clone) -> ${PREFIX} ==="
aqemu_build_reims "${PREFIX}" "${JOBS}"

for name in qemu-system-reimsvgpu.exe qemu-system-reims.exe qemu-system-reims3d.exe qemu-system-x86_64-reims.exe; do
  if [[ -f "${PREFIX}/bin/${name}" ]]; then
    mkdir -p "${ROOT}/build_woa"
    cp -f "${PREFIX}/bin/${name}" "${ROOT}/build_woa/${name}"
    echo "Installed ${ROOT}/build_woa/${name}"
  fi
done

OLD="${ROOT}/third_party/reims-vgpu"
ROM_SRC=""
for cand in \
  "${PREFIX}/share/qemu/ReimsVgpu.rom" \
  "${OLD}/target/dist-vulkan-aarch64/share/reims-vgpu/ReimsVgpu.rom" \
  "${ROOT}/../rvg-a64/dist-vulkan/share/reims-vgpu/ReimsVgpu.rom"; do
  if [[ -f "${cand}" ]]; then
    ROM_SRC="${cand}"
    break
  fi
done
if [[ -n "${ROM_SRC}" ]]; then
  mkdir -p "${ROOT}/build_woa/share"
  cp -f "${ROM_SRC}" "${ROOT}/build_woa/share/ReimsVgpu.rom"
  echo "Installed ReimsVgpu.rom from ${ROM_SRC}"
fi

BIN="${ROOT}/build_woa/qemu-system-reims3d.exe"
"${BIN}" -device help 2>/dev/null | grep -E 'reims-vgpu-pci|apple-gfx-pci' || {
  echo "ERROR: ${BIN} missing Reims GPU device" >&2
  exit 1
}
echo "=== done: ${BIN} ==="
ls -la "${BIN}"
file "${BIN}" 2>/dev/null || true
