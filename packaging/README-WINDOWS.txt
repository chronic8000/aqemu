AQEMU 1.0.0 — Windows portable (x64)
====================================

Updated build: 2026-07-25 (wizard + virt-manager-inspired UX refresh).

Please file bugs:
  https://github.com/chronic8000/aqemu/issues

Run:  aqemu.exe

This zip includes:
  - AQEMU 1.0.0 (Qt5 + embedded SPICE)
  - QEMU 11.0.2 (qemu-system-x86_64, i386, aarch64, qemu-img)
  - UEFI/BIOS firmware under share/
  - OpenPartitionDxe.efi (Intel macOS OpenCore prep)
  - qemu_machine_catalog.json (New VM wizard machine list)

What's new in this zip (vs the first 1.0.0 drop):
  - New VM wizard: Platform / Architecture paths, guest-aware devices
  - Import disk, ISO OS-guess, URL / kernel+initrd network install
  - File → Storage Browser…, File → Remote Hosts…
  - Serial console + live migrate URI dialog in the session toolbar

Notes:
  - GitHub Release zips are NOT auto-updated. Grab a newer zip when we publish one.
  - Microsoft Store (pending certification) will be the update channel for Store installs.
  - You must supply your own OS ISOs / disks / OpenCore / OSK where required.
  - License: GNU GPLv2 — source at https://github.com/chronic8000/aqemu
  - Privacy: https://github.com/chronic8000/aqemu/blob/master/PRIVACY.md

If Windows SmartScreen warns on first run, use More info → Run anyway (unsigned portable build).
