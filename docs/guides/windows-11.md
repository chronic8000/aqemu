# Windows 11 and Legacy Windows

This is the Windows guide. It covers Windows 11 on ARM, Windows 11 on x86, and the classics: Windows 9x, XP and 7. AQEMU ships tuned defaults for all of them. This guide shows you where those defaults are, what they do, and what to change when you need to.

> You need your own Windows installer ISO and licence. AQEMU doesn't ship Windows.

**Contents**

1. [Windows 11 on ARM](#1-windows-11-on-arm)
2. [Windows 11 on x86_64](#2-windows-11-on-x86_64)
3. [Getting past OOBE (local account)](#3-getting-past-oobe-local-account)
4. [Legacy Windows: 9x, XP, 7](#4-legacy-windows-9x-xp-7)

---

## 1. Windows 11 on ARM

### Step 1: Prepare a slipstreamed ISO

Windows Setup on ARM can't see a VirtIO disk without drivers. The cleanest fix is a Windows 11 ARM64 ISO with the [Windows VirtIO drivers](https://github.com/virtio-win/virtio-win-pkg-scripts) (ARM64 builds) already added to `boot.wim` and `install.wim`. AQEMU's wizard expects this.

If your ISO isn't slipstreamed, the wizard can attach `virtio-win.iso` as a second CD so you can load drivers by hand during Setup.

### Step 2: Create the VM

1. Open the wizard: `VM → Add VM → Add New VM` (`Ctrl+N`).
2. Choose **Custom / Advanced**, then **Template**, and set **OS Type** to **Windows 11 ARM**.
   - On an ARM host, **Guest Operating System → Microsoft → Windows 11** also switches to AArch64 + `virt` automatically.
3. The **Windows 11 ARM Install** page asks for:
   - **Virtual disk:** **Create a new disk image** or **Use an existing disk image**. If Windows is already on that disk, tick **Windows is already installed on this disk**.
   - **Windows 11 ARM ISO:** your slipstreamed ISO.
   - **Also attach virtio-win.iso (only if drivers were not slipstreamed)**.
   - **TPM 2.0 (start swtpm with this VM)**. This is only enabled if `swtpm` is installed. The VM still boots without it.
4. Finish. AQEMU picks UEFI (AAVMF) firmware and creates a private VARS file for this VM.

### What AQEMU sets up for you

These are the **Windows 11 ARM** template defaults. We checked them by running AQEMU's argument builder:

| Setting | Default | QEMU flag |
| --- | --- | --- |
| Machine | `virt`, GICv3, high memory, ITS | `-machine virt,gic-version=3,highmem=on,its=on` |
| CPU / cores | `max`, 4 vCPUs | see the accelerator table below |
| Memory | 8 GB | `-m 8192` |
| Disk | 64 GB, VirtIO | |
| Graphics | VirtIO GPU with EDID | `-device virtio-gpu-pci,edid=on,max_outputs=1` |
| Audio | USB audio on its own xHCI controller | `-device qemu-xhci,id=usb_audio_xhci -device usb-audio` |
| Network | User-mode NAT, VirtIO NIC | `-netdev user` + `virtio-net-pci` |
| Input | USB tablet + USB keyboard on xHCI | |
| Extras | VirtIO RNG, VirtIO balloon, the guest-agent channel, guest clock in local time | `virtio-rng-pci`, `virtio-balloon-pci`, `-rtc base=localtime` |
| Firmware | AAVMF UEFI + per-VM VARS | two `pflash` drives |

**CPU and accelerator.** AQEMU chooses the CPU flags itself:

| Host | Accelerator | CPU AQEMU uses |
| --- | --- | --- |
| x86_64 (Windows or Linux) | TCG (emulation, slow) | `-cpu max,pauth-impdef=on` with `-accel tcg,thread=multi,tb-size=1024` |
| Windows on ARM | WHPX | `-cpu max`. AQEMU avoids `-cpu host` here because it has been tied to host crashes on Insider WHPX-ARM builds. WHPX also needs the AAVMF firmware. |
| Linux on ARM | KVM | `-cpu host` |

> Running ARM Windows on an x86 PC is full emulation. It works, but expect it to be much slower than an ARM host.

### Step 3: The three lifecycle modes

On the **Machine** tab, Windows 11 ARM VMs get their own panel with three buttons. This is the part that makes Windows on ARM painless:

| Button | What it does | When to use it |
| --- | --- | --- |
| **Install Windows** | "Install profile (BVM firstboot): ramfb only, USB installer ISO, CD boot first." | While Windows Setup runs |
| **First boot** | "First boot / OOBE (BVM boot): virtio-gpu only, no installer ISO, HDD boot, cache=none." | After Setup, for the OOBE screens |
| **Normal** | "Daily use (BVM boot): virtio-gpu only, no installer ISO, HDD boot." | Every day after that |

A new install starts in **Install Windows**: the display is `ramfb` and a VirtIO keyboard is added so Setup works before any drivers load. When Setup finishes, switch to **First boot**, complete OOBE, then switch to **Normal**. The panel shows the current mode (for example "Mode: Normal").

---

## 2. Windows 11 on x86_64

**Quickest path:** **Guest Operating System → Microsoft → Windows 11**. The wizard profile uses x86_64 + `q35`, `-cpu max`, 4 GB RAM, a 40 GB disk, NVMe as the first disk choice, VirtIO or e1000e networking, Intel HDA audio, and VirtIO VGA.

The older **Windows 11** template (under **Custom / Advanced → Template**) is plainer: `q35`, `max`, 2 CPUs, 4 GB, 64 GB, standard VGA, Intel HDA and e1000.

**Windows 11's checks need UEFI and TPM 2.0.** In AQEMU:

- **TPM:** **Machine → Advanced → TPM:** set to **emulator (swtpm socket)**. This needs `swtpm` installed.
- **UEFI:** **Machine → Advanced → OVMF CODE / OVMF VARS**.

**Acceleration:** choose **WHPX** on Windows hosts or **KVM** on Linux in the **Accelerator** box on the Machine tab.

If you can't use a TPM, the well-known manual workaround still works. In Setup, press **Shift+F10** and add the `LabConfig` keys `BypassTPMCheck`, `BypassSecureBootCheck` and `BypassRAMCheck` (DWORD `1`) under `HKLM\SYSTEM\Setup\LabConfig`. AQEMU doesn't do this for you.

---

## 3. Getting past OOBE (local account)

Newer Windows 11 builds push hard for a Microsoft account. AQEMU has a helper button: **Machine → OOBE / UCPD…** ("OOBE account bypass (Shift+F10) and UCPD.sys removal helpers"). It gives you commands to paste into the guest.

To open a command prompt during OOBE, press **Shift+F10**. The running VM's toolbar also has a **Send Shift+F10** button. Then try these in order:

```bat
rem 1) Local-account OOBE URI (works on many 24H2+ builds):
start ms-cxh:localonly

rem 2) Classic BypassNRO + reboot (if step 1 does nothing):
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\OOBE" /v BypassNRO /t REG_DWORD /d 1 /f
shutdown /r /t 0

rem 3) Optional: kill networking so OOBE offers offline path:
ipconfig /release
```

**UCPD fix.** If an ARM VM reboots or blue-screens right after OOBE, the same button gives you the UCPD.sys removal commands. Run them in the guest command prompt, then reboot.

---

## 4. Legacy Windows: 9x, XP, 7

### Windows 95 / 98

Use **Guest Operating System → Microsoft → Windows 98** (or Windows 95). It's a proven AQEMU profile:

- `qemu-system-i386`, `pc` machine, 1 CPU, 128 MB RAM
- **Pure TCG** with a `pentium2` CPU (`-accel tcg,thread=single`)
- Cirrus VGA, IDE disk, NE2000 PCI network, Sound Blaster 16, ACPI off

You can turn on **Force pure TCG (Win9x / pre-ME)** for any VM under **Machine → Advanced**. AQEMU then swaps `max`/`host`/`qemu64` CPUs for `pentium2` and drops hardware acceleration. Windows 9x is not happy on modern virtualised CPUs.

Tips:
- Keep RAM at 512 MB or less. Windows 9x has well-known problems with more.
- Set **Pointer mode** to **Relative** (Windows 9x has no tablet driver). Click the screen to capture the mouse, and middle-click or press Escape to release it.
- To move files in, use **Paste files**. AQEMU builds a floppy (A:) or a FAT USB drive. See [Copy and paste](copy-and-paste.md).

### Windows XP and Windows 7

The built-in templates:

| Template | Emulator | RAM | Disk | Audio |
| --- | --- | --- | --- | --- |
| Windows XP | `qemu-system-i386` | 256 MB | 20 GB | ES1370 |
| Windows XP x64 | `qemu-system-x86_64` | 256 MB | 20 GB | ES1370 |
| Windows 7 | `qemu-system-i386` | 1024 MB | 40 GB | ES1370 |
| Windows 7 x64 | `qemu-system-x86_64` | 1024 MB | 40 GB | ES1370 |

XP and 7 have no VirtIO drivers in the box, so stay with **IDE** or **AHCI (SATA)** disks and an e1000 or RTL8139 NIC. Moving to VirtIO later means loading the drivers inside the guest first.

---

Next: [Operating System Catalog](os-catalog.md) · [User Guide](user-guide.md) · [Documentation Hub](../README.md)
