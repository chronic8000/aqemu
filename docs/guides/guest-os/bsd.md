# BSD guests

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

FreeBSD, NetBSD, OpenBSD, DragonFly BSD and GhostBSD all have wizard profiles. The 64-bit ones are fast on x64 PCs. TrueNAS CORE (FreeBSD-based) is in [TrueNAS and storage labs](truenas.md).

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **BSD** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## BSD

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **DragonFly BSD** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **FreeBSD (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **FreeBSD (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **GhostBSD** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **NetBSD (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **NetBSD (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **OpenBSD (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **OpenBSD (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **DragonFly BSD:** DragonFly BSD → x86_64 (PC) + Q35 with VirtIO.
- **FreeBSD (32-bit):** FreeBSD 32-bit → i386 PC.
- **FreeBSD (64-bit):** FreeBSD → x86_64 (PC) + Q35 with VirtIO.
- **GhostBSD:** GhostBSD → x86_64 + Q35 with VirtIO (FreeBSD-based desktop).
- **NetBSD (32-bit):** NetBSD 32-bit → i386 PC.
- **NetBSD (64-bit):** NetBSD → x86_64 (PC) + Q35 with VirtIO.
- **OpenBSD (32-bit):** OpenBSD 32-bit → i386 PC.
- **OpenBSD (64-bit):** OpenBSD → x86_64 (PC) + Q35 with VirtIO.


---

More: [All guest OS guides](README.md) · [Accelerators](../features/accelerators.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
