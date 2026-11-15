# Preferences

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

**File → Preferences…** holds settings for AQEMU itself, not for one VM. The First Start Wizard sets several of them for you the first time you open AQEMU. For per-VM settings, see the [Settings tab map](README.md).

---

## Emulator

**QEMU installation:**

- **Use built-in portable QEMU (recommended)** — the QEMU that ships with AQEMU. **Leave this on unless you know you need something else.**
- **Use system-installed QEMU** — a QEMU you installed yourself.
- **Use a custom QEMU installation path** — pick a **QEMU folder**. AQEMU warns "No qemu-system-* binaries found in:" if the folder has no QEMU programs.

**WSL / KVM launch** (Windows only): **Enable Launch via WSL/KVM (for Intel macOS and other VMs)**, then set the **Distro**, **WSL Username** and an optional **WSL Password** (a sudo fallback). Tick **Remember password securely (Windows Credential Manager)** to store it in Windows' own credential store. Set **QEMU binary in WSL**, then click **Probe /dev/kvm** to check that KVM works. See [Accelerators](../../guides/features/accelerators.md).

**iOS firmware tools** (experimental Apple SoC VMs only): paths to **Python 3**, **pyimg4** and **img4**. Leave a box empty to use your system `PATH`.

## User Interface

- **AQEMU VM folder** — where new VMs are saved. AQEMU won't store VM data in its own install folder.
- **Default VM template**
- **Language**, **VM icon size**
- **New removable device menu**, **Ask before exiting**, **Show boot logo**, **Use screenshot as VM icon**
- **VM User Interface** — how VM windows are arranged

## Screenshots

Format (**PNG**, **JPEG** with a quality setting, or **PPM**) and an optional **Shared screenshot folder**.

## Logs And Monitoring

- **AQEMU Logs** — print to stdout and/or save to a file. Choose error, warning and debug messages, and the **Log path**.
- **QEMU Monitor Settings** — **stdio** ("Does not work with Windows version of QEMU") or **TCP socket**, with a hostname and first port.

## Network

How AQEMU makes MAC addresses for new network cards: **Random MAC address**, **QEMU segment MAC address (52:54:00:XX:XX:XX)**, or **Valid MAC address (Especially for each network card model)**.

## Other

- **Minimize AQEMU to the system tray**
- **Execute at start** / **Execute at exit** — run a program or script when QEMU starts or stops
- **First embedded VNC port**, **Host sound system** (for example `dsound` on Windows, `pa` or `pipewire` on Linux), **qemu-img path**, **Don't use default audio driver**

## Info Tab

Checkboxes for what the VM's **Info** tab shows: machine details, memory, CPUs, disks, CD-ROM, network cards, VNC/SPICE, QEMU arguments and more.

## USB

- **QEMU USB Mode** — **-device (New style)** (recommended; "Supports USB 2.0 and USB 3.0 controllers emulation") or **-usbdevice (Old style)** (USB 1.1 only).
- **USB Device ID** — how AQEMU identifies host USB devices you pass through: **Bus + Device Path** (default), **Vendor_ID + Product_ID**, or **Bus + Address**.

## Additional Devices

**Additional CDROM Devices** — extra host CD/DVD drives to offer in the device lists.

---

[Documentation Hub](../../README.md) · [Settings tab map](README.md) · [Menus](../menus.md)
