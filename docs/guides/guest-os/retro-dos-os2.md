# DOS, OS/2 and retro PCs

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

DOS and OS/2 run on an emulated 1990s PC. AQEMU runs them with TCG (emulation) on purpose: the wizard switches on **Force TCG** for MS-DOS, PC DOS, DR-DOS, OS/2, eComStation and ArcaOS. FreeDOS uses the `i386` emulator, which is TCG only anyway.

The Windows 1.x – 3.x, 95 and 98 profiles are in [Windows guests](windows.md).

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **Microsoft (DOS) or IBM (OS/2)** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## DOS

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **DR-DOS** | `qemu-system-i386` | `pc-i440fx` | `486` | 32 MB | 2 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **FreeDOS** | `qemu-system-i386` | `pc-i440fx` | `max` | 64 MB | 2 GB | `ne2k_pci` | TCG on every host (`i386` binary is TCG only) |
| **MS-DOS** | `qemu-system-i386` | `pc-i440fx` | `486` | 32 MB | 2 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |
| **PC DOS** | `qemu-system-i386` | `pc-i440fx` | `486` | 32 MB | 2 GB | `ne2k_pci` | TCG on every host (the wizard turns on **Force TCG**) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **DR-DOS:** DR-DOS → x86 (i386 PC) + Legacy ISA.
- **FreeDOS:** FreeDOS → i386 PC (IDE). Supports FAT32/LFN/CD-ROM better than MS-DOS. Prefer IBM PC 32Bit.
- **MS-DOS:** DOS → x86 (i386 PC) + Legacy ISA. 64-bit guests cannot run DOS.
- **PC DOS:** PC DOS → x86 (i386 PC) + Legacy ISA.

## OS/2 family

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **OS/2** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 512 MB | 4 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |
| **eComStation** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 512 MB | 8 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |
| **ArcaOS** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 512 MB | 8 GB | `rtl8139` | TCG on every host (the wizard turns on **Force TCG**) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **OS/2:** OS/2 Warp (proven AQEMU profile): i386 + pc, Force TCG / pentium3, cirrus, ACPI off, IDE (not VirtIO), rtl8139, ES1370, 512 MB, 1 CPU. LVM Error 9 usually means VirtIO/ACPI.
- **eComStation:** eComStation → i386 PC with IDE disk (not VirtIO), ACPI off.
- **ArcaOS:** ArcaOS → i386 PC with IDE disk (not VirtIO), ACPI off.

> The OS/2 profile in the wizard is just called **OS/2**. Its tip calls it "OS/2 Warp". There is no separate "OS/2 Warp 4" entry.


---

More: [All guest OS guides](README.md) · [Accelerators](../features/accelerators.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
