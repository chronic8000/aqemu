# Unix and workstation guests

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Classic workstation and server systems: Solaris and illumos, IRIX, HP-UX, Tru64 and OpenVMS, SCO, AIX, NeXTSTEP and OPENSTEP. Most of these are other CPU families (SPARC, MIPS, PA-RISC, Alpha, POWER, m68k), so they run under TCG and are **slow**. That's normal. The x86 ones (Solaris x86, OpenSolaris, illumos, OmniOS, SCO) can use WHPX/KVM on an x64 PC when they use `qemu-system-x86_64`.

Several tips say "experimental". Believe them: these OSes need suitable install media and patience.

## How to make one of these VMs

1. Click **Add New VM** (or press **Ctrl+N**).
2. Pick **Guest Operating System**, open the **vendor (Sun, SGI, HP, DEC, SCO, IBM or Apple)** branch and choose the OS.
3. On **Confirm QEMU system**, check the Computer Type and Machine Type. They're filled in from the table below and you can still change them.
4. Go through Accelerator → Name/CPU → Disk → Devices → Network → Finish.
5. On the VM's **Media** tab, point the CD/DVD drive at your installer image, then start the VM.

You supply the installer media and any licence the OS needs. AQEMU doesn't include operating systems.

> **What the table shows:** each OS's wizard starting point, taken from `resources/wizard_trees.json`. For some older OSes the wizard adjusts things further (for example, the Windows XP family gets one CPU, IDE disks and Force TCG), so the **Confirm** page has the final word. "Speed" follows AQEMU's accelerator rules: see [Accelerators](../features/accelerators.md).

## Sun

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **illumos** | `qemu-system-x86_64` | `pc-i440fx` | `qemu64` | 2 GB | 20 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **OmniOS** | `qemu-system-x86_64` | `pc-q35` | `max` | 2 GB | 20 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **OpenSolaris** | `qemu-system-x86_64` | `pc-i440fx` | `qemu64` | 4 GB | 20 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Solaris SPARC** | `qemu-system-sparc64` | `sun4u` | `TI-UltraSparc-II` | 1 GB | 20 GB | `sunhme` | TCG on every host |
| **Solaris x86** | `qemu-system-x86_64` | `pc-i440fx` | `qemu64` | 8 GB | 32 GB | `e1000` | WHPX on Windows x64, KVM on Linux x64. TCG on ARM hosts |
| **Solaris x86 (32-bit)** | `qemu-system-i386` | `pc-i440fx` | `qemu32` | 1 GB | 20 GB | `e1000` | TCG on every host (`i386` binary is TCG only) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **illumos:** illumos → x86_64 (PC) + Q35 with VirtIO.
- **OmniOS:** OmniOS → x86_64 (PC) + Q35 with VirtIO.
- **OpenSolaris:** OpenSolaris → same family as Solaris x86: std VGA, IDE, e1000, AC97, WHPX/KVM preferred.
- **Solaris SPARC:** Solaris SPARC → SPARC 64 / sun4u.
- **Solaris x86:** Solaris 11.4 x86 (r/qemu_kvm recipe): pc+usb=off, std VGA, e1000, sb16, IDE, 8 GB, WHPX/KVM, PIIX4 disable_s3/s4. Use sol-11_4-text-x86.iso. Install can sit on the SunOS banner / at 99% for 1–2 hours while the disk image keeps growing — not frozen. After install: create a normal user (root console login blocked); later pkg update + solaris-desktop; then VirtIO is optional.
- **Solaris x86 (32-bit):** Legacy Solaris 8/9/early 10 x86 (32-bit only) → IBM PC 32Bit. Modern ISOs will say “No long mode support” here — use Solaris x86 instead.

## SGI

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **IRIX 5.x** | `qemu-system-mips64` | `magnum` | `5Kf` | 256 MB | 8 GB | `e1000` | TCG on every host |
| **IRIX 6.x** | `qemu-system-mips64` | `magnum` | `5Kf` | 256 MB | 8 GB | `e1000` | TCG on every host |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **IRIX 5.x:** IRIX 5.x → qemu-system-mips64 -M magnum. Experimental; needs a suitable IRIX image.
- **IRIX 6.x:** IRIX 6.x → qemu-system-mips64 -M magnum. Experimental; needs a suitable IRIX image.

## HP

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **HP-UX** | `qemu-system-hppa` | `B160L` | `pa-8700` | 512 MB | 10 GB | `tulip` | TCG on every host |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **HP-UX:** HP-UX → qemu-system-hppa + B160L. Use SCSI disk (lsi53c895a). Needs a PA-RISC QEMU build and HP-UX install media.

## DEC

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **OpenVMS** | `qemu-system-alpha` | `clipper` | `ev4` | 512 MB | 10 GB | `e1000` | TCG on every host |
| **Tru64 UNIX** | `qemu-system-alpha` | `clipper` | `ev4` | 512 MB | 10 GB | `e1000` | TCG on every host |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **OpenVMS:** OpenVMS → Alpha Server / clipper (best QEMU fit). VAX is not available in mainline QEMU.
- **Tru64 UNIX:** Tru64 UNIX → Alpha Server / clipper.

## SCO

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **OpenServer** | `qemu-system-i386` | `pc` | `max` | 512 MB | 8 GB | `rtl8139` | TCG on every host (`i386` binary is TCG only) |
| **UnixWare** | `qemu-system-i386` | `pc` | `max` | 512 MB | 8 GB | `rtl8139` | TCG on every host (`i386` binary is TCG only) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **OpenServer:** SCO OpenServer → i386 PC, IDE, cirrus VGA. Classic retail/banking Unix; period media required.
- **UnixWare:** SCO UnixWare → i386 PC, IDE, cirrus VGA.

## IBM

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **AIX** | `qemu-system-ppc64` | `pseries` | `POWER8` | 2 GB | 20 GB | `virtio-net-pci` | TCG on every host |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **AIX:** AIX → qemu-system-ppc64 -M pseries -cpu POWER8. WHPX cannot accelerate POWER (TCG only, very slow). Use virtio-scsi, ≥2 GB RAM. For a VGA window do not set prom-env input/output to /vdevice/vty (that is for -nographic serial). True host↔guest clipboard needs spice-vdagent in the guest — AIX/SLOF do not provide that; paste into the QEMU window is host-display only. Prebuilt VMware VMDKs often fail with SLOF E3403 Bad executable.

## NeXT (Apple branch)

| OS | Emulator | Machine | CPU | RAM | Disk | NIC | Speed (accelerator) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **NeXTSTEP** | `qemu-system-m68k` | `next-cube` | `m68040` | 128 MB | 4 GB | `dp8393x` | TCG on every host |
| **OPENSTEP** | `qemu-system-i386` | `pc` | `max` | 256 MB | 4 GB | `ne2k_pci` | TCG on every host (`i386` binary is TCG only) |

### AQEMU's tip for each OS

These are the tips the wizard shows when you pick the OS, copied from the source.

- **NeXTSTEP:** NeXTSTEP → qemu-system-m68k + next-cube. Supply NeXT install media; experimental.
- **OPENSTEP:** OPENSTEP / NeXTSTEP for Intel → i386 PC. Use period install media; drivers are picky.


---

More: [All guest OS guides](README.md) · [Accelerators](../features/accelerators.md) · [New VM Wizard](../features/new-vm-wizard.md) · [Documentation Hub](../../README.md)
