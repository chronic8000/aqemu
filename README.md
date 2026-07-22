# AQEMU

<p align="center">
  <img src="resources/icons/aqemu_logo.png" alt="AQEMU logo" width="280"/>
</p>

<p align="center">
  <b>A modern QEMU frontend for Windows and Linux</b><br/>
  Maintained by <a href="https://github.com/chronic8000">Chronic Engineering</a>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-GPL--2.0-blue.svg" alt="License: GPL-2.0"/></a>
  <a href="https://github.com/chronic8000/aqemu/releases"><img src="https://img.shields.io/badge/releases-GitHub-black.svg" alt="Releases"/></a>
  <a href="PRIVACY.md"><img src="https://img.shields.io/badge/privacy-policy-lightgrey.svg" alt="Privacy"/></a>
</p>

---

## This project was picked up — it is active again

AQEMU began as Andrey Rijov’s (RDron) Qt frontend for QEMU, later carried in the community by Tobias Gläßer (Qt5 / 0.9.x). Development slowed; the historical trees people still find online include:

- Earlier community GitHub: [tobimensch/aqemu](https://github.com/tobimensch/aqemu) *(upstream history — not this project’s homepage)*
- Older SourceForge project pages *(archive / third-party — not ours)*

**Chronic Engineering** has forked and revived the code here:

**https://github.com/chronic8000/aqemu**

This repository is the **current** source of truth: Windows + Linux hosts, embedded SPICE sessions, Windows 11 ARM workflows, classic PowerPC Mac guests, experimental Intel macOS (user-supplied OpenCore), optional WSL/KVM launch, and packaging aimed at GitHub Releases and the **Microsoft Store**.

We keep credit for the original authors. We do **not** claim their old websites, donation campaigns, or SourceForge listings.

| | |
|--|--|
| **Current home** | https://github.com/chronic8000/aqemu |
| **Issues** | https://github.com/chronic8000/aqemu/issues |
| **License** | [GNU GPLv2](LICENSE) |
| **Privacy** | [PRIVACY.md](PRIVACY.md) |
| **Authors** | [AUTHORS](AUTHORS) |

---

## What AQEMU does

AQEMU is a **Qt5 GUI** that configures and starts **QEMU** virtual machines. On supported hosts it can:

- Discover installed `qemu-system-*` binaries and machine/CPU/device lists  
- Guide **Windows 11 ARM** setups (VirtIO / UEFI-oriented defaults)  
- Offer **classic Mac OS (PowerPC)** and **experimental Intel macOS** guest profiles  
- Run guests in an **embedded session** (`-display none` + localhost SPICE/QMP)  
- Optionally launch Linux QEMU via **WSL/KVM** on Windows when `/dev/kvm` works  
- Force **pure TCG** for Win9x / pre-ME guests that break under WHPX  

**Host support:** Windows and Linux (including Raspberry Pi 5).  
**Not supported as a host:** macOS (building/running AQEMU *on* a Mac is out of scope).  
macOS / classic Mac as **guests** are supported only when **you** supply the media.

---

## License (Store / paid builds)

Licensed under **GNU GPL version 2**. You may charge for pre-built installers (Store, itch, Releases). Anyone who gets a binary is entitled to the matching source — **this repo**.

Full text: [`LICENSE`](LICENSE) (also [`COPYING`](COPYING)). In-app: **Help → About**.

### We do not ship

- Windows install ISOs or product keys  
- Apple OS / recovery / BaseSystem images  
- OpenCore images  
- An Apple SMC **OSK** (never pre-filled)

Point the UI at files you obtained lawfully.

### Trademarks

Microsoft, Windows, Apple, macOS, and related names belong to their owners. AQEMU is independent and not endorsed by Microsoft, Apple, or the QEMU project.

---

## Install

### GitHub Releases

https://github.com/chronic8000/aqemu/releases

**Windows:** unzip the portable build and run `aqemu.exe`. Install [QEMU for Windows](https://www.qemu.org/download/#windows) (or use a bundled build) and set paths in First Start / Settings.

**Raspberry Pi / Debian:** install the `.deb` from Releases, or build below.

**Microsoft Store:** when published, the listing will use this repo for source and [PRIVACY.md](PRIVACY.md) for the privacy URL.

---

## Build (short)

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

Same as Linux, plus:

```bash
cmake -G Ninja -DPI5_OPTIMIZATIONS=ON -DAQEMU_WITH_SPICE_GTK=ON ..
```

Prefer Wayland: `QT_QPA_PLATFORM=wayland aqemu`

### Windows (Win64)

Toolchain: **WinLibs UCRT MinGW** + **Qt 5.15**. SPICE libs from **MSYS2 ucrt64** via `PKG_CONFIG_PATH` only (do not mix CRTs).

```powershell
$env:PKG_CONFIG_PATH = "C:\msys64\ucrt64\lib\pkgconfig"
cd C:\path\to\aqemu
mkdir build_win -Force; cd build_win
cmake -G Ninja `
  -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/mingw81_64" `
  -DAQEMU_WITH_SPICE_GTK=ON `
  ..
ninja
.\aqemu.exe
```

Close `aqemu.exe` before rebuilding.

### Useful CMake options

| Option | Default | Meaning |
|--------|---------|---------|
| `AQEMU_WITH_SPICE_GTK` | OFF | spice-client-glib embedded viewer |
| `WITHOUT_EMBEDDED_DISPLAY` | OFF | Disable LibVNC viewer |
| `AQEMU_BUNDLE_QEMU` | OFF | Copy `qemu-system-*` from `AQEMU_QEMU_PREFIX` |
| `PI5_OPTIMIZATIONS` | OFF | Cortex-A76 + 64KB alignment |

Optional vendored QEMU: see `third_party/README.md` and `scripts/build_qemu_*.sh`.

---

## Credits

- **Current maintainers:** Chronic Engineering  
- **Prior community maintainer:** Tobias Gläßer (0.9.0+)  
- **Original author:** Andrey Rijov (RDron)  

Contributor names remain in **Help → About → Thanks To**.

---

## Troubleshooting (quick)

- **Win11 ARM on x86 Windows hosts** runs under **TCG only** — expect slow guests; use 4+ vCPUs.  
- **Embedded session** uses `-display none`; SPICE/LibVNC show the guest inside AQEMU.  
- **Win9x mouse:** General → Mouse → USB Tablet + UHCI + USB 1.1.  
- **Stuck QEMU on Windows:** Task Manager → end stray `qemu-system-*`, then restart AQEMU.
