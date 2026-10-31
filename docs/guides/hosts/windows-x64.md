# AQEMU on Windows x64

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This page is for a normal Intel or AMD Windows PC (64-bit). It's the most common AQEMU host.

---

## Get AQEMU

**Recommended:** [AQEMU on the Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs) (£12.99, 7-day trial). QEMU is included. See [Install from the Microsoft Store](../getting-started/install-from-store.md).

**From source:** the repo has `scripts/build_aqemu_win64.ps1`, which builds x86_64 AQEMU in the **MSYS2 UCRT64** environment. Its switches are `-InstallDeps` (install the toolchain and Qt), `-SkipSpice` and `-BuildMsix`. QEMU for Windows is built separately (`scripts/build_qemu_win_x86_64.ps1`).

## Turn on fast virtualization (WHPX)

Without hardware acceleration, every guest runs on **TCG** (software emulation), which works but is slow. On Windows x64, AQEMU uses the **Windows Hypervisor Platform (WHPX)** for x86 guests.

1. Open **Windows Features** (`optionalfeatures` from Start) and tick:
   - **Hyper-V**
   - **Virtual Machine Platform**
   - **Windows Hypervisor Platform**
2. Restart Windows.
3. In AQEMU, open **Help → Diagnostics → WHPX health…**. The **Hyper-V / WHPX health** window checks that QEMU can see WHPX and reports the state of **Core isolation (Memory Integrity)**. Its buttons open the matching Windows Settings pages.

If WHPX fails when a VM starts, AQEMU offers **Force TCG** the next time the VM starts, so the VM still runs, just slowly.

## Which guests are fast?

| Guest | Accelerator |
| --- | --- |
| 64-bit x86 guests (`qemu-system-x86_64`: modern Windows, Linux, BSD…) | **WHPX** (fast) |
| 32-bit x86 guests that use `qemu-system-i386` (Windows 9x/XP 32-bit, DOS, OS/2…) | **TCG**. AQEMU never offers WHPX/KVM for the `i386` binary. |
| ARM64, PowerPC, SPARC, MIPS, RISC-V, m68k… | **TCG** (emulated, slower) |

WHPX only accelerates a guest whose CPU type matches your PC. AQEMU switches other guests to TCG automatically and says so in the accelerator tooltip: "WHPX runs a guest that matches this … PC. This … guest uses TCG."

Some older guests (OS/2, ReactOS) use **Force TCG** in their wizard profiles on purpose. Leave it on.

## Optional: KVM through WSL

If you have **WSL 2** with a Linux distro, AQEMU can run the **Linux** build of QEMU inside WSL with **KVM**:

- Choosing **KVM** in a VM's accelerator list turns **Run via WSL** on ("Linux KVM inside WSL").
- Choosing **WHPX** turns **Run via WSL** off and starts the Windows QEMU.
- **File → Host → Configure WSL…** sets the WSL distro and username AQEMU uses for KVM launches.
- KVM needs a usable `/dev/kvm` inside WSL. AQEMU adds your WSL user to the `kvm` group when it launches there.
- **Help → Diagnostics → WSL dashboard…** shows the WSL side.

There is **no separate "WSL2 accelerator"**. It's KVM, run inside WSL.

## Windows-specific behaviour

- **Shared folders (9p / Samba) need a Linux host.** On Windows, the folder-sharing **Add** button is hidden. Use **Paste files** on the session toolbar instead. See [File transfer](../features/file-transfer.md).
- **Audio:** host playback uses DirectSound.
- **Where things live:** settings and VMs are under your AppData folder (the Store version uses its own `aqemu-store` folder).

## Good first guests on Windows x64

- [Windows 11 x86_64](../windows-11.md#2-windows-11-on-x86_64)
- [Linux (Ubuntu, Debian, Fedora…)](../guest-os/linux.md)
- [Retro Windows and DOS](../guest-os/retro-dos-os2.md)

---

Other hosts: [Windows on ARM](windows-arm64.md) · [Linux x64](linux-x64.md) · [Linux ARM64](linux-arm64.md) · [Documentation Hub](../../README.md)
