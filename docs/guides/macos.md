# macOS and Classic Mac OS (Experimental)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

> **Experimental.** Intel macOS in AQEMU is an experimental profile, and the app labels it "Intel macOS (experimental)". It is not a supported feature, and **it may not boot on your hardware**. Nothing here promises that it will.

> **Apple software: you're on your own, on purpose.** AQEMU does not include, and cannot give you, any Apple software: no macOS or Mac OS installers, no Mac ROMs, no SMC key (OSK). We also can't tell you where to get any of it. Use your brain: you must obtain anything proprietary yourself, legally, and follow Apple's licence terms. AQEMU also doesn't ship OpenCore or OVMF for this profile. The wizard makes you tick "I will supply all Apple / OpenCore / OVMF files myself".

AQEMU has three Mac paths:

| Path | Emulator | Guide section |
| --- | --- | --- |
| **Intel macOS (experimental)** | `qemu-system-x86_64` + `q35` + OpenCore + OVMF | [Intel macOS](#1-intel-macos-experimental) |
| **Classic Mac OS 7 to 9 and Mac OS X on PowerPC** | `qemu-system-ppc` (`mac99` / `g3beige`) | [PowerPC](#2-classic-mac-os-and-mac-os-x-on-powerpc) |
| **Mac OS 6 on 68k** | `qemu-system-m68k` (`q800`) | [68k](#3-mac-os-6-68k) |

> **"macOS Apple Silicon (ARM64)"** appears in the wizard's Apple list. There is no separate profile for it. AQEMU sets it up exactly like the [iOS / Apple SoC](ios-apple-soc.md) VM (Inferno, machine `t8030`). It is not a way to run Apple Silicon macOS.

---

## 1. Intel macOS (experimental)

### What you need

| Item | Notes |
| --- | --- |
| **OpenCore** | A bootable OpenCore ISO or disk image that you prepare yourself. An `.iso` attaches as a CD/DVD. |
| **Installer disk image** | Your own macOS installer image, chosen in the wizard's installer image field |
| **Apple SMC OSK** | You paste your own. AQEMU **never pre-fills it** and refuses to start this profile without one: "Apple SMC OSK is empty. Paste your own OSK string into the VM settings. AQEMU does not provide one." |
| **OVMF** | x86 UEFI firmware. The wizard looks for it and shows "OVMF CODE found: …" or tells you to install OVMF / edk2 firmware. |

### What AQEMU builds (all hosts)

| Setting | Value |
| --- | --- |
| Emulator / machine | `qemu-system-x86_64`, `q35` |
| CPU Type | `Skylake-Client-v4` |
| CPU line AQEMU passes | `-cpu Skylake-Client-v4,vendor=GenuineIntel,+invtsc,tsc-frequency=2000000000` |
| CPU topology | `-smp sockets=1,cores=N,threads=1` (2 cores by default) |
| Memory | 4096 MB minimum |
| SMC | `-device isa-applesmc,osk=<your key>` (commas in the key are escaped for you) |
| Network | `virtio-net-pci` (AQEMU forces this for the profile) |
| Display | VMware SVGA at the host's native resolution |
| Audio | Intel HDA (`ich9-intel-hda`) |
| Input | USB tablet on xHCI, USB hub |

If you set the CPU Type to `max`, `host`, a `qemu…` model or `Penryn`, AQEMU swaps it back to `Skylake-Client-v4` at launch.

### Which host to use

macOS here is an **x86_64 guest**, so it only gets hardware acceleration on an **x86_64 host**.

| Host | Accelerator | CPU | Verdict |
| --- | --- | --- | --- |
| **Windows x64** | WHPX by default, or KVM inside WSL with **Launch via WSL/KVM** | Skylake line above. In WSL with KVM AQEMU also adds `kvm=on,vmware-cpuid-freq=on`. | The best Windows option. Booting is not verified. |
| **Windows ARM64** | TCG only | Skylake line above | Full emulation. Expect it to be far too slow. |
| **Linux x64** | KVM | Skylake line above | The best overall option. Booting is not verified. |
| **Linux ARM64** | TCG only | Skylake line above | Full emulation. Expect it to be far too slow. |

Under pure TCG, QEMU warns that `invtsc` isn't supported.

#### Windows x64

1. Install AQEMU (see [Install from the Microsoft Store](getting-started/install-from-store.md)).
2. Create the VM ([below](#create-the-vm)). The profile picks **WHPX** on an x86_64 host.
3. Optional, to use KVM inside WSL instead:
   - Install WSL and a distro, then run **File → Configure WSL…**.
   - In **File → Preferences… → WSL / KVM launch**, tick **Enable Launch via WSL/KVM (for Intel macOS and other VMs)**. **QEMU binary in WSL** defaults to `qemu-system-x86_64`, so that must be installed in your distro. **Probe /dev/kvm** should report "Status: /dev/kvm OK".
   - Tick **Prefer WSL/KVM when /dev/kvm is available (Windows)** in the wizard, or **Launch via WSL/KVM** on the Machine tab afterwards.
4. Click **Start** and follow [Install](#install).

**Limitations:** these docs don't confirm that macOS boots under WHPX. The AMD GPU passthrough option is "bare-metal Linux with VFIO — not Windows/WSL".

#### Windows ARM64

1. Create the VM the same way. AQEMU will use TCG, because WHPX on ARM can't run an x86_64 guest, and neither can KVM in WSL.
2. Start and wait. It's included here for completeness, not as a recommendation.

#### Linux x64

1. Make sure `/dev/kvm` works for your user (see [Linux x64 host guide](hosts/linux-x64.md#turn-on-kvm)) and that OVMF is installed (the wizard tells you).
2. Create the VM, set **Accelerator** to **KVM** on the Machine tab if it isn't already, and click **Start**.
3. Optional and advanced: if AQEMU detects an AMD GPU, the wizard offers AMD GPU passthrough. It "only works on bare-metal Linux with VFIO". The wizard says to install macOS first, then enable passthrough.

#### Linux ARM64

The same steps as Linux x64, but the guest runs under TCG. Expect it to be far too slow for real use.

### Create the VM

1. **VM → Add VM → Add New VM** (`Ctrl+N`), then **Guest Operating System → Apple → macOS** (or **Mac OS X Intel**, or **Darwin**: all three use the same Intel macOS profile).
2. The **Intel macOS Setup** page asks for:
   - **OpenCore (ISO or disk image)**
   - **System disk:** **Create a new disk image** or **Use an existing disk image**
   - **Installer disk image**
   - **Apple SMC OSK (required to start)**: "Paste your own OSK — AQEMU never pre-fills this"
   - **I will supply all Apple / OpenCore / OVMF files myself** (must stay ticked)
   - **Prefer WSL/KVM when /dev/kvm is available (Windows)** (Windows only)
3. Finish. Afterwards, these settings live on the Machine tab's Intel macOS panel and in **Machine → Advanced → Enable Intel macOS profile (q35 + OpenCore + SMC)**.

### Install

1. Click **Start**. If the OpenCore path is missing, AQEMU stops with "OpenCore path is missing or does not exist". OpenCore's picker should appear.
2. Pick the installer, use Disk Utility to erase the system disk as **APFS**, then install.
3. After the reboots, pick the system disk in OpenCore.

If it doesn't boot, open an [issue](https://github.com/chronic8000/aqemu/issues) with your QEMU arguments (**Show QEMU Arguments** on the toolbar), **with the OSK removed**.

---

## 2. Classic Mac OS and Mac OS X on PowerPC

**Guest Operating System → Apple → Mac OS 7 / Mac OS 8 / Mac OS 9 / Mac OS X PPC**, or the **Mac OS Classic PPC** template under **Custom / Advanced → Template**.

| | Mac OS 7 / 8 / 9 | Mac OS X PPC | Mac OS Classic PPC template |
| --- | --- | --- | --- |
| Emulator | `qemu-system-ppc` | `qemu-system-ppc` | `qemu-system-ppc` |
| Machine | `mac99` (or `g3beige`) | `mac99` (or `g3beige`) | `mac99` |
| CPU | G4 | G4 | default |
| RAM | 256 MB | 512 MB | 512 MB |
| Disk | 4 GB, IDE | 10 GB, IDE | 10 GB |
| Network | sungem | sungem | sungem |
| Sound | screamer (or none) | screamer (or none) | |

**On every host (Windows x64, Windows ARM64, Linux x64, Linux ARM64) this is TCG emulation.** No accelerator can run PowerPC on an x86 or ARM CPU. The CPU is the G4 model from the profile on every host.

1. Check that your QEMU has `qemu-system-ppc`. The Store build bundles its own QEMU. A self-built QEMU needs the `ppc-softmmu` target.
2. Create the VM, attach your own install CD/ISO on the **Media** tab, and click **Start**.

## 3. Mac OS 6 (68k)

**Guest Operating System → Apple → Mac OS 6** uses `qemu-system-m68k` with the Quadra 800 machine (`q800`), 64 MB, a 2 GB SCSI disk, and no sound. The profile itself says "Experimental; supply your own disk/ROM image." It is TCG on every host.

NeXTSTEP and OPENSTEP are also listed under Apple. See [Unix and workstation systems](guest-os/unix.md).

---

Related: [iOS / Apple SoC (experimental)](ios-apple-soc.md) · [Accelerators](features/accelerators.md) · [Documentation Hub](../README.md)
