# AQEMU Documentation Hub

Welcome to the AQEMU docs! AQEMU is a desktop front end for QEMU on Windows and Linux. You build and run virtual machines from a GUI, and you never have to type a QEMU command line unless you want to.

Everything in these guides was checked against the AQEMU source on `master`: menu names, button labels, defaults, and the QEMU flags AQEMU generates. If something in the app doesn't match what you read here, please [open an issue](https://github.com/chronic8000/aqemu/issues) and tell us.

---

## Start here

| Guide | What it covers |
| --- | --- |
| [User Guide: Overview and the New VM Wizard](USER_GUIDE_OVERVIEW_AND_WIZARD.md) | First start, the main window, the **New Virtual Machine Wizard**, and the settings tabs |
| [Windows 11 (ARM and x86) and Legacy Windows](WINDOWS_11_ARM_AND_X86_GUIDE.md) | Windows 11 ARM with slipstreamed VirtIO drivers, Windows 11 x86, Windows 9x/XP/7 |
| [Operating System Catalog](MULTI_PLATFORM_OS_INSTALLATION_CATALOG.md) | Linux, BSD, retro and other guests, the built-in templates, and the AQEMU guest tools |
<!-- site:skip-start -->
| [macOS Guide (Experimental)](MACOS_INSTALLATION_GUIDE.md) | Classic Mac OS on PowerPC and the experimental Intel macOS profile |
| [iOS / Apple SoC Guide (Experimental)](IOS_APPLE_SOC_INSTALLATION_GUIDE.md) | The experimental Inferno (`qemu-system-applesoc`) tooling for iPhone 11 / t8030 |
<!-- site:skip-end -->

## Reference docs already in the repo

| Doc | What it covers |
| --- | --- |
| [New VM Wizard paths](VM_WIZARD_THREE_PATH.md) | How the wizard's Guest OS / Platform / Architecture / Custom paths map to QEMU machines, and the guest-aware Devices page |
| [QEMU Machine Catalog](QEMU_MACHINE_CATALOG.md) | Friendly names for every QEMU machine the wizard knows about |
| [Copy and paste](COPY_AND_PASTE.md) | Text clipboard through the built-in display, **Paste files**, **To guest** / **From guest**, and the guest helper |
| [Intel macOS graphics](intel-macos-gpu.md) | Display defaults for the Intel macOS profile |

---

## What AQEMU does

### Making VMs

- **New Virtual Machine Wizard** (`VM → Add VM → Add New VM`, or `Ctrl+N`). Seven ways in: *Guest Operating System*, *System / Machine Platform*, *CPU Architecture*, *Custom / Advanced*, *Import Existing Disk*, *Import Virtual Appliance (OVA / OVF)*, and *Recover storage*.
- **Templates.** 25 built-in templates (Windows 9x through Windows 11 and Windows 11 ARM, Linux, FreeBSD, Haiku, FreeDOS, Raspberry Pi OS, RISC-V Linux, and Mac OS Classic PPC). You can save your own with `VM → Save As Template`.
- **Appliances.** `File → Import and Export → Import Appliance (OVA / OVF)…` and `Export Appliance (OVA)…`.

### Running VMs

- **Accelerators.** TCG everywhere; WHPX on Windows; KVM on Linux (and inside WSL); Xen where available.
- **Session tools.** The `VM → Tools` menu has the QEMU monitor, serial monitor, **Show QEMU Arguments**, **Create Shell Script**, and the QEMU error log.
- **Snapshots and recovery.** `VM → Storage → Manage Snapshots`, plus `VM → Recovery → Storage Recovery…` and `Recovery Checkpoint…`.
- **Remote storage.** `VM → Storage → Attach Remote SAN Storage (iSCSI / NVMe-oF)…`.
- **Command line.** `aqemu start|stop|… <vm>` and `aqemu list` for scripting.

### Copy and paste

The built-in display shares the **text** clipboard with no guest software. For files, use **Paste files** (no guest software needed; AQEMU builds a floppy or a USB stick for the guest), or **To guest** / **From guest**, which need `qemu-guest-agent` in the guest. Full details are in [Copy and paste](COPY_AND_PASTE.md).

---

## Getting AQEMU

- **Windows:** [AQEMU on the Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs_hub)
- **Linux and source:** [github.com/chronic8000/aqemu](https://github.com/chronic8000/aqemu)

AQEMU doesn't ship operating systems, Apple firmware, or console ROMs. You bring your own installers and licences.
