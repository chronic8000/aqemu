AQEMU 1.5.2 — Windows portable (x64)
====================================

Updated build: 2026-10-03 (version read from VERSION.txt).

Please file bugs:
  https://github.com/chronic8000/aqemu/issues

Run:  aqemu.exe

This zip includes:
  - AQEMU 1.5.2 (Qt5 + embedded SPICE)
  - QEMU 11.0.2 (full softmmu set + qemu-img)
  - UEFI/BIOS firmware under share/
  - OpenPartitionDxe.efi (Intel macOS OpenCore prep)
  - qemu_machine_catalog.json (New VM wizard machine list)
  - qemu_probe_full_v3/*.json (full per-arch QEMU option lists for VM config)

What's new in 1.5.2:
  - File, VM, and Help commands are grouped. Exit is the last File item
  - Apple commands live under VM → Apple. Windows-only items are grey on other hosts
  - Exit asks for confirmation. Preferences can turn that prompt off
  - Dark mode follows the OS theme. Disabled menu items draw in grey
  - Full guide: docs/LAB_1.5.2.md
  - SeaBIOS boot logo on 32-bit and 64-bit PCs, with a switch in settings and on each machine
  - OVF import maps old Windows guests to the right profile and i386 PC
  - Host audio follows the QEMU binary. The Windows package prefers spice
  - VM → Lab, the Network port table, support bundle, snapshot timeline, and Chain Studio

Still in this zip:
  - Apple SoC / Inferno iOS path via WSL + companion restore UI
  - Reims path via WSL (AMD and NVIDIA Vulkan)
  - Main Window / Custom: full machines, CPUs, NICs, display devices per arch from probes
  - Wizard: curated OS defaults with probe-validated CPUs

Notes:
  - GitHub Release zips are NOT auto-updated. Grab a newer zip when we publish one.
