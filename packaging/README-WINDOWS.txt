AQEMU 1.5.0 — Windows portable (x64)
====================================

Updated build: 2026-10-01 (QEMU Lab; version read from VERSION.txt).

Please file bugs:
  https://github.com/chronic8000/aqemu/issues

Run:  aqemu.exe

This zip includes:
  - AQEMU 1.5.0 (Qt5 + embedded SPICE)
  - QEMU 11.0.2 (full softmmu set + qemu-img)
  - UEFI/BIOS firmware under share/
  - OpenPartitionDxe.efi (Intel macOS OpenCore prep)
  - qemu_machine_catalog.json (New VM wizard machine list)
  - qemu_probe_full_v3/*.json (full per-arch QEMU option lists for VM config)

What's new in 1.5.0:
  - VM → Lab and a Network port table (SSH, RDP, HTTP presets)
  - Help → Support bundle (zip; OSK, IPSW, and secret arguments redacted)
  - Snapshot timeline, Chain Studio, and Windows 11 ARM TPM / Secure Boot
  - Bring-your-own xemu, Android SDK, and OpenCore. No ROMs or disk images included
  - Full guide: docs/LAB_1.5.0.md

Still in this zip from 1.4.x:
  - Apple SoC / Inferno iOS path via WSL + companion restore UI
  - Reims path via WSL (AMD and NVIDIA Vulkan)
  - Main Window / Custom: full machines, CPUs, NICs, display devices per arch from probes
  - Wizard: curated OS defaults with probe-validated CPUs

Notes:
  - GitHub Release zips are NOT auto-updated. Grab a newer zip when we publish one.
