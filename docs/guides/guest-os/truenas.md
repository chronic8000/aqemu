# TrueNAS and storage labs

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

The wizard's **TrueNAS / Storage** branch has three TrueNAS installs and three ready-made storage labs. The TrueNAS profiles default to 4096-byte (4Kn) disks, as their tips explain. Reading a set of real NAS disks safely is a separate wizard method: **Recover storage** (see [The New VM Wizard](../features/new-vm-wizard.md#7-recover-storage)).

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **TrueNAS / Storage** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## TrueNAS

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **TrueNAS CORE** | `qemu-system-x86_64` | `q35` | `max` | 8 GB | 32 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **TrueNAS SCALE** | `qemu-system-x86_64` | `q35` | `max` | 8 GB | 32 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **TrueNAS SCALE (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 8 GB | 32 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **TrueNAS CORE:** TrueNAS CORE (FreeBSD/ZFS) → x86_64 + Q35 with VirtIO. Defaults to 4096-byte (4Kn) native sector size for optimal ZFS performance.
- **TrueNAS SCALE:** TrueNAS SCALE (Linux/Debian/ZFS) → x86_64 + Q35 with VirtIO. Defaults to 4096-byte (4Kn) native sector size for optimal OpenZFS performance.
- **TrueNAS SCALE (ARM64):** TrueNAS SCALE (ARM64 / Linux / ZFS) → AArch64 + virt with VirtIO. Defaults to 4096-byte (4Kn) native sector size for optimal OpenZFS performance.

## Storage labs

These create empty disks or empty VMs for practice. None of them creates a pool or installs an OS for you.

- **TrueNAS lab:** TrueNAS lab: 8 GB RAM, UEFI, four empty 4Kn VirtIO-SCSI disks with unique serials, and a VirtIO NIC. No pool is created. Boot your own TrueNAS ISO.
- **Folder-backed disks:** Creates empty 4Kn disks in a folder you choose and attaches them with VirtIO-SCSI or NVMe. No pool is created.
- **Storage network:** Four empty VMs on one loopback multicast network: two TrueNAS nodes, a Linux client, and a Windows client. No operating system and no pool is created.

---

More: [All guest OS guides](README.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
