# Guest operating systems: the full list

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

A **guest** is the operating system inside your VM. AQEMU's New VM Wizard has a ready-made **profile** for each OS below: the emulator, machine, CPU, memory, disk and devices to start with, plus a tip. Pick **Guest Operating System** on the wizard's first page and choose from the tree.

## How many? (counted from the source)

| What | Count | Where it comes from |
| --- | --- | --- |
| Named operating systems in the wizard | **144** | `resources/wizard_trees.json` (`operating_systems`), not counting consoles/boards, lab setups, helper VMs and "Other" |
| Consoles, retro machines and dev boards | **69** | the **Consoles & Retro** branch |
| Lab setups (advanced) | **13** | the **Lab** branch |
| Ready-made wizard profiles in total | **230** | `os_profiles` in the same file |
| Saved VM templates | **25** | `resources/os_templates/*.aqvmt` |
| QEMU machine types AQEMU knows about | **1,175** across **37** QEMU emulators | `docs/qemu_machine_catalog.json` (generated on a Linux host) |

A profile is a starting point, not a promise that every release of that OS installs cleanly. Some tips say "experimental", and they mean it.

## Guides by family

| Family | Guide | OSes |
| --- | --- | --- |
| Windows | [Windows guests](windows.md) | 31 (Windows 1.x – Windows 11, Windows Server 2000 – 2025) |
| Windows 11 on ARM | [Windows 11 on ARM, host by host](windows-11-arm.md) | Windows 11 (ARM64) on every host |
| Linux | [Linux guests](linux.md) | 47 (x86_64, ARM64 and 32-bit x86 distros, plus Linux on IBM Z) |
| BSD | [BSD guests](bsd.md) | 8 |
| DOS and OS/2 | [DOS, OS/2 and retro PCs](retro-dos-os2.md) | 7 |
| Unix and workstations | [Unix and workstation guests](unix.md) | 16 (Solaris, illumos, IRIX, HP-UX, Tru64, OpenVMS, SCO, AIX, NeXTSTEP, OPENSTEP) |
| Other | [Other operating systems](other.md) | 22 (Haiku, ReactOS, ChromeOS, SteamOS, Android, Plan 9…) |
| TrueNAS | [TrueNAS and storage labs](truenas.md) | 3 TrueNAS installs + 3 storage labs |
| Apple (experimental) | [macOS and classic Mac OS](../macos.md) · [iOS / Apple SoC](../ios-apple-soc.md) | 10 |
| Boards | [ARM, RISC-V, PowerPC, MIPS and other boards](arm-riscv-boards.md) | 69 |

31 + 47 + 8 + 7 + 16 + 22 + 3 + 10 = **144**.

## Apple entries

Apple guests are **experimental**. AQEMU doesn't include, distribute or tell you where to get any Apple software, firmware or keys. You source those yourself.

| Wizard entry | Covered in |
| --- | --- |
| macOS, Mac OS X Intel, Darwin | [macOS guide](../macos.md) (Intel path) |
| Mac OS 6, Mac OS 7, Mac OS 8, Mac OS 9, Mac OS X PPC | [macOS guide](../macos.md) (classic Mac paths) |
| iOS (ARM64) | [iOS / Apple SoC guide](../ios-apple-soc.md) |
| macOS Apple Silicon (ARM64) | No profile of its own yet. See the [macOS guide](../macos.md). |
| NeXTSTEP, OPENSTEP | [Unix and workstation guests](unix.md) |

## Lab setups

The **Lab** branch holds 13 advanced setups for people who already know QEMU: PowerNV, s390x Debian, RISC-V virt lab, LoongArch virt lab, WoA native Linux, WoA x86 via TCG, Original Xbox, Android (AOSP Emulator), macOS (OpenCore), Nitro Enclave, SEV-SNP Guest, TDX Guest and Darwin research shell. They aren't covered in the v1 guides.

## Speed in one line

A guest is fast only when its CPU family matches your PC and AQEMU can use WHPX (Windows) or KVM (Linux). Otherwise it runs under TCG, which is slower but runs nearly anything. Details: [Accelerators](../features/accelerators.md).

| Your PC | Fast guests |
| --- | --- |
| Windows x64 / Linux x64 | Guests that use `qemu-system-x86_64` |
| Windows ARM64 / Linux ARM64 | Guests that use `qemu-system-aarch64` |

---

More: [Documentation Hub](../../README.md) · [New VM Wizard](../features/new-vm-wizard.md)
