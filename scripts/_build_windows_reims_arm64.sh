#!/usr/bin/env bash
# Fresh-clone steelbrain/reims-vgpu and build Windows ARM64 qemu-system-reims3d.exe
# into third_party/qemu-install-arm64 and build_woa next to aqemu.exe.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
if [[ "${MSYSTEM:-}" != "CLANGARM64" ]]; then
  echo "ERROR: run from MSYS2 CLANGARM64 (got MSYSTEM=${MSYSTEM:-none})" >&2
  exit 1
fi

export PATH="/clangarm64/bin:/usr/bin:${PATH}"
export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
cd "${ROOT}"

# shellcheck source=qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags

# shellcheck source=qemu_special_targets.sh
source "${ROOT}/scripts/qemu_special_targets.sh"

STAMP="$(date +%Y%m%d-%H%M%S)"
OLD="${ROOT}/third_party/reims-vgpu"
BAK="${ROOT}/third_party/reims-vgpu.bak-${STAMP}"
if [[ -d "${OLD}" ]]; then
  echo "=== moving previous tree to ${BAK} ==="
  mv "${OLD}" "${BAK}"
fi

echo "=== cloning https://github.com/steelbrain/reims-vgpu.git ==="
# Only vendor/qemu is required. --recurse-submodules pulls unused EDK2/openssl trees.
git clone --depth 1 https://github.com/steelbrain/reims-vgpu.git "${OLD}"
git -C "${OLD}" submodule update --init --depth 1 vendor/qemu

MESON="${OLD}/vendor/qemu/meson.build"
if [[ -f "${MESON}" ]]; then
  python3 - "${MESON}" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
t = p.read_text(encoding="utf-8")
old = """    if pvg.found() and (target in accelerator_targets.get('CONFIG_HVF', []) or
                        target in accelerator_targets.get('CONFIG_KVM', []) or
                        target in accelerator_targets.get('CONFIG_WHPX', []) or
                        (host_os == 'windows' and target == 'x86_64-softmmu'))
      target_kconfig += 'CONFIG_MAC_PVG=y'
    endif"""
new = """    # Reims is an x86_64 guest; enable the PCI device whenever the library is
    # present (Windows ARM64 TCG and Linux/WSL ARM64 TCG included).
    if pvg.found() and (target in accelerator_targets.get('CONFIG_HVF', []) or
                        target in accelerator_targets.get('CONFIG_KVM', []) or
                        target in accelerator_targets.get('CONFIG_WHPX', []) or
                        target == 'x86_64-softmmu')
      target_kconfig += 'CONFIG_MAC_PVG=y'
    endif"""
if old in t:
    p.write_text(t.replace(old, new), encoding="utf-8")
    print("Patched meson MAC_PVG gate")
elif "host_os == 'windows' and target == 'x86_64-softmmu'" in t:
    p.write_text(
        t.replace(
            "(host_os == 'windows' and target == 'x86_64-softmmu')",
            "target == 'x86_64-softmmu'",
            1,
        ),
        encoding="utf-8",
    )
    print("Patched meson MAC_PVG gate (windows-only -> x86_64-softmmu)")
elif "target == 'x86_64-softmmu')" in t and "CONFIG_MAC_PVG=y" in t:
    print("meson MAC_PVG gate already allows x86_64-softmmu")
else:
    print("WARN: could not patch meson MAC_PVG gate; continuing")
PY
fi

PCI="${OLD}/vendor/qemu/hw/display/apple-gfx-pci.c"
if [[ -f "${PCI}" ]] && ! grep -q 'reims-vgpu-pci' "${PCI}"; then
  python3 - "${PCI}" <<'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
t = p.read_text(encoding="utf-8")
needle = """static const TypeInfo apple_gfx_pci_types[] = {
    {
        .name          = TYPE_APPLE_GFX_PCI,
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(AppleGFXPCIState),
        .class_init    = apple_gfx_pci_class_init,
        .instance_init = apple_gfx_pci_init,
        .instance_finalize = apple_gfx_pci_finalize,
        .interfaces = (const InterfaceInfo[]) {
            { INTERFACE_PCIE_DEVICE },
            { },
        },
    }
};
DEFINE_TYPES(apple_gfx_pci_types)"""
repl = """static const TypeInfo apple_gfx_pci_types[] = {
    {
        .name          = TYPE_APPLE_GFX_PCI,
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(AppleGFXPCIState),
        .class_init    = apple_gfx_pci_class_init,
        .instance_init = apple_gfx_pci_init,
        .instance_finalize = apple_gfx_pci_finalize,
        .interfaces = (const InterfaceInfo[]) {
            { INTERFACE_PCIE_DEVICE },
            { },
        },
    },
    /* AQEMU uses this name for the same PCI device. */
    {
        .name   = "reims-vgpu-pci",
        .parent = TYPE_APPLE_GFX_PCI,
    },
};
DEFINE_TYPES(apple_gfx_pci_types)"""
if needle in t:
    p.write_text(t.replace(needle, repl), encoding="utf-8")
    print("Added reims-vgpu-pci type alias")
else:
    print("WARN: apple-gfx-pci.c shape changed; alias not applied")
PY
fi

PREFIX="${ROOT}/third_party/qemu-install-arm64"
JOBS="$(nproc 2>/dev/null || echo 4)"
echo "=== aqemu_build_reims -> ${PREFIX} ==="
aqemu_build_reims "${PREFIX}" "${JOBS}"

for name in qemu-system-reimsvgpu.exe qemu-system-reims.exe qemu-system-reims3d.exe qemu-system-x86_64-reims.exe; do
  if [[ -f "${PREFIX}/bin/${name}" ]]; then
    cp -f "${PREFIX}/bin/${name}" "${ROOT}/build_woa/${name}"
    echo "Installed ${ROOT}/build_woa/${name}"
  fi
done

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
