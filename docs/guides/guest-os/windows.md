# Windows guests (Windows 1.x to Windows Server 2025)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

AQEMU's wizard has a profile for every Windows release below, from Windows 1.x to Windows Server 2025. 64-bit Windows runs fast on an x64 PC (WHPX on Windows, KVM on Linux). 32-bit and older Windows use the `qemu-system-i386` emulator, which AQEMU runs with TCG (emulation), so they are slower but very compatible.

- Windows 11 on an ARM PC: [Windows 11 on ARM, host by host](windows-11-arm.md)
- Windows 11 / legacy Windows walkthrough: [Windows 11 and Legacy Windows](../windows-11.md)
- DOS: [DOS, OS/2 and retro PCs](retro-dos-os2.md)

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **Microsoft** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## Classic Windows (1.x – ME, NT)

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Windows 1.x** | `qemu-system-i386` | `pc-i440fx` | `486` | 16 MB | 2 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows 2.x** | `qemu-system-i386` | `pc-i440fx` | `486` | 16 MB | 2 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows 3.x** | `qemu-system-i386` | `pc-i440fx` | `486` | 16 MB | 2 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows 95** | `qemu-system-i386` | `pc-i440fx` | `pentium2` | 128 MB | 8 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows 98** | `qemu-system-i386` | `pc-i440fx` | `pentium2` | 128 MB | 8 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows ME** | `qemu-system-i386` | `pc-i440fx` | `pentium2` | 256 MB | 8 GB | `ne2k_pci` | TCG on every host (`i386` binary is TCG only) |
| **Windows NT 3.x** | `qemu-system-i386` | `q35` | `pentium2` | 128 MB | 8 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows NT 4.0** | `qemu-system-i386` | `q35` | `pentium2` | 128 MB | 8 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows 2000** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 512 MB | 16 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **Windows 1.x:** Windows 1.x needs x86 (i386 PC) / ISA PC. Cannot run as an x86_64 guest.
- **Windows 2.x:** Windows 2.x needs x86 (i386 PC) / ISA PC. Cannot run as an x86_64 guest.
- **Windows 3.x:** Windows 3.x needs x86 (i386 PC) / ISA PC. Cannot run as an x86_64 guest.
- **Windows 95:** Windows 95 → x86 (i386 PC) + pc (i440FX). Force TCG on Windows hosts; CPU pentium2; cirrus; ACPI off.
- **Windows 98:** Windows 98 (proven AQEMU profile): i386 + pc, Force TCG / pentium2, cirrus, ACPI off, IDE disk, ne2k_pci, sb16, 128 MB, 1 CPU.
- **Windows ME:** Windows ME → x86 (i386 PC) + pc (i440FX). Not forced to pure TCG (unlike 95/98).
- **Windows NT 3.x:** Windows NT 3.x → x86 (i386 PC). Avoid x86_64.
- **Windows NT 4.0:** Windows NT 4.0 → x86 (i386 PC). Avoid x86_64.
- **Windows 2000:** Windows 2000 → x86 (i386 PC) + pc. Win2K disk hack can help.

## Windows XP – Windows 11

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Windows XP (32-bit)** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 1 GB | 20 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows XP (64-bit)** | `qemu-system-x86_64` | `pc-i440fx` | `qemu64` | 2 GB | 40 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows Vista (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 40 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows Vista (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows 7 (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 2 GB | 40 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows 7 (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows 8 (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 2 GB | 40 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows 8 (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows 8.1 (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 2 GB | 40 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows 8.1 (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows 10 (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 2 GB | 40 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows 10 (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows 11** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `virtio-net-pci` | x64 hosts: WHPX / KVM. ARM hosts: switches to ARM64 ([guide](windows-11-arm.md)) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **Windows XP (32-bit):** Windows XP 32-bit (proven AQEMU profile): i386 + pc, Force TCG / pentium3, std VGA, ACPI off, IDE, rtl8139, AC97, 1 GB, 1 CPU. Avoid WHPX (black text-mode setup).
- **Windows XP (64-bit):** Windows XP x64 Edition → qemu-system-x86_64 + pc (i440FX).
- **Windows Vista (32-bit):** Windows Vista 32-bit → i386 PC.
- **Windows Vista (64-bit):** Windows Vista 64-bit → x86_64 + Q35.
- **Windows 7 (32-bit):** Windows 7 32-bit → i386 PC.
- **Windows 7 (64-bit):** Windows 7 64-bit → x86_64 + Q35.
- **Windows 8 (32-bit):** Windows 8 32-bit → i386 PC.
- **Windows 8 (64-bit):** Windows 8 64-bit → x86_64 + Q35.
- **Windows 8.1 (32-bit):** Windows 8.1 32-bit → i386 PC.
- **Windows 8.1 (64-bit):** Windows 8.1 64-bit → x86_64 + Q35.
- **Windows 10 (32-bit):** Windows 10 32-bit → i386 PC (legacy; prefer 64-bit).
- **Windows 10 (64-bit):** Windows 10 64-bit → x86_64 + Q35.
- **Windows 11:** Windows 11 → x86_64 (PC) + Q35. On ARM hosts AQEMU switches to AArch64 + virt.

## Windows Server

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Windows Server 2000** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 1 GB | 20 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows Server 2003** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 1 GB | 20 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |
| **Windows Server 2008 (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 2 GB | 40 GB | `e1000e` | TCG on every host (`i386` binary is TCG only) |
| **Windows Server 2008 (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows Server 2012** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows Server 2016** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows Server 2019** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `e1000e` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows Server 2022** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Windows Server 2025** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **Windows Server 2000:** Windows Server 2000 → x86 (i386 PC).
- **Windows Server 2003:** Windows Server 2003 → x86 (i386 PC).
- **Windows Server 2008 (32-bit):** Windows Server 2008 32-bit → i386 PC.
- **Windows Server 2008 (64-bit):** Windows Server 2008 64-bit → x86_64 + Q35.
- **Windows Server 2012:** Windows Server 2012 → x86_64 (PC) + Q35.
- **Windows Server 2016:** Windows Server 2016 → x86_64 (PC) + Q35.
- **Windows Server 2019:** Windows Server 2019 → x86_64 (PC) + Q35.
- **Windows Server 2022:** Windows Server 2022 → x86_64 (PC) + Q35.
- **Windows Server 2025:** Windows Server 2025 → x86_64 (PC) + Q35.


---

More: [All guest OS guides](README.md) · [Accelerators](../features/accelerators.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
