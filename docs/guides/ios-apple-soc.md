# iOS and Apple SoC (Experimental)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

> **Experimental.** This is research tooling, not a supported feature and not an "iPhone simulator". AQEMU drives a third-party QEMU fork called Inferno (`qemu-system-applesoc`). It is slow, it needs a lot of manual preparation, and **it may never reach the home screen on your machine**. Nothing on this page promises that iOS will boot.

> **Apple software: you're on your own, on purpose.** AQEMU does not include, and cannot give you, any Apple software: no IPSWs, firmware, kernels, DeviceTrees, tickets, SEP files or ROMs. We also can't tell you where to get them. Use your brain: you must obtain anything proprietary yourself, legally, and follow Apple's licence terms. If you can't do that, don't use this feature.

**Contents**

1. [How it works](#1-how-it-works)
2. [Pick your host](#2-pick-your-host)
3. [Windows x64](#3-windows-x64)
4. [Windows ARM64](#4-windows-arm64)
5. [Linux x64](#5-linux-x64)
6. [Linux ARM64](#6-linux-arm64)
7. [Create the iOS VM (all hosts)](#7-create-the-ios-vm-all-hosts)
8. [Create the restore companion (all hosts)](#8-create-the-restore-companion-all-hosts)
9. [The Apple tools (VM → Apple)](#9-the-apple-tools-vm--apple)
10. [Order of operations](#10-order-of-operations)
11. [Hardware buttons](#11-hardware-buttons)
12. [Limitations](#12-limitations)

---

## 1. How it works

There are always **two VMs**:

| VM | What it is | Emulator |
| --- | --- | --- |
| **iOS VM** | An emulated iPhone 11 board (machine `t8030`) | `qemu-system-applesoc` (Inferno) |
| **Restore companion** | An ordinary Ubuntu Server VM that runs the restore tools and talks to the iOS VM over a USB bridge | Normal QEMU (`x86_64`) |

The USB bridge between them is a TCP port (`127.0.0.1:8030`) or a UNIX socket (`/tmp/InfernoUSBRemote`). Both VMs must use the same setting.

## 2. Pick your host

The guest is an ARM64 Apple board. Hardware acceleration (KVM or WHPX) only works when the guest CPU family matches the host, so **x64 hosts always emulate (TCG)**. Xen is never offered for this VM.

| Host | Emulator AQEMU starts | Accelerator | CPU | USB bridge default | Companion |
| --- | --- | --- | --- | --- | --- |
| **Windows x64** | Windows `qemu-system-applesoc.exe`. Falls back to WSL if it's missing. | TCG | `max` | IPv4 `127.0.0.1:8030` | Started in WSL |
| **Windows ARM64** | Windows `qemu-system-applesoc.exe`, or the Linux build in WSL if you pick KVM and WSL has `/dev/kvm` | TCG (KVM only through WSL) | `max` | IPv4 `127.0.0.1:8030` | Started in WSL |
| **Linux x64** | Native `qemu-system-applesoc` | TCG | `max` | UNIX `/tmp/InfernoUSBRemote` | Run its command in a terminal |
| **Linux ARM64** | Native `qemu-system-applesoc` | KVM if `/dev/kvm` works, otherwise TCG | `max` | UNIX `/tmp/InfernoUSBRemote` | Run its command in a terminal |

**CPU model on every host.** The wizard sets the CPU Type to `max`, and AQEMU passes it straight through as `-cpu max`. Change it on the Machine tab only if you know you need to.

---

## 3. Windows x64

**Accelerator:** TCG (full emulation). WHPX can't run an ARM guest on an x64 CPU, and AQEMU forces this VM to TCG.
**CPU:** `max`.

**How AQEMU starts it.** AQEMU uses the Windows `qemu-system-applesoc.exe` from its QEMU folder, with TCG. If that file isn't there, AQEMU asks whether to start it with WSL instead. If you say yes, AQEMU copies its own Linux build (`wsl/<cpu>/qemu-system-applesoc` from the app folder) into `/usr/local/bin` in your WSL distro and runs it there. If neither file is in your AQEMU package, starting fails with:

> "Windows QEMU for this VM is not installed, and WSL is not available. Install qemu-system-applesoc.exe next to AQEMU, or start WSL and try again."

**Step by step:**

1. Install WSL with an Ubuntu distro, then run **File → Configure WSL…** in AQEMU and pick the distro and user. You need WSL even if the iOS VM itself runs as a Windows `.exe`, because the companion and the device tools run through WSL.
2. Create the iOS VM ([section 7](#7-create-the-ios-vm-all-hosts)) and the companion ([section 8](#8-create-the-restore-companion-all-hosts)).
3. Leave **USB remote** on **IPv4 localhost (recommended)**, `127.0.0.1`, port `8030`. **Apply localhost preset (127.0.0.1:8030)** in the Apple SoC Restore window sets this for you.
4. Follow the [order of operations](#10-order-of-operations). In the Apple SoC Restore window use **Start companion in WSL** and **Stop companion**, and **Diagnose USB bridge** if the VMs can't see each other.

**Limitations:** slow. Expect long waits at every stage.

## 4. Windows ARM64

**Accelerator:** TCG by default. WHPX is not used for this VM.
**CPU:** `max`.

**How AQEMU starts it.** It's the same as Windows x64: the Windows `qemu-system-applesoc.exe` with TCG, or the WSL fallback if the `.exe` is missing. There is one extra path. If you set the Accelerator to **KVM** and your WSL distro has a usable `/dev/kvm`, AQEMU runs the Linux build inside WSL with `accel=kvm`. It copies the build into the distro, fixes `/dev/kvm` access, and asks you to set up WSL first if no distro or user is configured. Whether your WSL exposes `/dev/kvm` depends on your Windows and WSL setup. AQEMU only uses it when it's there.

**Step by step:**

1. Set up WSL and **File → Configure WSL…** as for [Windows x64](#3-windows-x64).
2. Create the iOS VM and the companion.
3. Optional: on the Machine tab, set **Accelerator** to **KVM** to try the WSL/KVM path. If WSL has no `/dev/kvm`, stay on TCG.
4. Use **IPv4 localhost**, `127.0.0.1:8030`, then follow the [order of operations](#10-order-of-operations).

**Limitations:** the KVM path is untested in these docs. TCG is the expected path.

## 5. Linux x64

**Accelerator:** TCG only. KVM can't run an ARM guest on an x64 CPU.
**CPU:** `max`.

**Getting the emulator.** AQEMU runs `qemu-system-applesoc` like any other QEMU binary, so it must be in the QEMU folder AQEMU uses. If you build QEMU yourself from the AQEMU repo, the special target builds it:

```bash
./scripts/build_qemu_linux_x86_64.sh applesoc
```

**Step by step:**

1. Make sure `qemu-system-applesoc` is available (above).
2. Create the iOS VM and the companion.
3. Use **UNIX socket**, `/tmp/InfernoUSBRemote`, on both VMs (the Linux default). IPv4 `127.0.0.1:8030` also works if you set it on both.
4. In **VM → Apple → Apple SoC Restore…**, click **Copy companion command** and run it in a terminal. On Linux, AQEMU tells you: "On Linux, run the companion command shown above in a terminal." To stop it, stop that QEMU process in the terminal.
5. Follow the [order of operations](#10-order-of-operations).

**Limitations:**
- Slow (TCG).
- **Diagnose USB bridge** is "implemented for the Windows/WSL path" only.
- The **Device** tools in *iOS Device Tools / Guest Internet* are "currently wired for Windows + WSL".

## 6. Linux ARM64

**Accelerator:** KVM when `/dev/kvm` is usable. AQEMU then passes `accel=kvm:tcg`, so QEMU falls back to TCG if KVM can't start. Without KVM you get TCG.
**CPU:** `max`, as the wizard set it. AQEMU only picks `host` by itself when the CPU Type is empty.

**Getting the emulator:**

```bash
./scripts/build_qemu_linux_arm64.sh applesoc
```

(The Raspberry Pi 5 QEMU script doesn't build this target.)

**Step by step:** the same as [Linux x64](#5-linux-x64). For KVM, make sure your user can open `/dev/kvm` (see [Linux ARM64 host guide](hosts/linux-arm64.md)).

**Limitations:** whether Inferno runs well under KVM on your board is not verified. Same Windows-only tool limits as Linux x64.

---

## 7. Create the iOS VM (all hosts)

**Guest Operating System → Apple → iOS (ARM64)**. The wizard sets:

| Setting | Value |
| --- | --- |
| Emulator | `qemu-system-applesoc` |
| Machine | `t8030` |
| CPU | `max` |
| Memory | at least 4096 MB |
| vCPUs | 4 (Inferno needs at least 2) |
| Video | none (no PC VGA) |
| Input | USB tablet, USB hub |
| KASLR | off (**Disable KASLR (kaslr-off)** is ticked) |
| UEFI | off |
| USB remote | Windows: IPv4 `127.0.0.1`:`8030`. Linux: UNIX `/tmp/InfernoUSBRemote`. |

The wizard also asks for the **NAND (root) size**: 16 / 32 / 64 / 128 / 256 GiB, or Custom (16 to 2048 GiB). The default is 32 GiB.

After creating the VM, the **Machine** tab has the Apple SoC fields. You fill in your own files: **Trustcache**, **Restore ticket**, **SEP firmware**, **SEP ROM**, **SecureROM (optional)**, **IPSW (restore)**, **Restore ramdisk (-initrd)**, plus **NAND (root) size**, **Disable KASLR**, **USB remote** and **Fill suggested boot args**.

## 8. Create the restore companion (all hosts)

**Guest Operating System → Apple → iPhone IPSW Restore Companion**. It's also listed under Linux as *IPSW Restore Companion (Ubuntu)*.

This makes a normal AQEMU VM: x86_64 `q35`, CPU `max`, 2048 MB, at least 2 vCPUs, a `companion.qcow2` disk, with the Ubuntu Server ISO as a CD. When it's installed you can SSH in from the host:

```
ssh -p 32222 user@127.0.0.1
```

The **Apple SoC Restore** window asks for the companion's **SSH user** and **SSH password**. Those are the Linux login you created inside the companion, not an Apple account.

## 9. The Apple tools (VM → Apple)

| Menu item | What it's for |
| --- | --- |
| **iOS Firmware Tool…** | Unpacks an IPSW you supply and prepares the files the Machine tab asks for. It needs the `img4` and `pyimg4` tools installed and tells you if they're missing. |
| **Apple SoC Restore…** | Companion setup, USB remote settings, start/stop the companion (WSL), **Diagnose USB bridge**, **Restore IPSW via SSH…** |
| **Apply iOS filesystem patches…** | Patches the iOS VM's `root` disk after a restore. Power off the iOS VM first. |
| **Guest Internet / iOS Device Tools…** | **Enable guest internet (reverse-tether)**, plus device info and screenshots through the companion (Windows + WSL only) |
| **Wipe Inferno disks…** | Start again with clean NAND images |

> Some older help text inside the app still says **File →** for these items. They live under **VM → Apple**.

## 10. Order of operations

1. **iOS Firmware Tool:** unpack your IPSW and prepare your files. All the files must come from the **same iOS build**. AQEMU warns you about mismatches, and the restore fails if they differ.
2. Fill in the file paths on the iOS VM's **Machine** tab. Set the NAND size **before** the first restore.
3. **Apple SoC Restore:** set the companion disk and SSH login, start the companion (WSL button on Windows, terminal command on Linux), and wait for SSH on port 32222.
4. Start the iOS VM. At best you'll see a restore screen, not a home screen.
5. Click **Restore IPSW via SSH…**. When it finishes, QEMU exits. That's expected.
6. Power off and run **Apply iOS filesystem patches…** on the iOS VM's `root` disk (not the companion's).
7. Power on again.

## 11. Hardware buttons

While the VM runs, click **Pad** on the session toolbar ("Show / hide floating iOS button pad"):

| Button | Inferno key | Action |
| --- | --- | --- |
| **Vol−** | F3 | Volume down |
| **Vol+** | F4 | Volume up |
| **Side** | F5 | Click: sleep/wake. Hold about 2 s: power menu |
| **Home** | F6 | Click: Home. Double-click: App Switcher |
| **SOS** | | Vol-up + Side combo |

## 12. Limitations

- **Experimental.** It may not boot at all on your setup.
- Emulated (TCG) on every x64 host. KVM is only possible on ARM64 hosts and is unverified.
- On Windows, the companion and device tools need WSL.
- These docs don't confirm whether `qemu-system-applesoc.exe` or the WSL Linux build is included in every AQEMU package. If yours lacks both, AQEMU tells you when you press Start.
- No app-install workflow is documented or supported here.

---

Related: [macOS (experimental)](macos.md) · [New VM wizard](features/new-vm-wizard.md) · [Accelerators](features/accelerators.md) · [Documentation Hub](../README.md)
