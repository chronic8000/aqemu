# User Guide: Getting Started and the New VM Wizard

Welcome to AQEMU! This guide walks you through the first start, the main window, and the **New Virtual Machine Wizard**, which is how most VMs get made. Every label below is the exact text you'll see in the app.

**Contents**

1. [First start](#1-first-start)
2. [The main window](#2-the-main-window)
3. [The New Virtual Machine Wizard](#3-the-new-virtual-machine-wizard)
4. [The settings tabs](#4-the-settings-tabs)
5. [Advanced Options and SMP Settings](#5-advanced-options-and-smp-settings)
6. [Seeing what AQEMU sends to QEMU](#6-seeing-what-aqemu-sends-to-qemu)
7. [Host platforms](#7-host-platforms)

---

## 1. First start

The first time you run AQEMU, the **First Start Wizard** asks three things:

1. **Choose interface language.**
2. **The folder for virtual machine configurations.** Every VM is saved there as a `.aqemu` file.
3. **Where QEMU lives.** Pick one:
   - **Use the QEMU binaries that ship with AQEMU (Recommended)**
   - **Use system-installed QEMU**
   - **Specify a custom QEMU executable or folder**

You can change all of this later in `File → Preferences…`.

---

## 2. The main window

The VM list is on the left and the selected VM's settings are on the right.

**Menus**

| Menu | What's in it |
| --- | --- |
| **File** | Disks, Import and Export (OVA/OVF appliances), Host tools, **Preferences…** |
| **VM** | **Add VM** (→ **Add New VM**, `Ctrl+N`), Save As Template, Start / Pause / Shutdown / Power Off / Reset, plus the **Apple**, **Recovery**, **Storage**, **Tools** and **Lab** submenus |
| **Help** | **About AQEMU**, **QEMU Help Browser…**, and the **Diagnostics** submenu |

The toolbar gives you one-click **Add New VM**, **Start**, **Pause**, **Shutdown**, **Power Off**, **Reset**, **Manage Snapshots** and **Show QEMU Arguments**.

---

## 3. The New Virtual Machine Wizard

Open it with `VM → Add VM → Add New VM` (or `Ctrl+N`, or the toolbar). The first page asks **how you want to create this VM**:

| Method | Use it when |
| --- | --- |
| **Guest Operating System** | You know what you want to install (Windows 11, Ubuntu, Mac OS 9, IRIX…). Pick a vendor, then the OS. |
| **System / Machine Platform** | You want a particular machine (Raspberry Pi 3, IBM PC Compatible, Sun, SGI…). |
| **CPU Architecture** | You want a CPU family first (x86-64, ARM64, PowerPC, RISC-V, s390x…), then a machine. |
| **Custom / Advanced** | The classic **Typical** / **Custom** / **Template** flow. This is where the **Windows 11 ARM** template lives. |
| **Import Existing Disk** | You already have a disk image (qcow2, raw, VHDX, VMDK…). |
| **Import Virtual Appliance (OVA / OVF)** | You have an appliance exported from another hypervisor. |
| **Recover storage** | You need to get data off a damaged or orphaned disk. |

The **Guest Operating System** list is grouped by vendor: Lab, Apple, BSD, DEC, HP, IBM, Linux, Microsoft, Other, SCO, SGI, Sun, TrueNAS / Storage, and Consoles & Retro. The **CPU Architecture** list covers 28 architecture families, backed by 29 QEMU system emulators.

### After you choose

The first three paths end on a **Confirm QEMU system** page, where the Computer Type (the `qemu-system-*` binary) and Machine Type are still editable. Then the wizard walks through:

**Accelerator → Name / CPU → Disk → Devices → Network → Finish**

The **Devices** page is guest-aware. AQEMU picks sensible devices for the OS you chose (for example IDE, NE2000, Sound Blaster 16 and Cirrus for Windows 98) and hides devices that won't work for that guest unless you tick **Show all QEMU options**. See [New VM Wizard paths](../reference/new-vm-wizard-paths.md) for the full mapping.

On the **Disk** page you choose **Create a new disk image** or **Use an existing disk image**. The install media page offers **Local ISO / image file**, **Download ISO from URL**, or **Network install (kernel + initrd URLs)**.

> **Everything stays editable.** The wizard just fills in a starting point. Every setting can be changed on the main window afterwards.

---

## 4. The settings tabs

Select a VM and you'll see these tabs:

| Tab | What you set there |
| --- | --- |
| **Info** | A summary of the selected VM |
| **Machine** | Name, Architecture, CPU Model, CPUs, Accelerator, Boot order, Graphics, Resolution, Keyboard, Pointer mode, USB, Memory, Audio, disk Interface, and the **Advanced** button. Windows 11 ARM and Intel macOS VMs get their own panel here too. |
| **Media** | Hard disks, CD/DVD and floppy images, plus **Boot Kernel** (direct kernel/initrd boot) and **Special Images** |
| **Display** | Display settings, with **VNC Remote** and **QEMU Window** sub-tabs. While a VM is running, its live screen also opens as a Display tab. |
| **Network** | Network cards, **Redirections** (port forwards), and **TFTP/SAMBA** |
| **Storage** | **Enable TrueNAS Guest Integration**: connect to a TrueNAS host (address, port, API token) and see its pools |

**Accelerator** choices on the Machine tab: **TCG** (pure emulation, works everywhere), **WHPX** (Windows Hypervisor Platform, on Windows hosts), **KVM** (Linux hosts, and Linux QEMU inside WSL), and **XEN**.

**Disk Interface** choices: VirtIO, VirtIO SCSI, SCSI, IDE, AHCI (SATA), SD and NVMe. The **Apply VirtIO defaults** button switches a VM to VirtIO in one click.

**Pointer mode:** *Seamless* uses an absolute tablet, so the host and guest cursors line up with no mouse grab. *Relative* is a PS/2-style mouse: click the screen to capture and middle-click or Escape to release. Use Relative for Windows 9x and other guests with no tablet driver.

---

## 5. Advanced Options and SMP Settings

**SMP Settings.** Click the **…** button next to **CPUs** to set **CPU count**, **Socket count**, **Cores per socket**, **Threads per core** and **Maximum number of hotpluggable CPUs**.

**Boot Order.** The **…** button next to **Boot** lets you reorder devices and set **Enable interactive boot menu**, **Boot menu wait (seconds)** and **Show boot logo**.

**Advanced Options.** The **Advanced** button on the Machine tab opens the power-user settings, including:

- **Force pure TCG (Win9x / pre-ME)**
- **VirtIO balloon**, **VirtIO RNG**, **VirtIO keyboard**
- **No default devices (-nodefaults)**, **Guest RTC clock**, **IOThread for VirtIO disks**, **Modern -netdev networking**
- UUID, BIOS image, **OVMF CODE** / **OVMF VARS** (UEFI firmware), NUMA, watchdog
- **TPM:** none, emulator (swtpm socket), or passthrough
- **Custom QEMU Command Line Arguments**, if you want to add your own flags

---

## 6. Seeing what AQEMU sends to QEMU

Want to see exactly what's going on? `VM → Tools → Show QEMU Arguments` (also on the toolbar) shows the full QEMU command line for the selected VM. `VM → Tools → Create Shell Script` saves it as a script you can run yourself.

AQEMU also works from a terminal:

```
aqemu list
aqemu start "My VM"
aqemu stop "My VM"
```

---

## 7. Host platforms

| Host | Accelerators | Notes |
| --- | --- | --- |
| **Windows 10/11 x64** | WHPX, TCG; KVM through WSL | Available from the [Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs_user_guide) |
| **Windows 11 on ARM** | WHPX, TCG | For AArch64 guests under WHPX, AQEMU uses `-cpu max` and needs the AAVMF UEFI firmware |
| **Linux x86_64 / ARM64** | KVM, TCG | Build from [source](https://github.com/chronic8000/aqemu) |

Next: [Windows 11 (ARM and x86) and Legacy Windows](windows-11.md) · [Operating System Catalog](os-catalog.md) · [Documentation Hub](../README.md)
