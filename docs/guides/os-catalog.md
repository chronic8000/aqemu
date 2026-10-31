# Operating System Catalog

AQEMU isn't just for Windows. The wizard knows **230 operating system profiles** across **28 CPU architecture families** (29 QEMU system emulators): Linux and BSD on PC and ARM, DOS and OS/2, Solaris on SPARC, AIX on POWER, Linux on IBM Z, and plenty of museum pieces.

This catalog shows you where to find each family in the **New Virtual Machine Wizard**, what AQEMU picks for you, and how to install the AQEMU guest tools.

> Windows has its own guide: [Windows 11 (ARM and x86) and Legacy Windows](windows-11.md).

**Contents**

1. [How to pick a guest](#1-how-to-pick-a-guest)
2. [Built-in templates](#2-built-in-templates)
3. [Linux and BSD](#3-linux-and-bsd)
4. [ARM: Raspberry Pi and ARM64](#4-arm-raspberry-pi-and-arm64)
5. [RISC-V, POWER, SPARC and IBM Z](#5-risc-v-power-sparc-and-ibm-z)
6. [DOS, OS/2 and retro](#6-dos-os2-and-retro)
7. [TrueNAS and remote storage](#7-truenas-and-remote-storage)
8. [AQEMU guest tools](#8-aqemu-guest-tools)

---

## 1. How to pick a guest

Open `VM → Add VM → Add New VM` (`Ctrl+N`) and choose:

- **Guest Operating System** when you know the OS. Vendors: Lab, Apple, BSD, DEC, HP, IBM, Linux, Microsoft, Other, SCO, SGI, Sun, TrueNAS / Storage, Consoles & Retro.
- **System / Machine Platform** when you know the hardware. Platforms: IBM PC Compatible, ARM Virtual Platforms, Raspberry Pi, Apple Macintosh, Sun Microsystems, IBM Power, IBM Mainframe, MIPS, DEC Alpha, HP PA-RISC, RISC-V, Embedded, Other Platforms, Generic, Consoles & Retro.
- **CPU Architecture** when you want a CPU family first: x86, x86-64, ARM, ARM64 / AArch64, PowerPC 32/64, MIPS (four variants), SPARC32/64, RISC-V 32/64, s390x, Alpha, PA-RISC, m68k, AVR, MicroBlaze, OpenRISC, Xtensa (two variants), RX, SH4 (two variants), TriCore, LoongArch.

Each profile sets the emulator, machine, CPU, RAM, disk size and guest-appropriate devices. The friendly-name-to-QEMU mapping is in the [QEMU Machine Catalog](../reference/qemu-machine-catalog.md).

## 2. Built-in templates

Under **Custom / Advanced → Template** you'll find 25 ready-made templates. The non-Windows ones:

| Template | Emulator / machine | CPU | RAM | Disk | Notes |
| --- | --- | --- | --- | --- | --- |
| Ubuntu Linux | `x86_64` / `q35` | host | 2048 MB | 25 GB | VirtIO GPU, Intel HDA |
| Linux 2.0 / 2.4 / 2.6 / 2.6 x86_64 | `x86` / `x86_64` | default | 64–256 MB | 4–20 GB | For old kernels |
| FreeBSD 4 / 7 / 8 | `x86` | default | 64–256 MB | 4–20 GB | |
| Haiku OS | `x86_64` / `pc` | core2duo | 1024 MB | 10 GB | Intel HDA |
| FreeDOS | `i386` / `pc` | pentium | 32 MB | 500 MB | Cirrus, SB16 + AdLib, PC speaker, boots from floppy |
| Raspberry Pi OS | `aarch64` / `raspi3b` | cortex-a53 ×4 | 1024 MB | 16 GB | Real Pi 3B board, USB (lan9514) network |
| RISC-V Linux | `riscv64` / `virt` | rv64 ×2 | 2048 MB | 20 GB | VirtIO GPU and network |
| Mac OS Classic PPC | `ppc` / `mac99` | default | 512 MB | 10 GB | sungem network (see the [macOS guide](macos.md)) |

You can save any VM as your own template with `VM → Save As Template`.

## 3. Linux and BSD

**Guest Operating System → Linux** offers Ubuntu, Debian, Fedora, Arch, openSUSE, Mint, Alpine, Rocky, AlmaLinux, NixOS and many more, in 32-bit, 64-bit and ARM64 versions. The 64-bit PC profiles use:

- `x86_64` + `q35`, `-cpu max`, 4096 MB RAM, a 20 GB disk
- VirtIO disk, VirtIO network and VirtIO VGA first, with AHCI/NVMe, e1000e/e1000 and QXL/std as fallbacks

> Ubuntu desktop needs about 4 GB RAM. AQEMU's tip: "GNOME live/install fails on 2 GB under TCG." Use WHPX or KVM if you can.

FreeBSD (64-bit) and Haiku (64-bit) use the same Q35 + VirtIO pattern.

## 4. ARM: Raspberry Pi and ARM64

You have two choices:

- **Emulate a real Pi board.** Use **System / Machine Platform → Raspberry Pi** (Pi Zero, 1, 2, 3, 4) or the **Raspberry Pi OS** template (`raspi3b`, see above). This is the most authentic option, but it's slow and limited to the board's devices.
- **Run Raspberry Pi OS on a generic ARM64 VM.** Use **Guest Operating System → Linux → ARM64 / AArch64 (Modern ARM) → Raspberry Pi OS (ARM64)**. It uses `aarch64` + `virt`, VirtIO, UEFI, 2048 MB and 16 GB. This is much faster, and it uses KVM on ARM Linux hosts.

ARM64 Linux profiles (Ubuntu, Debian, Fedora…) get dual-pflash UEFI, xHCI input, `-cpu host` with KVM on ARM hosts, and `-cpu max` with TCG elsewhere.

## 5. RISC-V, POWER, SPARC and IBM Z

| Guest | Profile |
| --- | --- |
| RISC-V | **RISC-V Linux** template (`virt`, rv64, VirtIO), or **Lab → RISC-V virt lab** (open firmware and kernels only) |
| AIX | `ppc64` + `pseries`, POWER8, 2 GB+. TCG only (WHPX can't accelerate POWER), so it's very slow. |
| Solaris SPARC | `sparc64` + `sun4u`, UltraSPARC II, sunhme network, SCSI disk |
| Linux on IBM Z | `s390x` + `s390-ccw-virtio`, VirtIO CCW disk and network, usually headless |
| IRIX 6.x | `mips64` + `magnum`, marked **experimental**; needs a suitable IRIX image |

## 6. DOS, OS/2 and retro

| Guest | What AQEMU picks |
| --- | --- |
| MS-DOS | `i386` PC, 486 CPU, 32 MB, IDE, Cirrus |
| FreeDOS | `i386` PC, 64 MB, IDE, NE2000/PCnet/RTL8139, SB16 |
| OS/2 Warp | "Proven AQEMU profile": Force TCG / pentium3, Cirrus, ACPI off, IDE (not VirtIO), RTL8139, ES1370, 512 MB, 1 CPU. AQEMU's tip: "LVM Error 9 usually means VirtIO/ACPI." |
| Original Xbox (Lab) | Runs through a separate **xemu** binary that you install. You bring xemu, the MCPX, the flash BIOS, the HDD, the DVD and the EEPROM. AQEMU doesn't ship Microsoft ROMs. |

For these guests, set **Pointer mode** to **Relative** on the Machine tab.

## 7. TrueNAS and remote storage

- **TrueNAS SCALE** (`TrueNAS / Storage`): `x86_64` + `q35`, 8192 MB, a 32 GB disk on NVMe, VirtIO networking, and **4096-byte (4Kn) sectors** by default for OpenZFS.
- The **Storage** tab has **Enable TrueNAS Guest Integration**. Enter the TrueNAS address, port and API token, then click **Refresh from TrueNAS** to see its pools.
- `VM → Storage → Attach Remote SAN Storage (iSCSI / NVMe-oF)…` attaches network block storage to a VM.

## 8. AQEMU guest tools

For basic use you don't need anything in the guest:

- **Text** copy/paste works through AQEMU's built-in display.
- **Paste files** gives the guest a floppy (A:) or a FAT USB drive named **AQEMU**.

To copy files to and from guest paths (**To guest** / **From guest**), or to copy files in the guest's file manager and have them arrive on the host, click **Guest tools**. AQEMU inserts a floppy with the tools.

**Linux guest:** open a terminal in that folder and run

```sh
sh install-linux.sh
```

It installs `qemu-guest-agent` with `apt-get`, `dnf` or `pacman`, enables it, and starts the clipboard helper (`aqemu-guest.py`) if `python3` is present. The helper needs `wl-paste` or `xclip`.

**Windows guest:** install `qemu-guest-agent` from the QEMU project (it includes the virtio serial driver). Then, in the tools folder, run

```bat
powershell -STA -ExecutionPolicy Bypass -File aqemu-guest.ps1
```

**DOS and Windows 95/98:** these can't run the tools. Keep using **Paste files**.

More detail is in [Copy and paste](copy-and-paste.md).

---

Next: [User Guide](user-guide.md) · [Windows guide](windows-11.md) · [Documentation Hub](../README.md)
