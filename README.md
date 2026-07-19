# AQEMU (Raspberry Pi 5, Wayland & Windows)

AQEMU is a Qt5 GUI frontend for QEMU. This fork targets **Raspberry Pi 5** (Raspbian Trixie / Debian 13, Wayland, 16KB pages) and also provides a **Windows** build for managing multi-arch VMs (including Windows 11 ARM under TCG).

---

## Key Features

- **Pi 5 tuning**: `-mcpu=cortex-a76`, 64KB page load alignment, Wayland Qt platform
- **Windows 11 ARM wizard**: guided profile aligned with [win11-pi5-kiosk](https://github.com/chronic8000/win11-pi5-kiosk) (`virt`, VirtIO disk/net/GPU, UEFI, USB audio)
- **Full QEMU arch coverage**: discovers `qemu-system-*`, live-probes machines/CPUs/devices; Generate VM for any target
- **TCG-friendly aarch64 defaults** (Windows host): `gic-version=max`, `tcg,thread=multi,tb-size=1024`, `max,pauth-impdef=on`, VirtIO bootindex
- **Embedded session**: QEMU runs **headless** (`-display none`) with localhost **SPICE** + **QMP**; AQEMU owns the window (SPICE client via spice-gtk when built, otherwise VNC)
- **Bundled QEMU**: optional git submodule + build scripts to ship `qemu-system-*` next to AQEMU
- **Automated QEMU install on Linux** via `pkexec apt-get install -y qemu-system qemu-utils`

---

## Install from GitHub Releases

Download assets from the [Releases](https://github.com/chronic8000/aqemu/releases) page.

### Raspberry Pi (`.deb`)
```bash
sudo apt update
sudo apt install ./aqemu_0.9.8_arm64.deb
aqemu
```

### Windows (portable zip)
1. Install [QEMU for Windows](https://www.qemu.org/download/#windows) (include `qemu-system-aarch64` and EDK2 firmware), **or** build the bundled submodule (below).
2. Unzip `aqemu-0.9.8-win64.zip` and run `aqemu.exe`.

---

## Bundled QEMU (submodule)

Upstream QEMU is vendored as `third_party/qemu` (pinned to a release tag, currently **v11.0.2**).

```bash
git submodule update --init --depth 1 third_party/qemu
# bump later: cd third_party/qemu && git fetch --tags && git checkout v11.0.2
```

**Linux / Pi**
```bash
# needs spice-server for SPICE: sudo apt install libspice-server-dev ninja-build
scripts/build_qemu_linux.sh
cmake -DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=$PWD/third_party/qemu-install \
      -DAQEMU_WITH_SPICE_GTK=ON ..   # optional guest viewer
```

**Windows** — use MSYS2 MinGW64 (see `scripts/build_qemu_windows.ps1` / `.sh`), then:
```bash
cmake -DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=…/third_party/qemu-install …
```

Settings (defaults for new installs): `Embedded_Session=yes`, `Embedded_Display_Backend=spice`.

---

## Build on Raspberry Pi 5

```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev libqt5widgets5 qtwayland5 libvncserver-dev extra-cmake-modules

mkdir -p build && cd build
cmake -DPI5_OPTIMIZATIONS=ON ..
make -j$(nproc)
cpack   # produces aqemu_0.9.8_arm64.deb
```

---

## Troubleshooting

### Wayland / embedded VNC
- Native: `QT_QPA_PLATFORM=wayland aqemu`
- Safer grab: `QT_QPA_PLATFORM=xcb aqemu`

### Polkit missing
Install QEMU manually: `sudo apt install -y qemu-system qemu-utils`

### Windows 11 ARM performance
On an **x86_64 Windows host**, aarch64 guests use **TCG only** (no KVM/WHPX for ARM). Expect much slower than Pi + KVM. Use 4+ vCPUs and the TCG flags above.

### Embedded session / no guest picture
AQEMU starts QEMU with `-display none`. Without LibVNC (`WITHOUT_EMBEDDED_DISPLAY`) or spice-gtk, the session toolbar still works (CD/FD via QMP); use **Exit View** to return to the VM list while the guest keeps running.