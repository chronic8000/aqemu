# AQEMU Documentation

Welcome to the AQEMU docs! AQEMU is a desktop front end for QEMU on Windows and Linux. You build and run virtual machines from a GUI, and you never have to type a QEMU command line unless you want to.

These are the **v1** docs. Everything in them was checked against the AQEMU source on `master`: menu names, button labels, defaults, and the QEMU flags AQEMU generates. If something in the app doesn't match what you read here, please [open an issue](https://github.com/chronic8000/aqemu/issues) and tell us.

---

## Start here

New to AQEMU? Read these in order:

1. [Install AQEMU from the Microsoft Store](guides/getting-started/install-from-store.md) (AQEMU is £12.99, with a 7-day free trial)
2. [First start](guides/getting-started/first-start.md)
3. [Your first VM](guides/getting-started/your-first-vm.md)
4. Your host's guide (below)

## Pick your host

| Your computer | Guide | Fast guests (hardware acceleration) | Everything else |
| --- | --- | --- | --- |
| Windows on Intel/AMD (x64) | [Windows x64](guides/hosts/windows-x64.md) | x86 64-bit guests with **WHPX**, or **KVM** inside WSL | TCG (software) |
| Windows on ARM (Snapdragon) | [Windows ARM64](guides/hosts/windows-arm64.md) | ARM64 guests with **WHPX** | TCG (software) |
| Linux on Intel/AMD (x64) | [Linux x64](guides/hosts/linux-x64.md) | x86 64-bit guests with **KVM** | TCG (software) |
| Linux on ARM64 | [Linux ARM64](guides/hosts/linux-arm64.md) | ARM64 guests with **KVM** | TCG (software) |

32-bit guests (`qemu-system-i386` and `qemu-system-arm`) always use TCG. See [Accelerators](guides/features/accelerators.md).

## Guest operating systems

The New VM Wizard names **144 operating systems** (Windows 31, Linux 47, BSD 8, DOS/OS2 7, Unix 16, Other 22, TrueNAS 3, Apple 10), plus **69 Consoles & Retro** entries and **13 Lab** entries.

| Guide | What it covers |
| --- | --- |
| [Guest OS guides (all families)](guides/guest-os/README.md) | Every OS in the wizard, grouped by family, with the settings AQEMU picks |
| [Windows 11 and Legacy Windows](guides/windows-11.md) | Windows 11 ARM and x86, Windows 9x/XP/7 |
| [Windows 11 ARM on each host](guides/guest-os/windows-11-arm.md) | Per-host steps for Windows 11 on ARM64 |
| [iOS and Apple SoC (Experimental)](guides/ios-apple-soc.md) | The experimental Apple SoC tooling. You supply all Apple software yourself. |
| [macOS (Experimental)](guides/macos.md) | Classic Mac OS on PowerPC and the experimental Intel macOS profile. You supply all Apple software yourself. |
| [Operating System Catalog](guides/os-catalog.md) | Where each family lives in the wizard, templates, and the AQEMU guest tools |

## Using AQEMU

| Guide | What it covers |
| --- | --- |
| [New VM Wizard](guides/features/new-vm-wizard.md) | The wizard's 7 methods, step by step |
| [Session toolbar](guides/features/session-toolbar.md) | Every button above a running VM: power, media, files, keyboard, full screen |
| [Display, mouse and keyboard](guides/features/display.md) | Embedded vs separate window, Seamless vs Relative mouse, VNC/SPICE |
| [Networking](guides/features/networking.md) | NAT, bridge, TAP, port forwarding, which network card |
| [Storage and disks](guides/features/storage-and-disks.md) | Disk formats, Media tab, disk bus, conversion, SAN, recovery |
| [File transfer](guides/features/file-transfer.md) | Paste files, To guest / From guest, shared folders |
| [Copy and Paste](guides/copy-and-paste.md) | The text clipboard and the guest helper |
| [Snapshots](guides/features/snapshots.md) | Saved state, snapshots, snapshot mode, Recovery Checkpoint |
| [Accelerators](guides/features/accelerators.md) | TCG, WHPX, KVM and Xen, and when each is used |
| [Troubleshooting and FAQ](guides/features/troubleshooting.md) | Error messages, slow guests, getting help |
| [Command line](guides/getting-started/command-line.md) | `aqemu start/stop/…` over D-Bus (Linux) |
| [User Guide](guides/user-guide.md) | The original one-page walkthrough of the main window and wizard |
| [QEMU Lab (AQEMU 1.6)](guides/qemu-lab-1.6.md) | Power-user Lab features |

## Settings at a glance

| Tab | What it's for |
| --- | --- |
| **Info** | Read-only summary of the VM |
| **Machine** | CPU, memory, accelerator, boot, graphics, mouse, sound, disk bus |
| **Media** | Disks, CDs, floppies, shared folders, ports, kernel boot |
| **Display** | Where the guest appears, VNC/SPICE remote |
| **Network** | Cards, port forwarding, TFTP/SMB |
| **Storage** | TrueNAS integration |

Full details: [Settings tab map](reference/settings/README.md) · [Preferences](reference/settings/preferences.md) · [Menus](reference/menus.md)

## In-session toolbar

When a VM runs inside AQEMU, a toolbar sits above it: **Power** (pause, save state, reset, shut down, power off), **Media** (swap CD/floppy), **Files** (Paste files, To guest, From guest), **Devices** (USB, serial console), and **Keyboard and screen** (Ctrl+Alt+Del, grab mouse, full screen). See [Session toolbar](guides/features/session-toolbar.md).

## Reference

| Doc | What it covers |
| --- | --- |
| [Settings tab map](reference/settings/README.md) | Every per-VM settings tab |
| [Preferences](reference/settings/preferences.md) | **File → Preferences…** |
| [Menus](reference/menus.md) | Every menu item |
| [New VM Wizard Paths](reference/new-vm-wizard-paths.md) | How the wizard's paths map to QEMU machines |
| [QEMU Machine Catalog](reference/qemu-machine-catalog.md) | Friendly names for every QEMU machine the wizard knows (1,175 machines across 37 emulators) |
| [Intel macOS Graphics](reference/intel-macos-graphics.md) | Display defaults for the Intel macOS profile |
| [AQEMU vs virt-manager](reference/virt-manager-comparison.md) | Where AQEMU differs from virt-manager |

## Development

| Doc | What it covers |
| --- | --- |
| [Tobimensch issue triage](development/tobimensch-issue-triage.md) | Status of the old tobimensch/aqemu issue list (generated by `scripts/_gen_triage.py`) |
| [Probe audits](development/probe-audit/) | Wizard-vs-QEMU probe audit output (generated by `scripts/audit_wizard_vs_probes.py` and `scripts/final_probe_audit.py`) |
| [`qemu_machine_catalog.json`](qemu_machine_catalog.json) | Machine data the New VM Wizard loads at runtime. The build bundles it from this path, so it stays here |

## Elsewhere in the repo

| File | What it covers |
| --- | --- |
| [Root README](../README.md) | Product overview |
| [CHANGELOG](../CHANGELOG) | Release history |
| [Installer README](../installer/README.md) | MSI and MSIX packaging. The version comes from `VERSION.txt` |
| [Windows portable zip notes](../packaging/README-WINDOWS.txt) | Text shipped with the Windows portable zip |
| [xemu notes](../extras/xemu/README.txt) | Original Xbox: bring your own xemu and firmware |

## Archive

Superseded files are kept in [`archive/`](archive/) for history: the old docs index and the old single-page website. The public website lives on the `gh-pages` branch.

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
- **Command line.** On Linux, `aqemu start|stop|… <vm>` and `aqemu list` talk to the running AQEMU over D-Bus. See [Command line](guides/getting-started/command-line.md).

### Copy and paste

The built-in display shares the **text** clipboard with no guest software. For files, use **Paste files** (no guest software needed; AQEMU builds a floppy or a USB stick for the guest), or **To guest** / **From guest**, which need `qemu-guest-agent` in the guest. Full details are in [Copy and Paste](guides/copy-and-paste.md).

---

## Getting AQEMU

- **Windows:** [AQEMU on the Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs)
- **Linux and source:** [github.com/chronic8000/aqemu](https://github.com/chronic8000/aqemu)

AQEMU doesn't ship operating systems, Apple firmware, or console ROMs. You bring your own installers and licences.
