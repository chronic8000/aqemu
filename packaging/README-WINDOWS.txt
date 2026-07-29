AQEMU 1.1.0 — Windows portable (x64)
====================================

Updated build: 2026-07-29 (full QEMU probe catalogs + wizard CPU defaults).

Please file bugs:
  https://github.com/chronic8000/aqemu/issues

Run:  aqemu.exe

This zip includes:
  - AQEMU 1.1.0 (Qt5 + embedded SPICE)
  - QEMU 11.0.2 (full softmmu set + qemu-img)
  - UEFI/BIOS firmware under share/
  - OpenPartitionDxe.efi (Intel macOS OpenCore prep)
  - qemu_machine_catalog.json (New VM wizard machine list)
  - qemu_probe_full_v3/*.json (full per-arch QEMU option lists for VM config)

What's new in this zip:
  - Main Window / Custom: full machines, CPUs, NICs, display devices per arch from probes
  - Wizard: curated OS defaults with probe-validated CPUs (no more 486 for Ubuntu)
  - Architecture switch refreshes full option lists without re-running the wizard

Notes:
  - GitHub Release zips are NOT auto-updated. Grab a newer zip when we publish one.
