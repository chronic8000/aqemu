# AQEMU

<p align="center">
  <img src="resources/icons/aqemu_logo.png" alt="AQEMU logo" width="260"/>
</p>

<p align="center">
  <b>The QEMU virtual machine manager built for maximum power and modern ease.</b><br/>
  ~30k lines beyond the legacy community tree — embedded SPICE, bundled QEMU <b>11.0.2</b>, Win11 ARM on x86, Win9x done right, classic Mac PPC + Intel macOS.<br/>
  <b>Now Available on the Microsoft Store!</b><br/>
  Maintained by <a href="https://github.com/chronic8000">Chronic Engineering</a>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--2.0-blue.svg" alt="License: GPL-2.0"/></a>
  <a href="https://apps.microsoft.com/detail/9p0hgkhq9w19"><img src="https://img.shields.io/badge/Microsoft%20Store-AQEMU%20VM%20Manager-0078D4?logo=microsoftstore" alt="Microsoft Store"/></a>
  <a href="https://neonsovereign.store/aqemu.html"><img src="https://img.shields.io/badge/website-neonsovereign.store-brightgreen.svg" alt="Official Website"/></a>
  <a href="https://github.com/chronic8000/aqemu"><img src="https://img.shields.io/badge/QEMU-11.0.2%20bundle-orange.svg" alt="QEMU 11.0.2"/></a>
  <a href="https://github.com/chronic8000/aqemu"><img src="https://img.shields.io/badge/host-Windows%20%7C%20Linux%20%7C%20Pi%205-success.svg" alt="Hosts"/></a>
  <a href="PRIVACY.md"><img src="https://img.shields.io/badge/privacy-policy-lightgrey.svg" alt="Privacy"/></a>
</p>

<p align="center">
  <img src="screenshots/win98-setup.png" alt="Windows 98 running in AQEMU" width="920"/>
</p>

<p align="center">
  <i>AQEMU VM Manager on Windows — high-fidelity emulation with built-in QEMU 11.0.2 & embedded SPICE display.</i>
</p>

---

## 🛒 Get AQEMU VM Manager

AQEMU is open-source under the **GNU General Public License v2 (GPL-2.0)**. You can freely compile the source code yourself or build custom packages anytime! 

If you want to support ongoing development, maintenance, and future feature releases while enjoying seamless automatic updates directly on Windows, purchase **AQEMU VM Manager** on the Microsoft Store:

<p align="center">
  <a href="ms-windows-store://pdp/?productid=9P0HGKHQ9W19">
    <b>👉 Open in Microsoft Store app (`ms-windows-store://pdp/?productid=9P0HGKHQ9W19`)</b>
  </a><br/>
  <a href="https://apps.microsoft.com/detail/9p0hgkhq9w19">
    <b>🌐 View on Web (`https://apps.microsoft.com/detail/9p0hgkhq9w19`)</b>
  </a>
</p>

---

## Help us test & refine

| | |
|--|--|
| **Microsoft Store** | [AQEMU VM Manager on Microsoft Store](https://apps.microsoft.com/detail/9p0hgkhq9w19) (Official Windows installation with automatic updates) |
| **Website** | [AQEMU Official Website](https://neonsovereign.store/aqemu.html) |
| **File bugs** | [GitHub Issues](https://github.com/chronic8000/aqemu/issues) — host OS, guest OS, and exact log outputs help us triage quickly. Historical [tobimensch/aqemu issues](https://github.com/tobimensch/aqemu/issues) are archived context only; triage lives in [`docs/TOBIMENSCH_ISSUE_TRIAGE.md`](docs/TOBIMENSCH_ISSUE_TRIAGE.md) |
| **What we care about** | Install wizards, embedded SPICE, Win9x/XP TCG, Win11 ARM, Intel macOS, Solaris x86, AIX/pseries, migrate/QMP tools |

---

## This project is alive again

AQEMU started with **Andrey Rijov (RDron)**, then the community era under **Tobias Gläßer** (Qt5 / 0.9.x). Development went quiet. People still bump into old trees online:

- Historical community GitHub: [tobimensch/aqemu](https://github.com/tobimensch/aqemu) *(history only — not our homepage)*
- Abandoned / third-party mirrors (including old SourceForge pages) — **not operated by us**

**Chronic Engineering picked it up.** This is the active project repository:

### https://github.com/chronic8000/aqemu

Compared to [tobimensch/aqemu](https://github.com/tobimensch/aqemu): **embedded SPICE sessions**, **bundled QEMU 11.0.2**, **Win11 ARM**, proper **Win9x TCG**, **classic Mac + Intel macOS** (WSL/KVM), Solaris/AIX/OS/2 recipes, QMP/blockdev/migrate UI, Windows packaging + **Microsoft Store**. Roughly **+30,000 lines** of new work.

We keep the original authors’ names. We do **not** inherit their old donation pages, crowdfunding, or SourceForge homepage.

| | |
|--|--|
| **Home** | https://github.com/chronic8000/aqemu |
| **Store** | https://apps.microsoft.com/detail/9p0hgkhq9w19 |
| **Website** | https://neonsovereign.store/aqemu.html |
| **Issues** | https://github.com/chronic8000/aqemu/issues |
| **Discussions** | https://github.com/chronic8000/aqemu/discussions |
| **License** | [GNU GPLv2](LICENSE) |
| **Privacy** | [PRIVACY.md](PRIVACY.md) |
| **Authors** | [AUTHORS](AUTHORS) |

---

## What’s new vs the old AQEMU (tobimensch / ~0.9.x)

The last widely known community tree — [tobimensch/aqemu](https://github.com/tobimensch/aqemu) — went quiet years ago (Qt5 port, VNC-era display, you brought your own QEMU). **This fork is not a polish pass.** From that baseline we are **~+30,000 lines / −2,000 lines across ~120 files** of real product work. Here is why you should download **1.0.0**:

### 1. Built-in QEMU 11.0.2 (Windows portable)
Old AQEMU assumed a system QEMU install. Our Release zip / Store build **ships QEMU next to `aqemu.exe`** — `qemu-system-x86_64`, `i386`, `aarch64`, `qemu-img`, firmware — with a **File → Configure → Emulator** toggle for **built-in vs custom folder**. Unzip and run.

### 2. Embedded SPICE sessions (not a lost SDL window)
QEMU runs headless; you see the guest **inside AQEMU** via modern **spice-client-glib**, with a real **session toolbar** (media, USB hotplug, net, power) and **QMP** control. Old AQEMU leaned on LibVNC / separate display chrome. This is how you actually *use* a VM in 2026.

### 3. Guests people fight with today — with recipes that work
| Guest | What we added |
|--|--|
| **Windows 11 ARM** | Wizard + lifecycle on **x64 Windows hosts via TCG** (and KVM on Pi 5 / aarch64) |
| **Windows 95 / 98 / ME** | **Force pure TCG** — WHPX hangs classic 9x; we stop that footgun |
| **Intel macOS** | OpenCore + OVMF + OSK UI, host-matching resolution, optional **WSL/KVM** on Windows |
| **Classic Mac OS (PPC)** | `mac99` / PowerPC profiles, no PC floppy nonsense |
| **Solaris 11.4 / AIX / OS/2 / ReactOS** | Wizard defaults matching real QEMU flags (AIX = `ppc64`/`pseries`, TCG on Windows) |

### 4. Deeper QEMU — without living in a shell
Advanced Options grew serious CLI parity: **NUMA, TPM, watchdog, secrets, SMBIOS, fw_cfg, audiodev, icount/sandbox**, richer chardev/blockdev hooks, a **blockdev graph** editor, and **migrate progress/cancel** over QMP. Old AQEMU could launch VMs; this one can express much more of what modern QEMU can do.

### 5. Windows + Pi hosts that match how people work
- **WHPX** when the guest arch allows it; **never** shoved onto PPC/ARM guests that cannot use it  
- **WSL/KVM** launch path for heavy guests (Intel macOS) when `/dev/kvm` is there  
- **Raspberry Pi 5** build flags and aarch64 friendliness  
- Portable packaging + **Microsoft Store** track (MSIX) for updates later  

### 6. Still AQEMU — just alive
Same GPLv2 soul, same authors credited, same “GUI over QEMU” mission. New home, new maintainers, **Issues + Discussions open**, Release zips you can actually run.

Full chronology: [`CHANGELOG`](CHANGELOG). Historical tree (not our homepage): [tobimensch/aqemu](https://github.com/tobimensch/aqemu).

---

## The pitch: everything QEMU does — with a GUI

QEMU already speaks almost every guest architecture and machine type under the sun. AQEMU’s job is to **stop making you hand-write 40-flag command lines** and give you:

- A real **desktop app** (Qt5) on **Windows** and **Linux** (including **Raspberry Pi 5**)
- **Embedded SPICE + QMP** sessions — guest display and controls in one window
- **Built-in or custom QEMU** — portable zip includes **11.0.2**
- **Wizards and profiles** for Win9x → Win11 ARM, classic Mac, Intel macOS, Solaris, AIX, and more
- **Active maintenance** — this is the fork that ships builds and wants your bug reports

If QEMU can boot it, AQEMU aims to **configure and launch it**. Bring your own ISOs, disks, firmware, and keys where the law requires it.

---

## Windows 11 ARM on x86_64 — yes, really

On a normal **Intel/AMD Windows PC**, an aarch64 guest cannot use WHPX for ARM. AQEMU drives **TCG** with sane defaults (multi-vCPU, VirtIO-oriented Win11 ARM wizard, UEFI/AAVMF discovery, install → first-boot → normal lifecycle).

It is **not as fast as a Pi 5 + KVM** or an ARM laptop. It **is usable**: you can install, update, and work through OOBE in the embedded SPICE view (see [Screenshots](#screenshots)). Perfect for testing ARM Windows without buying ARM hardware.

On **Raspberry Pi 5 / Linux aarch64** hosts, the same profile can lean on **KVM** where available — much snappier.

---

## Windows from 1.x through modern

| Era | What AQEMU helps with |
|-----|------------------------|
| **Windows 1.x / 2.x / 3.x** | Period-friendly PC settings; treat them as real vintage targets |
| **Windows 95 / 98 / ME** | **Force pure TCG** (WHPX hangs classic 9x at splash), pentium2-class CPU, PS/2 or USB tablet guidance, cirrus-era video defaults |
| **NT / 2000 / XP / Vista / 7 / 10 / 11** | Modern q35 / virtio paths where appropriate; full device manager |
| **Windows 11 ARM** | Dedicated wizard + lifecycle modes, VirtIO, UEFI, embedded SPICE |

From **Win9x setup screens** to **Windows 11 ARM “Almost there”** — same app, same session chrome.

---

## Unix & other heavy guests

| Guest | Notes |
|-------|--------|
| **Solaris 11.4 x86** | Text installer ISO, `pc` + `usb=off`, e1000, IDE, WHPX/KVM on Windows; install can sit at 99% for a long time while the disk grows |
| **AIX (POWER)** | `qemu-system-ppc64` + `pseries` + POWER8, **TCG only** on x86 hosts (very slow). Virtio-SCSI. VMware VMDKs often fail under SLOF (`E3403 Bad executable`) — prefer a real AIX install ISO |
| **OS/2 / eComStation / ReactOS** | Proven IDE / ACPI-off / Force TCG style defaults in the wizard |
| **Linux / BSD / Haiku / …** | Full arch + machine pickers; VirtIO where it makes sense |

---

## Mac guests — classic and modern (bring your own media)

### Classic Mac OS (PowerPC)

- `qemu-system-ppc` + **mac99** (G3 beige still available in the machine list)
- Tuned RAM defaults (e.g. 256 MB for 7–9, 512 MB for OS X PPC)
- New-World-friendly NIC hints (`sungem` / macio when probed), screamer audio hint, machine-native video
- **You supply** the boot CD/ISO or HDD image — we never ship Apple install media
- Clear warning if `qemu-system-ppc` isn’t installed

### Intel macOS / Darwin (experimental)

- `qemu-system-x86_64`, **q35**, dual-pflash **OVMF**, OpenCore as first disk, Apple SMC **only if you paste your own OSK**
- Optional Recovery/BaseSystem path
- Native **WHPX** on Windows, or **WSL/KVM** when `/dev/kvm` works (`wsl.exe` + Linux QEMU, SPICE still on localhost)
- Host-matching resolution via OpenCore; AMD Metal passthrough UI on bare-metal Linux (see [`docs/intel-macos-gpu.md`](docs/intel-macos-gpu.md))
- **AQEMU does not ship** OpenCore, OVMF bundles as Apple IP, OS images, or a pre-filled OSK

### Out of scope (for now)

Apple Silicon macOS guests on Snapdragon Windows hosts — not this release’s promise. Everything else QEMU can express stays on the table via architecture / machine pickers.

---

## Screenshots

More of the guest zoo — Win11 ARM and classic Windows under the same session UI. (Sonoma teaser is at the top of this README.)

**Windows 11 ARM** running *inside* AQEMU on a Windows host — embedded SPICE session, full toolbar, guest progressing through setup:

![Windows 11 ARM setup in AQEMU](screenshots/win11-arm-setup.png)

**Windows 98** install under the same modern UI — classic guests are first-class, not an afterthought:

![Windows 98 Setup in AQEMU](screenshots/win98-setup.png)

**Session chrome** — boot console / toolbar while QEMU runs headless behind SPICE:

![AQEMU session UI](screenshots/session-ui-boot-console.png)

More captures live in [`screenshots/`](screenshots/).

---

## Built-in QEMU 11.0.2

Optional **git submodule** pins upstream QEMU at **v11.0.2** (`third_party/qemu`). Build scripts for Linux/Pi and Windows produce an install tree you can **bundle next to AQEMU** (`AQEMU_BUNDLE_QEMU=ON`).

One frontend. Current QEMU. Your guests.

Details: [`third_party/README.md`](third_party/README.md).

---

## Feature highlights

- **vs old AQEMU** — ~+30k lines: SPICE-in-app, bundled QEMU, modern guest recipes (see [What’s new](#whats-new-vs-the-old-aqemu-tobimensch--09x))
- **QEMU installation** — **Use built-in QEMU** (portable zip) or **custom folder**
- **Embedded SPICE + QMP** session UI (CD/FD/HDD/USB hotplug/net toolbar while the guest runs)
- **Full arch discovery** — every `qemu-system-*` QEMU exposes
- **Windows 11 ARM** guided install / first boot / normal modes
- **Force pure TCG** for pre-ME Windows that WHPX breaks
- **Classic PPC Mac** + **experimental Intel macOS** (OpenCore / WSL-KVM)
- **Solaris / AIX / OS/2 / ReactOS** wizard defaults that match real QEMU recipes
- **Advanced QEMU options** — NUMA, TPM, migrate, SMBIOS, fw_cfg, blockdev graph, …
- **WSL/KVM launch** path on Windows (probe `/dev/kvm` in Settings)
- **Pi 5** optimizations (`-mcpu=cortex-a76`, 64KB page alignment, Wayland)
- **Microsoft Store–ready** posture: GPLv2 source public, [privacy policy](PRIVACY.md), no proprietary OS media in the box

---

## License & what we never ship

**GNU GPL version 2** — [`LICENSE`](LICENSE). Selling installers (Store, itch, Releases) is allowed; source for those binaries is this repo.

We **never** ship:

- Windows ISOs / product keys  
- Apple OS / recovery / BaseSystem  
- OpenCore images  
- A default Apple **OSK**
- IBM AIX / Oracle Solaris media

You point at files you obtained lawfully.

**Trademarks:** Microsoft, Windows, Apple, macOS, IBM, Oracle, etc. belong to their owners. AQEMU is independent and not endorsed by Microsoft, Apple, IBM, Oracle, or the QEMU project.

---

## Install

### Microsoft Store (Windows)

Get the official **AQEMU VM Manager** package on the Microsoft Store:

<p align="center">
  <a href="ms-windows-store://pdp/?productid=9P0HGKHQ9W19">
    <b>👉 Open in Microsoft Store app (`ms-windows-store://pdp/?productid=9P0HGKHQ9W19`)</b>
  </a><br/>
  <a href="https://apps.microsoft.com/detail/9p0hgkhq9w19">
    <b>🌐 View on Web (`https://apps.microsoft.com/detail/9p0hgkhq9w19`)</b>
  </a>
</p>

The Microsoft Store version includes:
- **Bundled QEMU 11.0.2** binaries (no separate QEMU setup required)
- **Automatic background updates** via the Microsoft Store
- Dedicated Windows app installation & single-click launcher

### Build from Source (Linux / Pi 5 / Windows)

AQEMU is 100% open-source software under **GPLv2**. You can clone and build the application directly from source on Linux, Raspberry Pi 5, or Windows (see [Build instructions](#build) below).

---

## Build

### Linux

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
  qtbase5-dev libqt5widgets5 libvncserver-dev extra-cmake-modules \
  libspice-client-glib-2.0-dev qemu-system qemu-utils

git clone --recursive https://github.com/chronic8000/aqemu.git
cd aqemu && mkdir build && cd build
cmake -G Ninja -DAQEMU_WITH_SPICE_GTK=ON ..
ninja && ./aqemu
```

### Raspberry Pi 5

```bash
cmake -G Ninja -DPI5_OPTIMIZATIONS=ON -DAQEMU_WITH_SPICE_GTK=ON ..
# QT_QPA_PLATFORM=wayland aqemu
```

### Windows

**WinLibs UCRT MinGW** + **Qt 5.15**; SPICE from **MSYS2 ucrt64** via `PKG_CONFIG_PATH` only.

```powershell
$env:PKG_CONFIG_PATH = "C:\msys64\ucrt64\lib\pkgconfig"
mkdir build_win -Force; cd build_win
cmake -G Ninja `
  -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/mingw81_64" `
  -DAQEMU_WITH_SPICE_GTK=ON `
  ..
ninja
.\aqemu.exe
```

### Bundle QEMU 11.0.2

```bash
git submodule update --init --depth 1 third_party/qemu
# Linux: scripts/build_qemu_linux.sh
# Windows MSYS2: scripts/build_qemu_windows_msys.sh
cmake -DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=$PWD/third_party/qemu-install ...
```

| CMake option | Meaning |
|--------------|---------|
| `AQEMU_WITH_SPICE_GTK` | Embedded spice-client-glib viewer |
| `WITHOUT_EMBEDDED_DISPLAY` | Disable LibVNC fallback |
| `AQEMU_BUNDLE_QEMU` | Copy `qemu-system-*` beside AQEMU |
| `PI5_OPTIMIZATIONS` | Cortex-A76 + 64KB alignment |

---

## Credits

- **Current maintainers:** Chronic Engineering  
- **Prior community maintainer:** Tobias Gläßer (0.9.x)  
- **Original author:** Andrey Rijov (RDron)  

Full contributor list: **Help → About → Thanks To**.

---

## Troubleshooting

- **Win11 ARM on x86 Windows** = TCG only → give it RAM + 4 vCPUs and patience; it’s usable, not native-ARM fast  
- **Win95/98 splash freeze** → enable **Force pure TCG**  
- **Classic Mac won’t start** → install `qemu-system-ppc` and attach your own ISO/HDD  
- **Intel macOS** → your OpenCore + OVMF + OSK; empty OSK refuses to start (by design)  
- **Solaris 11.4 “stuck”** → often still installing; watch the disk image grow; use text ISO + WHPX/KVM  
- **AIX on Windows** → PPC is TCG-only; dead keyboard at OF prompt usually means console was forced to serial VTY while you typed in the VGA window  
- **AIX `E3403 Bad executable`** → disk is not a QEMU/SLOF-bootable AIX image (common with VMware VMDKs)  
- **Guest disk locked on Windows** → end stray `qemu-system-*` in Task Manager, restart AQEMU  
- **Clipboard host↔guest** → real sync needs SPICE + `spice-vdagent` *in the guest*; AIX/SLOF and many vintage guests will not do that  

Questions and patches: [GitHub Issues](https://github.com/chronic8000/aqemu/issues) / PRs welcome.
