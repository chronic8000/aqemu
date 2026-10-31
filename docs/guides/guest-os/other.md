# Other operating systems

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Everything in the wizard's **Other** branch: Haiku, ReactOS, ChromeOS / Chrome OS Flex, SteamOS, Android, Plan 9 / 9front, hobby OSes and more.

If your OS isn't listed anywhere, pick **Other**: "Generic default from host architecture. Computer Type remains fully editable."

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **Other** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## Other

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **9front** | `qemu-system-x86_64` | `pc` | `max` | 1 GB | 10 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **AmigaOS** | `qemu-system-ppc` | `amigaone` | `g4` | 512 MB | 10 GB | `e1000` | TCG on every host |
| **Android** | `qemu-system-aarch64` | `virt` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **BeOS** | `qemu-system-i386` | `pc` | `pentium3` | 256 MB | 8 GB | `rtl8139` | TCG on every host (`i386` binary is TCG only) |
| **Chrome OS Flex** | `qemu-system-x86_64` | `q35` | `max` | 8 GB | 40 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **ChromeOS** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Fuchsia** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Haiku (32-bit)** | `qemu-system-i386` | `q35` | `max` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Haiku (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **KolibriOS** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 128 MB | 2 GB | `ne2k_pci` | TCG on every host (`i386` binary is TCG only) |
| **MenuetOS** | `qemu-system-x86_64` | `pc` | `max` | 256 MB | 1 GB | `rtl8139` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Minix** | `qemu-system-i386` | `pc` | `max` | 256 MB | 4 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **MorphOS** | `qemu-system-ppc` | `sam460ex` | `g4` | 512 MB | 10 GB | `e1000` | TCG on every host |
| **Plan 9** | `qemu-system-x86_64` | `pc` | `max` | 1 GB | 10 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **QNX** | `qemu-system-x86_64` | `pc` | `max` | 1 GB | 10 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **ReactOS (32-bit)** | `qemu-system-i386` | `pc-i440fx` | `pentium3` | 1 GB | 4 GB | `e1000` | TCG on every host (the wizard turns on **Force TCG**) |
| **ReactOS (64-bit)** | `qemu-system-x86_64` | `pc-i440fx` | `qemu64` | 4 GB | 8 GB | `e1000` | TCG on every host (the wizard turns on **Force TCG**) |
| **Redox OS** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 10 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **RISC OS** | `qemu-system-arm` | `virt` | `max` | 256 MB | 4 GB | `virtio-net-pci` | TCG on every host (`arm` binary is TCG only) |
| **SerenityOS** | `qemu-system-x86_64` | `q35` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **SteamOS** | `qemu-system-x86_64` | `q35` | `max` | 8 GB | 64 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **TempleOS** | `qemu-system-i386` | `pc` | `max` | 512 MB | 2 GB | `rtl8139` | TCG on every host (`i386` binary is TCG only) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **9front:** 9front (Plan 9 fork) → x86_64 + pc, std VGA, IDE, e1000. Attach 9front ISO; networking is central.
- **AmigaOS:** AmigaOS → PowerPC / amigaone. Requires qemu-system-ppc with amigaone machine support.
- **Android:** Android → AArch64 virt + VirtIO.
- **BeOS:** BeOS → x86 (i386 PC). Use a BeOS-compatible disk/ISO.
- **Chrome OS Flex:** Chrome OS Flex: extract recovery .bin, attach as raw disk (not ISO), install to VirtIO/qcow2. Prefer VirtIO VGA; on Linux/KVM try VirtIO VGA (GL). Not official in VMs.
- **ChromeOS:** ChromeOS (Brunch/Chromium OS style) → x86_64 + Q35. For Google’s Flex recovery .bin use “Chrome OS Flex” instead.
- **Fuchsia:** Fuchsia → x86_64 + Q35/VirtIO. Official workflow often uses Google ffx + QEMU; AQEMU gives a starting preset.
- **Haiku (32-bit):** Haiku 32-bit → i386 PC.
- **Haiku (64-bit):** Haiku → x86_64 (PC) + Q35 with VirtIO.
- **KolibriOS:** KolibriOS → x86 (i386 PC).
- **MenuetOS:** MenuetOS 64-bit → qemu-system-x86_64. Boot from floppy image (FDA) or ISO; distinct from 32-bit KolibriOS.
- **Minix:** Minix 3 → i386 PC. IDE disk recommended for classic images.
- **MorphOS:** MorphOS → PowerPC / sam460ex. Requires qemu-system-ppc with sam460ex.
- **Plan 9:** Plan 9 → x86_64 + pc, std VGA, IDE, e1000. Prefer 9front for an actively maintained fork.
- **QNX:** QNX Neutrino → x86_64 PC. Timer-sensitive: AQEMU adds -rtc clock=vm. Legacy QNX 4 may need i386.
- **ReactOS (32-bit):** ReactOS 32-bit (proven AQEMU profile): i386 + pc, Force TCG / pentium3, std VGA, IDE (classic -hda), e1000, AC97, usb-tablet, native display, 1 GB, 1 CPU.
- **ReactOS (64-bit):** ReactOS 64-bit (experimental builds) → x86_64 PC. Prefer 32-bit for stable ISOs.
- **Redox OS:** Redox OS (Rust microkernel) → x86_64 + Q35, std VGA. Use official Redox images.
- **RISC OS:** RISC OS → ARM virt. Needs a specialized RISC OS image; machine choice may need tuning.
- **SerenityOS:** SerenityOS → x86_64 (PC) + Q35 with VirtIO.
- **SteamOS:** SteamOS recovery (.bz2→.img): UEFI + NVMe install disk required (/dev/nvme0n1). After install, force Plasma before reboot — Gaming Mode needs Deck AMD GPU (passthrough optional).
- **TempleOS:** TempleOS → x86 (i386 PC).


---

More: [All guest OS guides](README.md) · [Accelerators](../features/accelerators.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
