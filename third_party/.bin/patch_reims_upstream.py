#!/usr/bin/env python3
"""AQEMU patches for third_party/reims-vgpu (meson MAC_PVG + device alias)."""
from pathlib import Path
import sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
meson = root / "third_party/reims-vgpu/vendor/qemu/meson.build"
pci = root / "third_party/reims-vgpu/vendor/qemu/hw/display/apple-gfx-pci.c"

if meson.is_file():
    t = meson.read_text(encoding="utf-8")
    changed = False
    if "if host_os in ['darwin', 'linux']" in t and "dependency('reims-vgpu'" in t:
        t2 = t.replace(
            "if host_os in ['darwin', 'linux']",
            "if host_os in ['darwin', 'linux', 'windows']",
            1,
        )
        if t2 != t:
            t = t2
            changed = True
            print("Patched meson pvg host_os to include windows")
    old = """    if pvg.found() and (target in accelerator_targets.get('CONFIG_HVF', []) or
                        target in accelerator_targets.get('CONFIG_KVM', []))
      target_kconfig += 'CONFIG_MAC_PVG=y'
    endif"""
    new = """    # AQEMU: x86_64 softmmu always when the library is present (TCG hosts too).
    if pvg.found() and (target in accelerator_targets.get('CONFIG_HVF', []) or
                        target in accelerator_targets.get('CONFIG_KVM', []) or
                        target in accelerator_targets.get('CONFIG_WHPX', []) or
                        target == 'x86_64-softmmu')
      target_kconfig += 'CONFIG_MAC_PVG=y'
    endif"""
    if old in t:
        t = t.replace(old, new)
        changed = True
        print("Patched meson MAC_PVG gate for x86_64-softmmu")
    elif "target == 'x86_64-softmmu')" in t and "CONFIG_MAC_PVG=y" in t:
        print("meson MAC_PVG gate already allows x86_64-softmmu")
    else:
        print("WARN: could not patch meson MAC_PVG gate")
    if changed:
        meson.write_text(t, encoding="utf-8", newline="\n")
else:
    print("WARN: missing", meson)

if pci.is_file():
    t = pci.read_text(encoding="utf-8")
    if "reims-vgpu-pci" in t:
        print("reims-vgpu-pci alias already present")
    else:
        anchor = "DEFINE_TYPES(apple_gfx_pci_types)"
        alias = """    /* AQEMU uses this name for the same PCI device. */
    {
        .name   = "reims-vgpu-pci",
        .parent = TYPE_APPLE_GFX_PCI,
    },
"""
        close = t.rfind("};\n" + anchor)
        if close < 0:
            close = t.rfind("};\r\n" + anchor)
        if close < 0:
            print("WARN: apple-gfx-pci.c shape changed; alias not applied")
        else:
            pci.write_text(t[:close] + alias + t[close:], encoding="utf-8", newline="\n")
            print("Added reims-vgpu-pci type alias")
else:
    print("WARN: missing", pci)
