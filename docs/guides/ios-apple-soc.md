# iOS and Apple SoC (Experimental)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

> **Experimental.** This is research tooling. It isn't a supported feature, and it isn't an "iPhone simulator". AQEMU drives an Apple SoC emulator (`qemu-system-applesoc`) that models one iPhone board. It is slow, it needs a lot of manual preparation, and **it may never reach a home screen on your machine**. Nothing on this page promises that iOS will boot.

> **Apple software: you're on your own, on purpose.** AQEMU does not include any Apple firmware or software: no IPSWs, kernels, DeviceTrees, tickets, SEP files, ROMs or keys. We can't distribute them, and we can't tell you where to get them. If you use this feature, you source anything proprietary yourself, legally, using your own judgement, and you follow Apple's licence terms. If you can't do that, don't use this feature.

> **Third-party links inside the app.** A few buttons and messages in the Apple dialogs open or mention third-party websites. Those sites aren't part of AQEMU, and these docs don't cover them.

**Contents**

1. [How it works](#1-how-it-works)
2. [What you need, per host](#2-what-you-need-per-host)
3. [What AQEMU automates on each host](#3-what-aqemu-automates-on-each-host)
4. [One-time setup](#4-one-time-setup)
5. [Create the restore companion VM](#5-create-the-restore-companion-vm)
6. [Create the iOS VM](#6-create-the-ios-vm)
7. [The Machine tab (Apple SoC fields)](#7-the-machine-tab-apple-soc-fields)
8. [VM → Apple → iOS Firmware Tool…](#8-vm--apple--ios-firmware-tool)
9. [VM → Apple → Apple SoC Restore…](#9-vm--apple--apple-soc-restore)
10. [VM → Apple → Apply iOS filesystem patches…](#10-vm--apple--apply-ios-filesystem-patches)
11. [VM → Apple → Guest Internet / iOS Device Tools…](#11-vm--apple--guest-internet--ios-device-tools)
12. [VM → Apple → Wipe Inferno disks…](#12-vm--apple--wipe-inferno-disks)
13. [Order of operations (Windows + WSL)](#13-order-of-operations-windows--wsl)
14. [The boot process: what to expect](#14-the-boot-process-what-to-expect)
15. [Hardware buttons](#15-hardware-buttons)
16. [Limitations and what isn't working yet](#16-limitations-and-what-isnt-working-yet)
17. [Troubleshooting](#17-troubleshooting)
18. [FAQ](#18-faq)

---

## 1. How it works

You always run **two VMs at the same time**:

| VM | What it is | Emulator |
| --- | --- | --- |
| **iOS VM** | An emulated iPhone 11 board (machine `t8030`, model `n104ap`) | `qemu-system-applesoc` |
| **Restore companion** | An ordinary Ubuntu Server VM. The restore tools (`usbmuxd`, `idevicerestore` and friends) run **inside** it. | An x86_64 QEMU build with a `usb-tcp-remote` device, named `qemu-system-x86_64-inferno` |

The emulated iPhone's USB port isn't connected to your host. It's carried over a **USB bridge**: a TCP port (`127.0.0.1:8030`) or a UNIX socket (`/tmp/InfernoUSBRemote`). The companion listens on the bridge, the iOS VM connects to it, and the phone then appears as a USB device inside the companion. **Both VMs must use the same bridge setting**, and the companion must be listening **before** the iOS VM starts.

The menu items for all of this are under **VM → Apple**, in this order:

1. **iOS Firmware Tool…**
2. **Apple SoC Restore…**
3. **Apply iOS filesystem patches…**
4. **Guest Internet / iOS Device Tools…**
5. **Wipe Inferno disks…**

If you open one without an iOS VM selected, AQEMU says: "Add or select an Apple SoC / iOS (Inferno) virtual machine first."

## 2. What you need, per host

The guest is an ARM64 Apple board. Hardware acceleration only works when the guest CPU family matches the host, so **x64 hosts always emulate (TCG)**. Xen is never offered for this VM. The CPU Type is `max` on every host, and AQEMU passes it straight through as `-cpu max`.

| Host | iOS VM emulator AQEMU starts | Accelerator | Companion | Guided restore |
| --- | --- | --- | --- | --- |
| **Windows x64** | Windows `qemu-system-applesoc.exe`. If it's missing, AQEMU offers to run its Linux build in WSL instead. | TCG | Started in WSL | Yes |
| **Windows ARM64** | Windows `qemu-system-applesoc.exe`, or the Linux build in WSL if you set **KVM** and WSL has `/dev/kvm` | TCG (KVM only through WSL) | Started in WSL | Yes |
| **Linux x64** | Native `qemu-system-applesoc` | TCG | You run its command in a terminal | No (manual) |
| **Linux ARM64** | Native `qemu-system-applesoc` | KVM if `/dev/kvm` works (`accel=kvm:tcg`), otherwise TCG | You run its command in a terminal | No (manual) |

**Memory and CPU.** The iOS VM is set to **4096 MB** and **4 vCPUs** (it needs at least 2). The companion is set to **2048 MB** and 2 vCPUs. That's about 6 GB of guest memory while both run, on top of what your host itself needs.

**Disk space.**

- iOS VM: the **NAND (root)** image is the size you pick (default **32 GiB**), plus about 276 MiB of small images (listed in [section 14](#14-the-boot-process-what-to-expect)). AQEMU checks free space first and wants the total **plus 512 MiB** free. On Windows the root image is sparse, so it only uses real space as it fills.
- Companion: a **20 GB** `companion.qcow2` (it grows as it fills) and the Ubuntu Server ISO (about 2–3 GB).
- Your own firmware files and the folder they're unpacked into.

**Software on Windows (x64 and ARM64):**

- **WSL** with an Ubuntu distro, set in **File → Configure WSL…**. You need it even when the iOS VM runs as a Windows `.exe`, because the companion, the restore and the device tools all run through WSL.
- Inside WSL: `qemu-system-x86_64-inferno` in `/usr/local/bin` and SeaBIOS (`/usr/share/seabios/bios-256k.bin`). AQEMU doesn't build or install the companion emulator for you. `sshpass` is installed automatically the first time you restore.
- For the filesystem patch step: **KVM inside WSL** (nested virtualization).
- For the Firmware Tool: **Python 3**, **img4** (img4lib) and **pyimg4**. Set their paths in **File → Preferences… → Emulator → iOS firmware tools**, or leave the boxes empty to use `PATH`. AQEMU also looks next to `aqemu.exe`.

**Software on Linux (x64 and ARM64):**

- `qemu-system-applesoc` in the QEMU folder AQEMU uses. If you build QEMU from the AQEMU repo, the special target builds it: `./scripts/build_qemu_linux_x86_64.sh applesoc` or `./scripts/build_qemu_linux_arm64.sh applesoc`. (The Raspberry Pi 5 script doesn't build this target.)
- `/usr/local/bin/qemu-system-x86_64-inferno` and SeaBIOS for the companion. AQEMU's build scripts don't build it.
- Python 3, img4 and pyimg4 for the Firmware Tool.
- For KVM on ARM64, your user must be able to open `/dev/kvm` (see [Linux ARM64](hosts/linux-arm64.md)).

## 3. What AQEMU automates on each host

The guided restore flow is built for **Windows + WSL**. On Linux, several buttons only print a message.

| Step | Windows + WSL | Linux |
| --- | --- | --- |
| Firmware Tool, Step 1 (Unpack IPSW) | Yes. It uses PowerShell `Expand-Archive`. | Uses PowerShell too, so expect it to fail. Unzip the IPSW yourself and browse to the files in Steps 2–4. |
| Firmware Tool, Steps 2–4 | Yes | Yes, if Python 3, img4 and pyimg4 are installed |
| **Start companion in WSL** / **Stop companion** | Yes | No. "On Linux, run the companion command shown above in a terminal." |
| **Diagnose USB bridge** | Yes | No. "Diagnose is implemented for the Windows/WSL path." |
| **Restore IPSW via SSH…** | Yes | No. "Automated companion SSH restore is implemented for the Windows/WSL path." |
| **Apply filesystem patches…** | Yes (a temporary Ubuntu KVM guest in WSL) | No. You run `extras/Inferno/apply-fs-patches-wsl.sh` with `ROOT_IMG` set. |
| Device Tools | Yes | No. "Device Tools SSH helper is currently wired for Windows + WSL." |
| Wipe Inferno disks | Yes | Yes |

On Linux, these docs don't walk through the manual restore inside the companion.

## 4. One-time setup

1. **Windows:** install WSL with Ubuntu, then **File → Configure WSL…** and pick the distro and user. Put `qemu-system-x86_64-inferno` and SeaBIOS into that distro.
2. Install Python 3, img4 and pyimg4 on the host, and set them in **File → Preferences… → Emulator → iOS firmware tools** if they aren't on `PATH`. (The Firmware Tool's own help text calls this "File → Configure → iOS firmware tools". The menu item is now **Preferences…**.)
3. **Optional, Windows:** if your WSL copy of `qemu-system-applesoc` isn't in `/usr/local/bin`, set **File → Preferences… → Emulator → Apple SoC binary in WSL**.

## 5. Create the restore companion VM

Run the New VM wizard and pick **Guest Operating System → Apple → iPhone IPSW Restore Companion**. It's also listed under Linux as *IPSW Restore Companion (Ubuntu)*.

When you click **Finish**, AQEMU:

1. Downloads the official **Ubuntu Server 24.04** live ISO (about 2–3 GB), unless you already have one. It asks first: "Continue download?" If the download fails, it tells you where to put the ISO by hand.
2. Creates a **20 GB** disk, `companion.qcow2`.
3. Mounts the ISO as the CD-ROM so the first **Power On** boots the installer.
4. Forwards SSH: host `127.0.0.1:32222` → guest port 22.

The VM is x86_64 `q35`, CPU `max`, 2048 MB, 2 vCPUs, `virtio-vga`, virtio disk and network. AQEMU then shows **Companion setup — do these steps next**, a checklist with a **Copy all** button.

**Inside the companion (you do this once):**

1. Power it on and install Ubuntu Server. Enable **OpenSSH** when asked, and create a user. That username and password are what AQEMU's **Companion SSH user/password** boxes mean. They're a Linux login, not an Apple account.
2. Shut down, remove the ISO (or put the hard disk first in the boot order), and power on again.
3. Install the iDevice tools the checklist lists (`usbmuxd`, the libimobiledevice stack and `idevicerestore`). AQEMU's restore script checks these and stops with a message if something is missing. The `idevicerestore` build has to be patched for this emulated board, otherwise the restore fails with "Unable to discover device type". The source repo includes the patch as `extras/Inferno/idevicerestore.patch`.
4. Give your companion user passwordless `sudo`. The restore script checks for it ("Checking passwordless sudo...") and stops if it's missing.
5. **Power Off the companion VM in the AQEMU list.** For restores, AQEMU starts the same disk itself with the bridge attached. Only one can use the disk at a time ("same disk, one lock").

To SSH in from Windows, use WSL: `wsl ssh -p 32222 user@127.0.0.1`. A plain Windows `ssh` to that port is refused. On Linux: `ssh -p 32222 user@127.0.0.1`.

## 6. Create the iOS VM

Run the New VM wizard and pick **Guest Operating System → Apple → iOS (ARM64)**. The wizard sets:

| Setting | Value |
| --- | --- |
| Emulator | `qemu-system-applesoc` |
| Machine | `t8030` |
| CPU | `max` |
| Memory | at least 4096 MB |
| vCPUs | 4 (needs at least 2) |
| Video | none (no PC VGA) |
| Input | USB tablet, USB hub |
| KASLR | off (**Disable KASLR (kaslr-off)** is ticked) |
| UEFI | off |
| USB remote | Windows: IPv4 `127.0.0.1`:`8030`. Linux: UNIX `/tmp/InfernoUSBRemote`. |

The disk page becomes **NAND (root) size**: 16 / 32 / 64 / 128 / 256 GiB, or **Custom…** from 16 to 2048 GiB. The default is **32 GiB**. Less than 16 is refused ("Pick at least 16 GiB…"). Pick the size now: changing it later doesn't grow a phone that's already restored.

The VM is created with no Apple files. You fill them in on the Machine tab.

## 7. The Machine tab (Apple SoC fields)

AQEMU never invents a boot file. Everything here is a file you supply.

| Field | What it is | Notes |
| --- | --- | --- |
| **Machine type** | `t8030` | Required |
| **DeviceTree (.dec / .dtb / .im4p)** | Passed as `-dtb` | Required. The Firmware Tool fills it if it's empty. |
| **Kernel (.research / .elf)** | The kernel | Required. The Firmware Tool doesn't fill it. |
| **Boot Arguments** | Passed as `-append` | Optional. **Fill suggested boot args** inserts `tlto_us=-1 mtxspin=-1 agm-genuine=1 agm-authentic=1 agm-trusted=1 serial=3 wdt=-1 -vm_compressor_wk_sw`. |
| **Trustcache** | | Required unless **SecureROM** is set. Auto-filled from `Firmware/<ramdisk>.trustcache` when that file exists. |
| **Restore ticket** | The `root_ticket.der` from Firmware Tool Step 2 | Must exist if set. The Firmware Tool always sets it. |
| **SEP firmware** | The **packed** `.new.img4` from Firmware Tool Step 3 | Not the raw `.im4p`. The tooltip warns the raw file causes error -256 after NORData. |
| **SEP ROM** | | Set both SEP fields, or neither. |
| **SecureROM (optional)** | | Must exist if set |
| **IPSW (restore)** | The IPSW you restore from | The Firmware Tool fills it if it's empty. |
| **Restore ramdisk (-initrd)** | The restore ramdisk named in BuildManifest | While it's set, AQEMU boots in recovery mode (`boot-mode=enter_recovery`). It's cleared automatically after filesystem patches succeed. |
| **NAND (root) size** | 16–2048 GiB, default 32 | Set before the first restore |
| **Disable KASLR (kaslr-off)** | | Ticked by default |
| **USB remote** | **(none / Inferno UNIX default)**, **IPv4 localhost (recommended)**, or **UNIX socket**, plus address and port (default `8030`) | Must match the companion |

Buttons: **Fill suggested boot args**, **Restore IPSW…** (opens Apple SoC Restore), **Wipe Inferno disks…**, and **Apply filesystem patches…** ("Required for SpringBoard / setup").

## 8. VM → Apple → iOS Firmware Tool…

The window is titled **iOS Firmware Tool**, with the heading **iOS Firmware Unpacker**. It prepares the files the Machine tab asks for **from an IPSW you supply**. It works through four steps. Under them are an **Output log**, an **Extracted file locations** list with **Copy locations**, and **Close**. Only one task runs at a time ("Wait for the current task to finish.").

### Step 1: Unpack IPSW Archive

| Field | Default |
| --- | --- |
| **IPSW File** (`*.ipsw *.zip`) | (empty) |
| **Extraction Output Directory** | `firmware_extracted` in the app folder |

Click **Unpack IPSW**. AQEMU unzips it with PowerShell (so this is Windows-only), then:

- lists the payloads in `Firmware/all_flash`;
- fills in BuildManifest, the two ticket output paths, the DeviceTree `.im4p`, the SEP `.im4p`, and the decrypted and packed SEP paths;
- suggests the restore ramdisk from BuildManifest. If there isn't one, it says so and warns you not to use the largest `.dmg` ("that is usually the OS image");
- sends the IPSW path to the Machine tab;
- logs "Next: Step 2 — Forge restore + SEP tickets."

### Step 2: Forge restore + SEP tickets

| Field | Default |
| --- | --- |
| **Model** | `n104ap — iPhone 11 / t8030` (the only choice) |
| **BuildManifest.plist** | Filled after Unpack IPSW |
| **ticket.shsh2** | Remembered between sessions |
| **Restore ticket (.der)** | `root_ticket.der` |
| **SEP ticket (.der)** | `sep_root_ticket.der` |

You need a `ticket.shsh2` file. It isn't inside the IPSW, and AQEMU doesn't ship it. Click **Forge restore + SEP tickets**. AQEMU finds Python (`py`, `python3` or `python`), installs `pyasn1` with `pip --user` if it's missing, then runs its own scripts `create_apticket.py` and `create_septicket.py` (from `extras/Inferno`) to write both `.der` files. The restore ticket is always copied to Machine → **Restore ticket**.

### Step 3: Pack SEP firmware

| Field | Default |
| --- | --- |
| **SEP .im4p** | Filled after Unpack IPSW |
| **IVKEY (IV then key, no space)** | (empty) |
| **Decrypted SEP** | `sep-firmware.n104.RELEASE` |
| **Packed SEP (.img4)** | `sep-firmware.n104.RELEASE.new.img4` |

The IVKEY is 96 hex characters: the SEP firmware's 32-character IV followed straight away by its 64-character key, for **this exact IPSW build**. Spaces and `iv:`/`key:` prefixes are stripped. AQEMU doesn't supply it.

Click **Decrypt + pack SEP firmware**. AQEMU runs `img4` to decrypt the `.im4p`, then packs it with your SEP ticket into the `.new.img4`, and sets Machine → **SEP firmware**.

### Step 4: Process IM4P (pyimg4)

| Field | Options |
| --- | --- |
| **IM4P Payload File** | Any `.im4p` |
| **Operation** | **Extract raw payload**, **Decrypt payload (--key/--iv)**, **Show payload info** |
| **AES IV**, **AES Key** | Optional, for Decrypt |

Click **Run pyimg4**. The output is `<file>.dec`. If the file name contains `DeviceTree`, the result goes into Machine → **DeviceTree** when that field is empty.

### What the Firmware Tool fills on the Machine tab

| Machine field | When |
| --- | --- |
| DeviceTree | Only if empty |
| Restore ticket | Always |
| IPSW (restore) | Only if empty |
| Restore ramdisk | Only if empty. Trustcache is then set from `Firmware/<ramdisk>.trustcache` if that file exists. |
| SEP firmware | Always |

## 9. VM → Apple → Apple SoC Restore…

The window is titled **Apple SoC Restore (Inferno companion)**. It runs the companion with the USB bridge and drives the restore. Its intro sums up the flow: start the companion here (don't also Power On the Ubuntu companion in the AQEMU list), it keeps running in WSL, Power On iOS with the same USB remote, then enter the SSH login and press **Restore IPSW via SSH…**.

### Fields

| Field | Default | What it does |
| --- | --- | --- |
| **IPSW** | From the VM | The file `idevicerestore` uses |
| **NAND (root) size** | 32 GiB (16/32/64/128/256 or Custom… 16–2048) | Saved to the VM. Doesn't grow a phone that's already restored. |
| **iOS VM file** | (label) | The `.aqemu` path and "Inferno disks: <folder>", or "(no iOS VM selected)" / "(unsaved VM)" |
| **Companion disk** | Remembered | **Browse…** or **Create…** (runs the companion setup in [section 5](#5-create-the-restore-companion-vm)) |
| **Companion SSH user** | `bob` (remembered) | Your Linux user inside the companion |
| **Companion SSH password** | (empty, never saved) | Masked |
| **USB remote type** | **IPv4 localhost (recommended — match both VMs)** or **UNIX socket** | On Windows it's forced to IPv4 unless the VM has an explicit UNIX path. |
| **USB remote addr** | `127.0.0.1` (IPv4) or `/tmp/InfernoUSBRemote` (UNIX) | |
| **USB remote port** | `8030` (1–65535) | IPv4 only |

**Apply localhost preset (127.0.0.1:8030)** saves IPv4 `127.0.0.1` port `8030` on the VM. If the Machine tab still shows UNIX or (none), set it there too, then restart the iOS guest once the companion is listening.

### Companion command

The box **Companion QEMU (Linux guest + usb-tcp-remote; start BEFORE iOS):** shows the exact command, which **Copy companion command** copies. It runs `/usr/local/bin/qemu-system-x86_64-inferno` with `-M q35`, 2048 MB (512 MB with no disk), `-accel tcg`, `-nographic`, the companion disk, SSH forwarded on `32222`, and a USB controller with the `usb-tcp-remote` bridge device. The status line under it tells you if the address is missing or invalid, or if no companion disk is set. A diskless companion only opens the bridge, so the restore tools can't see the phone.

### Buttons

| Button | What it does |
| --- | --- |
| **Start companion in WSL** | Windows only. Starts the command above in WSL, detached. It shares the IPSW's folder into the companion (9p, as `/mnt/aqemu_ipsw/aqemu-restore-current.ipsw`) and stages the restore ticket. Then it waits up to about 3 minutes for SSH on port 32222 ("TCG boot often needs 1-3 minutes"). A companion that's already running with the same settings is reused. Log: `/tmp/aqemu-inferno-companion.log` in WSL. |
| **Stop companion** | Windows only. Stops it. |
| **Diagnose USB bridge** | Windows only. Checks the bridge and the companion's view of USB. |
| **Copy companion command** | Copies the command |
| **Restore IPSW via SSH…** | Windows only. See below. |
| **Wipe Inferno disks…** | See [section 12](#12-vm--apple--wipe-inferno-disks) |
| **Apply filesystem patches…** | See [section 10](#10-vm--apple--apply-ios-filesystem-patches) |

### Restore IPSW via SSH…

1. AQEMU asks you to confirm the IPSW: "idevicerestore will use this file (must match tickets + SEP pack)". If you changed the IPSW, **Stop companion** and **Start companion** first so the shared folder is restaged.
2. It SSHes into the companion (installing `sshpass` in WSL the first time), checks disk space, `sudo`, the shared IPSW and the `idevicerestore` build, and sets up `usbmuxd`.
3. It logs **"Power On the iOS guest NOW (companion SSH is up). Waiting for USB..."**. Power On the iOS VM then, not before. The log warns: "iOS only waits ~120s for restore once the Apple logo appears."
4. When the phone shows up on the companion's USB, it runs `idevicerestore` (erase + restore mode, with your restore ticket). It tries up to twice.
5. At the end you see "Command finished with exit code %1". On success, the next step is filesystem patches.

## 10. VM → Apple → Apply iOS filesystem patches…

The window is titled **Apply iOS filesystem patches (Inferno)**. The step is "Required after idevicerestore for SpringBoard / setup to appear". It patches the **iOS VM's root disk**, not the companion.

| Control | What it does |
| --- | --- |
| **Guest root disk** | Placeholder `…/<VM>_inferno/root` |
| **Browse…** / **Use current VM root** | Pick the file |
| **Apply filesystem patches…** | Runs the patch |

The status line warns if the root image isn't found, if the helper scripts are missing, or that you must Power Off the iOS guest first (the root disk is locked while it runs).

On Windows it starts a temporary Ubuntu KVM guest in WSL with APFS support, so **WSL needs KVM** (nested virtualization). On success: "Dyld cache + LaunchDaemons updated…", and the **Restore ramdisk** field is cleared for you. On Linux the dialog tells you to run `extras/Inferno/apply-fs-patches-wsl.sh` yourself with `ROOT_IMG` set.

## 11. VM → Apple → Guest Internet / iOS Device Tools…

The window is titled **iOS Device Tools / Guest Internet**. It works through the companion over SSH, so it's **Windows + WSL only**. It asks for **Companion SSH user**, **password** and **SSH port** (`32222`). Use the same login as Apple SoC Restore.

- **Internet** tab: **Enable guest internet (reverse-tether)** sets up `usbmuxd` and connection sharing in the companion so the phone can reach the network through it. There are also **Diagnose USB / idevice** and **Copy manual commands**. Whether the guest actually gets online is not verified in these docs.
- **Device** tab: **Device info**, **List apps** and **Screenshot** (saved as `aqemu-ios.png`). The tab has other buttons that v1 docs don't cover.

## 12. VM → Apple → Wipe Inferno disks…

Deletes the iOS VM's NAND images so you can restore from scratch. Select the iOS VM and Power it Off first. The confirmation lists what will be removed (it defaults to **No**). It **doesn't** delete the VM file, your IPSW, the firmware extract or `companion.qcow2`.

Afterwards AQEMU says: "Start companion (if needed), Power On iOS (that writes the placeholder GPT on root), then Restore IPSW via SSH. Do not apply filesystem patches until restore rewrites that GPT."

## 13. Order of operations (Windows + WSL)

1. [One-time setup](#4-one-time-setup), then [create the companion](#5-create-the-restore-companion-vm) and install Ubuntu and the tools in it. **Power Off the companion VM** in the list.
2. [Create the iOS VM](#6-create-the-ios-vm). Choose the NAND size now.
3. **iOS Firmware Tool**, Steps 1 → 2 → 3, plus Step 4 if your DeviceTree is still an `.im4p`. Every file must come from **the same iOS build** as the IPSW.
4. **Machine tab:** check DeviceTree, Restore ticket, SEP firmware, IPSW and Restore ramdisk. Add your Kernel, Trustcache (or SecureROM) and SEP ROM. Set **USB remote** to IPv4 `127.0.0.1:8030`. Optionally click **Fill suggested boot args**.
5. **Apple SoC Restore:** set the companion disk, SSH user and password, and click **Apply localhost preset (127.0.0.1:8030)**.
6. Click **Start companion in WSL**. Wait for SSH (often 1–3 minutes).
7. Click **Restore IPSW via SSH…** and confirm.
8. When the log says **Power On the iOS guest NOW**, Power On the iOS VM. It boots in recovery mode because the Restore ramdisk is set.
9. Wait. When the restore completes, the emulator exits on purpose and AQEMU shows **iOS restore finished - apply filesystem patches**: "Inferno shut down after restore stage 1 (exit 0). That is expected."
10. Click **Apply filesystem patches...** (the iOS VM must be off). When that succeeds, the Restore ramdisk is cleared.
11. Power On the iOS VM. AQEMU's message says to "wait for data migration / setup". How far it gets depends on your setup and files.

## 14. The boot process: what to expect

- **First Power On creates the disks.** AQEMU makes a folder named `<VM file name>_inferno` next to the `.aqemu` file, containing: `root` (your NAND size), `firmware` (256 MiB), `panic_log` (16 MiB), `sep_ssc` (128 KiB), `sep_nvram` (64 KiB), and `syscfg`, `ctrl_bits`, `nvram`, `effaceable` (1 MiB each). `root` gets a placeholder GPT until a restore rewrites it.
- **Checks before launch.** AQEMU refuses to start if a required file is missing (see [Troubleshooting](#17-troubleshooting)). It never adds boot files or boot args you didn't set.
- **The screen takes a long time.** The session shows "Waiting for guest display… (n)" for up to about 3 minutes. After that it warns "Guest display still waiting" but keeps trying, and the VM keeps running.
- **It's slow.** Every x64 host emulates the whole ARM board in software (TCG). Expect long waits at every stage.
- **Lots of `info:`/`warning:` lines are normal.** The emulator prints its kernel-patch steps there. On WSL, ALSA "Failed to create voice" messages are harmless.
- **The log.** Everything the emulator prints goes to `qemu-boot.log` in the `_inferno` folder, with a header and an exit line. Check it first when something stops.
- **An exit after a restore is expected.** See step 9 above.
- **Without filesystem patches the screen stays black.**

## 15. Hardware buttons

While the VM runs, click **Pad** on the session toolbar ("Show / hide floating iOS button pad"):

| Button | Key | Action |
| --- | --- | --- |
| **Vol−** | F3 | Volume down |
| **Vol+** | F4 | Volume up |
| **Side** | F5 | Click: sleep/wake. Hold about 2 s: power menu. |
| **Home** | F6 | Click: Home. Double-click: App Switcher. |
| **SOS** | | Vol-up + Side combo |

## 16. Limitations and what isn't working yet

- **Experimental.** It may not boot at all on your setup.
- **One board only:** `n104ap` / iPhone 11 (`t8030`).
- **Slow:** TCG on every x64 host. KVM is only possible on ARM64 hosts (natively on Linux, through WSL on Windows) and is unverified in these docs.
- **The guided flow is Windows + WSL only.** On Linux you run the companion, the restore and the filesystem patches by hand ([section 3](#3-what-aqemu-automates-on-each-host)).
- **Firmware Tool Step 1 uses PowerShell**, so it doesn't work on Linux.
- **AQEMU doesn't build or ship `qemu-system-x86_64-inferno`** for the companion.
- These docs don't confirm whether `qemu-system-applesoc.exe` or the WSL Linux build is in every AQEMU package. If yours has neither, AQEMU tells you when you press Power On.
- **NAND size can't grow** after a restore. Wipe and restore again.
- **No app-install workflow** is documented or supported here, and the internet tab is unverified.
- **Help text in the app still mentions old menu paths** such as "File → Apple SoC Restore", "File → Apply iOS filesystem patches" and "File → Configure → iOS firmware tools". The real places are **VM → Apple** and **File → Preferences…**.

## 17. Troubleshooting

Messages are quoted from the app. "…" marks text we've shortened.

| Message | What it means / what to do |
| --- | --- |
| "Add or select an Apple SoC / iOS (Inferno) virtual machine first." | Select the iOS VM in the list, then reopen the VM → Apple item. |
| "Choose a Machine type on the MACHINE tab (for Inferno, typically t8030 / iphone11 / similar)." | Set Machine type to `t8030`. |
| "Missing %1. On the MACHINE tab, scroll to Options and choose the file for "%1"." | Kernel or DeviceTree is empty. Fill it in. |
| "%1 file not found:… Pick an existing file in Options on the MACHINE tab." | The path points at a file that's gone. |
| "SEP firmware and SEP ROM must both be set, or both left empty." | Set both, or clear both. |
| "USB remote type is set, but the address is empty…" | Use **Apply localhost preset**, or set `127.0.0.1` / `/tmp/InfernoUSBRemote`. |
| "USB remote is IPv4 but the TCP port is missing." | Set the port to `8030`. |
| "Not enough free disk space for Apple SoC images under… Need about %2 MiB free (have %3 MiB)." | Free space, or pick a smaller NAND size before the first boot. |
| "Cannot format seed APFS: extras/Inferno/format-seed-apfs.sh not found." | The `extras/Inferno` folder is missing next to AQEMU. Reinstall or rebuild. |
| "Windows QEMU for this VM is not installed, and WSL is not available…" | Neither `qemu-system-applesoc.exe` nor WSL is available. Set up WSL. |
| "AQEMU could not put its Linux Apple SoC binary into the WSL distro you selected (%1)… That file is not in this package yet." | Your package has no WSL build. Put `qemu-system-applesoc` in WSL yourself, or set **Apple SoC binary in WSL**. |
| "Linux Inferno binary not found on WSL PATH" | Same as above. |
| "WSL configuration is required to launch this VM." | **File → Configure WSL…** |
| "img4 not found" / "pyimg4 not found" / "Python 3 was not found." | Install it, or set its path in **File → Preferences… → iOS firmware tools**. |
| "Please select a valid .ipsw or .zip firmware file." | Step 1 or the Restore window has no valid IPSW. |
| "Unpack the IPSW first (Step 1), or browse to BuildManifest.plist." | Step 2 needs BuildManifest. |
| "ticket.shsh2 is not inside the IPSW and AQEMU does not ship it." | Step 2 needs a `ticket.shsh2` file you supply. |
| "Could not import/install pyasn1. Install Python 3 and retry." | Python or pip isn't working. |
| "Missing script": "create_apticket.py was not found:" | `extras/Inferno` is missing next to AQEMU. |
| "Need the SEP-Firmware IV (32 hex) immediately followed by its Key (64 hex), 96 characters total. Not iBoot. Must match this IPSW build. You pasted %1 hex character(s)." | Fix the IVKEY in Step 3. |
| "Forge restore + SEP tickets (Step 2) first." | Step 3 needs the SEP ticket. |
| "Could not start img4 pack (decrypted SEP missing)." | The decrypt part of Step 3 failed. Check the IVKEY and the log. |
| "Process failed with exit code %1" | Read the Output log above it. |
| "Missing companion disk. A diskless companion only binds the USB socket…" | Set **Companion disk**. |
| "WSL distro and username are required." | **File → Configure WSL…** |
| "qemu-system-x86_64-inferno not found" | Put the companion emulator in `/usr/local/bin` in WSL. |
| "bios-256k.bin not found — install seabios (and qemu-system-data) in WSL" | `sudo apt install seabios qemu-system-data` in WSL. |
| "If write lock: Power Off the AQEMU Ubuntu companion VM, then retry." | The companion VM is running from the list. Power it Off. |
| "The iPhone USB link needs Inferno's Linux QEMU (qemu-system-x86_64-inferno)…" | You powered on the companion from the list without the companion emulator in WSL. Use **Start companion in WSL** instead. |
| "Enter the companion Linux SSH username and password…" | Fill in the SSH user and password. |
| "Cannot SSH to companion after waiting." | The companion didn't boot or SSH isn't installed. Check `/tmp/aqemu-inferno-companion.log` in WSL. |
| "Companion needs passwordless sudo for bob…" | Give your companion user passwordless `sudo`. |
| "IPSW not on 9p mount at: /mnt/aqemu_ipsw/aqemu-restore-current.ipsw" | Stop companion, then Start companion with the IPSW selected. |
| "idevicerestore not found in companion PATH." / "FATAL: companion idevicerestore is missing the … patch." | Install the patched `idevicerestore` inside the companion ([section 5](#5-create-the-restore-companion-vm)). |
| "Still no Apple USB after waiting (lsusb had no 05ac)." | The order was wrong or the bridge doesn't match. Start companion → wait for SSH → Restore → Power On iOS when told. Check both VMs use `127.0.0.1:8030`. |
| "Another restore/diagnose command is still running." | Wait for it to finish. |
| "NORData then read error -256: USB bridge dropped; wipe disks and retry." | Wipe Inferno disks and run the restore again. Also check SEP firmware is the packed `.new.img4`. |
| "GPT 78/75: Power Off, Power On (APFS Data seed), Restore once." | Follow it exactly. |
| "Power Off the iOS guest first." | For Wipe and filesystem patches. |
| "Could not delete %1 (is iOS still Powered On?)" | Power it Off and retry. |
| "Failed to start wsl.exe. Configure File → WSL Launch first." | Set up WSL ([section 4](#4-one-time-setup)). |
| Filesystem patch failure: "Ensure the iOS VM is Powered Off and WSL has KVM (nested virtualization)." | Turn on nested virtualization for WSL. |
| "Guest display still waiting" | Not an error. Leave it running and check `qemu-boot.log`. |
| "qemu-system-applesoc crashed (SIGSEGV / exit 139) after starting." | Try Power On again. Start the companion first, or clear USB remote for a plain boot test. |
| "The Inferno loader exited 0 before the kernel printed anything." | Read the line under it. "Nothing is listening on…" means start the companion first. |
| "Apple SoC / iOS QEMU stopped (exit %1)." | Usual causes: companion bridge not up, a guest panic, or a restore waiting on `idevicerestore`. Check `qemu-boot.log`. |
| "Device Tools SSH helper is currently wired for Windows + WSL." | Linux isn't supported for Device Tools. |

## 18. FAQ

**Is this an iPhone simulator?**
No. It's an experimental emulator of one iPhone board, run as research tooling. It may not boot.

**Does AQEMU come with iOS, or tell me where to get it?**
No. AQEMU doesn't include any Apple firmware or software and can't say where to get it. You source it yourself, legally, using your own judgement.

**Which iOS versions work?**
We don't publish a list. The tools target `n104ap` (iPhone 11, `t8030`) only, and every file must come from the same build as your IPSW.

**Why do I need a second Ubuntu VM?**
The emulated phone's USB only exists over the bridge, and the restore tools run on Linux. The companion is where the phone appears as a USB device.

**Can I make it faster?**
Not on an x64 host: it's always TCG. On ARM64 you can try KVM (Linux natively, or Windows through WSL). That's unverified.

**Can I use it on Linux?**
The iOS VM starts on Linux, but the guided restore, diagnose, Device Tools and automatic filesystem patching are Windows + WSL only. See [section 3](#3-what-aqemu-automates-on-each-host).

**Why is the screen black?**
Usually because filesystem patches weren't applied after the restore, or because the display isn't up yet. See [section 14](#14-the-boot-process-what-to-expect).

**Can I make the NAND bigger later?**
No. Wipe Inferno disks, set the new size, and restore again.

**Can I install apps?**
These docs don't cover or support that.

**Does it delete my VM if I wipe?**
No. Wipe only removes the NAND images in the `_inferno` folder.

**What do I check first when it fails?**
The Output log in the dialog you used, then `qemu-boot.log` in the `_inferno` folder, then `/tmp/aqemu-inferno-companion.log` in WSL.

---

Related: [Accelerators](features/accelerators.md) · [New VM wizard](features/new-vm-wizard.md) · [Preferences](../reference/settings/preferences.md) · [Troubleshooting](features/troubleshooting.md) · [Documentation Hub](../README.md)
