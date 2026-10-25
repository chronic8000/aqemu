#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export PATH="/clangarm64/bin:/usr/bin:${PATH}"
export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
if [[ "${MSYSTEM:-}" != "CLANGARM64" ]]; then
  echo "ERROR: need CLANGARM64" >&2
  exit 1
fi

BAK="$(ls -d "${ROOT}/third_party"/reims-vgpu.bak-* 2>/dev/null | tail -1 || true)"
if [[ -z "${BAK}" || ! -d "${BAK}" ]]; then
  echo "ERROR: no reims-vgpu.bak-* tree found" >&2
  exit 1
fi
echo "=== restoring ${BAK} ==="
rm -rf "${ROOT}/third_party/reims-vgpu"
mv "${BAK}" "${ROOT}/third_party/reims-vgpu"

# Ensure Windows xtask gate + unix perms stay patched (bak already has these).
grep -q 'target_os = "windows"' "${ROOT}/third_party/reims-vgpu/xtask/src/dist.rs"

# MAC_PVG for all x86_64-softmmu + device alias
MESON="${ROOT}/third_party/reims-vgpu/vendor/qemu/meson.build"
if grep -q "host_os == 'windows' and target == 'x86_64-softmmu'" "${MESON}"; then
  sed -i "s/(host_os == 'windows' and target == 'x86_64-softmmu')/target == 'x86_64-softmmu'/" "${MESON}"
  echo "Patched meson MAC_PVG gate"
fi
PCI="${ROOT}/third_party/reims-vgpu/vendor/qemu/hw/display/apple-gfx-pci.c"
if [[ -f "${PCI}" ]] && ! grep -q 'reims-vgpu-pci' "${PCI}"; then
  python3 - "${PCI}" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
t = p.read_text(encoding="utf-8")
old = """    }
};
DEFINE_TYPES(apple_gfx_pci_types)"""
new = """    },
    {
        .name   = "reims-vgpu-pci",
        .parent = TYPE_APPLE_GFX_PCI,
    },
};
DEFINE_TYPES(apple_gfx_pci_types)"""
if old not in t:
    raise SystemExit("WARN: alias anchor missing")
p.write_text(t.replace(old, new, 1), encoding="utf-8")
print("Added reims-vgpu-pci alias")
PY
fi

# shellcheck source=qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags
# shellcheck source=qemu_special_targets.sh
source "${ROOT}/scripts/qemu_special_targets.sh"

PREFIX="${ROOT}/third_party/qemu-install-arm64"
JOBS="$(nproc 2>/dev/null || echo 4)"
aqemu_build_reims "${PREFIX}" "${JOBS}"

for name in qemu-system-reimsvgpu.exe qemu-system-reims.exe qemu-system-reims3d.exe qemu-system-x86_64-reims.exe; do
  [[ -f "${PREFIX}/bin/${name}" ]] || continue
  cp -f "${PREFIX}/bin/${name}" "${ROOT}/build_woa/${name}"
  echo "Installed ${ROOT}/build_woa/${name}"
done
BIN="${ROOT}/build_woa/qemu-system-reims3d.exe"
"${BIN}" -device help 2>/dev/null | grep -E 'reims-vgpu-pci|apple-gfx-pci'
echo "=== done: ${BIN} ==="
ls -la "${BIN}"
