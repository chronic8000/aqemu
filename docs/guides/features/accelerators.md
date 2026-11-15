# Accelerators: TCG, WHPX, KVM and Xen

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

The **accelerator** decides how fast your VM runs. Pick it in the wizard, or on the **Machine** tab (**Accelerator** list, plus an **Options** button).

---

## The four accelerators

| Accelerator | Where | What it is | Speed |
| --- | --- | --- | --- |
| **TCG** | Every host | "Software emulation. Runs any guest on this PC." | Slow, but runs any CPU type |
| **WHPX** | Windows | "Windows Hypervisor Platform" | Fast |
| **KVM** | Linux, or Windows through WSL | Linux's built-in hypervisor | Fast |
| **XEN** | Linux with Xen | Listed when Xen is present | Fast |

If an accelerator can't be used, the list shows it as "… (unavailable)" and the tooltip explains why.

## The one rule to remember

**Hardware acceleration (WHPX / KVM / Xen) only works when the guest's CPU type matches your PC's.**

| Your PC | Fast guests | Emulated (TCG) guests |
| --- | --- | --- |
| Windows x64 / Linux x64 | x86_64 (`qemu-system-x86_64`) | 32-bit `qemu-system-i386` guests, ARM, PowerPC, SPARC, MIPS, RISC-V… |
| Windows on ARM / Linux ARM64 | ARM64 (`qemu-system-aarch64`) | 32-bit `qemu-system-arm` guests, x86, x86_64, everything else |

AQEMU treats the 32-bit `qemu-system-i386` and `qemu-system-arm` binaries as TCG only, even on a matching host. If you want acceleration for a 32-bit x86 OS, the `qemu-system-x86_64` binary can run it too: change **Computer Type** on the Machine tab. Some very old guests are happier under TCG anyway.

AQEMU handles this for you. If the guest doesn't match, the tooltip says, for example, "WHPX runs a guest that matches this … PC. This … guest uses TCG."

## WHPX (Windows)

1. Turn on **Hyper-V**, **Virtual Machine Platform** and **Windows Hypervisor Platform** in Windows Features, then restart.
2. Check **Help → Diagnostics → WHPX health…** (window title **Hyper-V / WHPX health**). It asks QEMU which accelerators it supports and checks Core isolation (Memory Integrity). The buttons open the Windows Settings pages for Optional Features and Core isolation.
3. "A failed probe offers Force TCG the next time a VM starts."

**Windows on ARM:** ARM64 guests run under WHPX with CPU model `max`. x86 guests are TCG only.

## KVM (Linux)

- Needs `/dev/kvm` and permission to use it (usually the `kvm` group).
- Tooltip when ready: "Linux KVM. The guest matches this PC and /dev/kvm is usable."
- Tooltip when not: "KVM needs /dev/kvm on this PC. This guest uses TCG until then."
- ARM64 guests on an ARM64 Linux host use CPU `host` under KVM.

## KVM through WSL (Windows)

- Choosing **KVM** on Windows means "Linux KVM inside WSL. Choosing it turns Run via WSL on."
- Choosing **WHPX** turns Run via WSL off again.
- Set the WSL distro and user in **File → Host → Configure WSL…**.
- If WSL has no usable `/dev/kvm`: "KVM needs a usable /dev/kvm in WSL. This guest uses TCG until then."

> There is no "WSL2 accelerator". WSL is just where KVM runs.

## TCG details

- TCG runs anything, including ARM on x86 and x86 on ARM, slowly.
- For ARM guests under TCG, AQEMU uses CPU `max,pauth-impdef=on` (faster pointer authentication).
- **Force TCG** (used by some retro profiles such as OS/2 and ReactOS) makes x86 guests use TCG. If the CPU model is empty, `max`, `host`, `qemu64` or `qemu32`, AQEMU swaps it for `pentium2`.
- If you pick a CPU model yourself on the Machine tab, AQEMU uses yours.

## Which one should I choose?

1. If **WHPX** or **KVM** is offered and not "(unavailable)", use it.
2. If the guest profile says Force TCG (old OSes), leave it.
3. Otherwise TCG. It works, just slower.

---

[Documentation Hub](../../README.md) · [Troubleshooting](troubleshooting.md) · [Hosts](../hosts/windows-x64.md)
