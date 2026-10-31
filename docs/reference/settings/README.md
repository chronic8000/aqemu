# Settings tab map

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Select a VM in the list on the left, and its settings appear in tabs on the right. When you change something, click **Apply** to save it, or **Discard Changes** to undo it.

AQEMU-wide settings (not per VM) are in **File → Preferences…**. See [Preferences](preferences.md).

---

| Tab | Sub-tabs | What it's for | Full guide |
| --- | --- | --- | --- |
| **Info** | — | A read-only summary of the VM, with a screenshot. Choose what it shows under **Preferences → Info Tab**. | — |
| **Machine** | — | CPU, memory, accelerator, boot order, graphics, mouse, sound, disk bus, and special panels for some guests | [Machine tab](#machine-tab) below |
| **Media** | Device Manager, Folder Sharing, Computer Ports, Boot Kernel, Special Images | Disks, CDs, floppies, shared folders, ports, direct kernel boot, flash/ROM images | [Storage and disks](../../guides/features/storage-and-disks.md) |
| **Display** | VNC Remote, SPICE Remote, QEMU Window | Where the guest appears, plus remote viewing | [Display](../../guides/features/display.md) |
| **Network** | Network, Redirections, TFTP/SAMBA | Network cards, port forwarding, TFTP and SMB | [Networking](../../guides/features/networking.md) |
| **Storage** | — | **Enable TrueNAS Guest Integration** | [TrueNAS](../../guides/guest-os/truenas.md) |

## Machine tab

| Area | Settings |
| --- | --- |
| **Machine** | **Machine** (board), **CPU Model**, **Architecture**, **CPUs**, **Accelerator** |
| **Options** | **Name**, **Boot** order, **Graphics**, **Resolution**, **Keyboard** |
| **Mouse** | **Pointer mode** (Seamless / Relative), **Mouse device**, **USB controller**, **USB version**, **SPICE agent mouse** ([details](../../guides/features/display.md#2-mouse-seamless-or-relative)) |
| **Memory** | A memory slider with the host's free memory shown, and **Remove limitation on maximum amount of memory** |
| **Sound** | Sound Blaster 16, ENSONIQ ES1370, Yamaha YM3812, AC97, Gravis Ultrasound GF1, PC Speaker, Intel HDA, cs4231a, VirtIO Sound, USB Audio, plus `-audiodev`, timer period and HDA codec |
| **Disk bus** | **Interface** for the main disk ([details](../../guides/features/storage-and-disks.md#4-disk-interface-the-bus)) |
| **Checkboxes** | **Enable ACPI (Power & System Control)**, **Snapshot mode**, **Use local time**, **Fullscreen mode** |
| **Advanced** | Extra options for power users |

Some guests show an extra panel:

- **Windows 11 ARM:** **Apply VirtIO defaults** and a **Mode** chooser (Install Windows / First boot / Normal / OOBE). See [Windows 11 on ARM](../../guides/windows-11.md#1-windows-11-on-arm).
- **Intel macOS (experimental):** OpenCore, installer image, **Apple SMC OSK** (AQEMU never pre-fills this), **Launch via WSL/KVM** and AMD GPU passthrough. See [macOS](../../guides/macos.md).
- **Apple SoC / iOS (experimental):** DeviceTree, kernel and boot arguments. See [iOS (Apple SoC)](../../guides/ios-apple-soc.md).

> **Snapshot mode** throws away every change when the VM stops. See [Snapshots](../../guides/features/snapshots.md).

---

[Documentation Hub](../../README.md) · [Preferences](preferences.md) · [Menus](../menus.md)
