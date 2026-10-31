# Linux guests

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

The wizard's **Linux** branch is split three ways: **ARM64 / AArch64 (Modern ARM)**, **x86_64 (64-bit PC)** and **x86 32-bit (Legacy PC)**. Pick the one that matches your installer *and* your PC if you want speed: x86_64 distros are fast on x64 PCs; ARM64 distros are fast on ARM64 PCs (Windows on ARM, Raspberry Pi 5 and other ARM64 Linux machines).

TrueNAS SCALE is in [TrueNAS and storage labs](truenas.md).

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **Linux** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## x86_64 (64-bit PC)

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **AlmaLinux** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Alpine Linux (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 1 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Arch Linux (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **CentOS Stream** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Debian (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **elementary OS** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Fedora (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Generic Linux (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Gentoo** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Kali Linux** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Linux Mint (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **NixOS** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **openSUSE (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Pop!_OS** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 40 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **RHEL** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Rocky Linux** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Slackware** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **SUSE Linux** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Tiny Core Linux (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 512 MB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Ubuntu (64-bit)** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Void Linux** | `qemu-system-x86_64` | `q35` | `max` | 4 GB | 20 GB | `virtio-net-pci` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **AlmaLinux:** AlmaLinux → x86_64 (PC) + Q35 with VirtIO.
- **Alpine Linux (64-bit):** Alpine Linux → x86_64 (PC) + Q35 with VirtIO.
- **Arch Linux (64-bit):** Arch Linux → x86_64 (PC) + Q35 with VirtIO.
- **CentOS Stream:** CentOS Stream → x86_64 + Q35 with VirtIO (RHEL upstream).
- **Debian (64-bit):** Debian → x86_64 (PC) + Q35 with VirtIO.
- **elementary OS:** elementary OS → x86_64 + Q35 with VirtIO.
- **Fedora (64-bit):** Fedora → x86_64 (PC) + Q35 with VirtIO.
- **Generic Linux (64-bit):** Generic Linux → x86_64 (PC) + Q35 with VirtIO. Change Computer Type for other arches.
- **Gentoo:** Gentoo → x86_64 (PC) + Q35 with VirtIO.
- **Kali Linux:** Kali Linux → x86_64 (PC) + Q35 with VirtIO.
- **Linux Mint (64-bit):** Linux Mint → x86_64 (PC) + Q35 with VirtIO.
- **NixOS:** NixOS → x86_64 + Q35 + VirtIO. UEFI/OVMF recommended for modern installers.
- **openSUSE (64-bit):** openSUSE → x86_64 (PC) + Q35 with VirtIO.
- **Pop!_OS:** Pop!_OS → x86_64 + Q35. Prefer VirtIO VGA (GL) on Linux/KVM for COSMIC/GNOME.
- **RHEL:** RHEL → x86_64 (PC) + Q35 with VirtIO.
- **Rocky Linux:** Rocky Linux → x86_64 (PC) + Q35 with VirtIO.
- **Slackware:** Slackware → x86_64 (PC) + Q35 with VirtIO.
- **SUSE Linux:** SUSE Linux → x86_64 (PC) + Q35 with VirtIO.
- **Tiny Core Linux (64-bit):** Tiny Core Linux → x86_64 (PC) + Q35 with VirtIO.
- **Ubuntu (64-bit):** Ubuntu desktop → x86_64 + Q35, 4 GB RAM recommended (GNOME live/install fails on 2 GB under TCG). Prefer WHPX/KVM; CPU max.
- **Void Linux:** Void Linux → x86_64 + Q35 with VirtIO.

## ARM64 / AArch64 (Modern ARM)

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **AlmaLinux (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Alpine Linux (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 1 GB | 10 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Arch Linux ARM (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **CentOS Stream (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Debian (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Fedora (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Generic Linux (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 2 GB | 20 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Gentoo (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 30 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Kali Linux (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 30 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **NixOS (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 30 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **openSUSE (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Pop!_OS (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Raspberry Pi OS (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 2 GB | 16 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Rocky Linux (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Ubuntu (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 4 GB | 25 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |
| **Void Linux (ARM64)** | `qemu-system-aarch64` | `virt` | `max` | 2 GB | 15 GB | `virtio-net-pci` | WHPX on Windows ARM64, KVM on Linux ARM64. TCG on x64 hosts |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **AlmaLinux (ARM64):** AlmaLinux (ARM64 / AArch64) → virt + VirtIO, enterprise RHEL-compatible.
- **Alpine Linux (ARM64):** Alpine Linux (ARM64 / AArch64) → virt + VirtIO, lightweight Alpine for ARM.
- **Arch Linux ARM (ARM64):** Arch Linux ARM (AArch64) → virt + VirtIO, UEFI firmware. Uses KVM native acceleration on ARM hosts.
- **CentOS Stream (ARM64):** CentOS Stream (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.
- **Debian (ARM64):** Debian (ARM64 / AArch64) → virt + VirtIO, UEFI firmware. Uses KVM native acceleration on ARM hosts.
- **Fedora (ARM64):** Fedora (ARM64 / AArch64) → virt + VirtIO, UEFI firmware. Uses KVM native acceleration on ARM hosts.
- **Generic Linux (ARM64):** Generic Linux (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.
- **Gentoo (ARM64):** Gentoo (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.
- **Kali Linux (ARM64):** Kali Linux (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.
- **NixOS (ARM64):** NixOS (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.
- **openSUSE (ARM64):** openSUSE Leap / Tumbleweed (ARM64) → virt + VirtIO, UEFI firmware.
- **Pop!_OS (ARM64):** Pop!_OS (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.
- **Raspberry Pi OS (ARM64):** Raspberry Pi OS (ARM64 / 64-bit) → virt + VirtIO, Debian-based.
- **Rocky Linux (ARM64):** Rocky Linux (ARM64 / AArch64) → virt + VirtIO, enterprise RHEL-compatible.
- **Ubuntu (ARM64):** Ubuntu (ARM64 / AArch64) → virt + VirtIO, UEFI firmware. Uses KVM native acceleration on ARM hosts.
- **Void Linux (ARM64):** Void Linux (ARM64 / AArch64) → virt + VirtIO, UEFI firmware.

## x86 32-bit (Legacy PC)

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Alpine Linux (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 512 MB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Arch Linux (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Debian (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Fedora (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Generic Linux (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Linux Mint (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **openSUSE (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Tiny Core Linux (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 256 MB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |
| **Ubuntu (32-bit)** | `qemu-system-i386` | `q35` | `qemu32` | 1 GB | 10 GB | `virtio-net-pci` | TCG on every host (`i386` binary is TCG only) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **Alpine Linux (32-bit):** Alpine Linux 32-bit → i386 PC. Prefer for older ISOs.
- **Arch Linux (32-bit):** Arch Linux 32-bit → i386 PC. Prefer for older ISOs.
- **Debian (32-bit):** Debian 32-bit → i386 PC. Prefer for older ISOs.
- **Fedora (32-bit):** Fedora 32-bit → i386 PC. Prefer for older ISOs.
- **Generic Linux (32-bit):** Generic Linux 32-bit → i386 PC. Prefer for older ISOs.
- **Linux Mint (32-bit):** Linux Mint 32-bit → i386 PC. Prefer for older ISOs.
- **openSUSE (32-bit):** openSUSE 32-bit → i386 PC. Prefer for older ISOs.
- **Tiny Core Linux (32-bit):** Tiny Core Linux 32-bit → i386 PC. Prefer for older ISOs.
- **Ubuntu (32-bit):** Ubuntu 32-bit → i386 PC. Prefer for older ISOs.

## Linux on other hardware

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Linux on IBM Z** | `qemu-system-s390x` | `s390-ccw-virtio` | `max` | 4 GB | 20 GB | `virtio-net-ccw` | TCG on every host |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **Linux on IBM Z:** Linux on IBM Z → qemu-system-s390x + s390-ccw-virtio. VirtIO CCW disk/net; typically headless (no VGA).

> **Linux on IBM Z** lives under the **IBM** branch in the wizard.


---

More: [All guest OS guides](README.md) · [Accelerators](../features/accelerators.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
