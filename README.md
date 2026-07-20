# AQEMU (Raspberry Pi 5, Wayland & Windows)

AQEMU is a Qt5 GUI frontend for QEMU. This fork targets **Raspberry Pi 5** (Raspbian Trixie / Debian 13, Wayland) and **Windows** hosts for multi-arch VMs (including Windows 11 ARM under TCG).

---

## Key Features

- **Pi 5 tuning**: `-mcpu=cortex-a76`, 64KB page load alignment, Wayland Qt platform
- **Windows 11 ARM wizard**: guided profile aligned with [win11-pi5-kiosk](https://github.com/chronic8000/win11-pi5-kiosk)
- **Full QEMU arch coverage**: discovers `qemu-system-*`, probes machines/CPUs/devices
- **TCG-friendly aarch64 defaults** on Windows hosts
- **Embedded session**: QEMU runs headless (`-display none`) with localhost **SPICE** + **QMP**; AQEMU owns the window
- **Mouse / pointer settings** per VM (PS/2, USB tablet/mouse, VirtIO, VMware mouse, USB controller/version, SPICE agent-mouse)
- **Bundled QEMU**: optional git submodule + build scripts
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
2. Unzip `aqemu-*-win64.zip` and run `aqemu.exe`.

---

## CMake options

| Option | Default | Meaning |
|--------|---------|---------|
| `WITHOUT_EMBEDDED_DISPLAY` | OFF | Disable LibVNC embedded viewer |
| `AQEMU_WITH_SPICE_GTK` | OFF | Enable spice-client-glib embedded SPICE viewer (recommended) |
| `AQEMU_BUNDLE_QEMU` | OFF | Copy `qemu-system-*` from `AQEMU_QEMU_PREFIX` next to `aqemu` |
| `AQEMU_QEMU_PREFIX` | `third_party/qemu-install` | Path to a QEMU install tree |
| `PI5_OPTIMIZATIONS` | OFF | Cortex-A76 + 64KB page alignment (Pi 5) |
| `DEBUG` | OFF | Debug symbols / verbose build |

Runtime defaults (new installs): `Embedded_Session=yes`, `Embedded_Display_Backend=spice`.

---

## Build — Linux (Debian / Ubuntu / desktop)

### Dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  qtbase5-dev libqt5widgets5 \
  libvncserver-dev extra-cmake-modules \
  libspice-client-glib-2.0-dev \
  qemu-system qemu-utils
```

Optional (bundled QEMU from submodule):

```bash
sudo apt install -y libspice-server-dev ninja-build
```

### Configure & build

```bash
git clone --recursive https://github.com/chronic8000/aqemu.git
cd aqemu
mkdir -p build && cd build

cmake -G Ninja \
  -DAQEMU_WITH_SPICE_GTK=ON \
  ..

ninja
./aqemu
```

Without SPICE viewer (VNC fallback only):

```bash
cmake -DAQEMU_WITH_SPICE_GTK=OFF ..
```

Without any embedded display:

```bash
cmake -DWITHOUT_EMBEDDED_DISPLAY=ON ..
```

---

## Build — Raspberry Pi 5

Same stack as Debian, plus Pi 5 flags and Wayland Qt:

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake ninja-build pkg-config \
  qtbase5-dev libqt5widgets5 qtwayland5 \
  libvncserver-dev extra-cmake-modules \
  libspice-client-glib-2.0-dev \
  qemu-system qemu-utils

mkdir -p build && cd build
cmake -G Ninja \
  -DPI5_OPTIMIZATIONS=ON \
  -DAQEMU_WITH_SPICE_GTK=ON \
  ..
ninja
cpack   # produces aqemu_*_arm64.deb
```

Wayland:

```bash
QT_QPA_PLATFORM=wayland aqemu
# safer mouse grab if needed:
QT_QPA_PLATFORM=xcb aqemu
```

---

## Build — Windows (Win64)

The supported Windows toolchain is **WinLibs UCRT MinGW** + **Qt 5.15** for AQEMU, with **MSYS2 ucrt64** packages only for SPICE (linked by absolute path so CRTs do not mix).

### 1. Install toolchain & Qt

1. **WinLibs** MinGW-w64 UCRT (POSIX threads) — e.g. via winget `BrechtSanders.WinLibs.POSIX.UCRT`, or from [winlibs.com](https://winlibs.com/).
2. **Qt 5.15** with the matching MinGW kit (example path used below: `C:\Qt\5.15.2\mingw81_64`).
3. **CMake** and **Ninja** on `PATH`.
4. Put WinLibs `mingw64\bin` on `PATH` ahead of other MinGW installs.

### 2. Optional: LibVNC (embedded VNC fallback)

If `third_party/libvnc-install` exists, CMake uses it automatically. Otherwise install a system LibVNCServer, or disable VNC:

```text
-DWITHOUT_EMBEDDED_DISPLAY=ON
```

Vendored layout (when present):

- `third_party/libvnc-install/include/rfb/rfbclient.h`
- `third_party/libvnc-install/lib/libvncclient.a`
- `third_party/zlib-install/lib/libzlibstatic.a` (if needed)

### 3. SPICE client (recommended)

In an **MSYS2 UCRT64** shell:

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-spice-gtk \
  mingw-w64-ucrt-x86_64-pkgconf
```

AQEMU must see those `.pc` files without linking against MSYS2’s full lib path (wrong CRT vs WinLibs):

```powershell
$env:PKG_CONFIG_PATH = "C:\msys64\ucrt64\lib\pkgconfig"
# ensure pkgconf.exe from ucrt64\bin is findable, or set:
# $env:PKG_CONFIG = "C:\msys64\ucrt64\bin\pkgconf.exe"
```

On a successful build, `CopySpiceRuntime.ps1` copies SPICE DLLs (and `libspice-server-1.dll` when present) next to `aqemu.exe`.

### 4. Configure & build AQEMU

From **PowerShell** (example paths — adjust to your WinLibs + Qt install):

```powershell
cd C:\path\to\aqemu
mkdir build_win -Force
cd build_win

cmake -G Ninja `
  -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/mingw81_64" `
  -DAQEMU_WITH_SPICE_GTK=ON `
  -DWITHOUT_EMBEDDED_DISPLAY=OFF `
  ..

ninja
.\aqemu.exe
```

Close `aqemu.exe` before rebuilding (Windows locks the EXE while it runs).

### 5. QEMU for Windows guests

Either:

- Install [QEMU for Windows](https://www.qemu.org/download/#windows) and point AQEMU at it in First Start / Settings, **or**
- Build/bundle the submodule (next section).

---

## Bundled QEMU (submodule)

Upstream QEMU is vendored as `third_party/qemu` (pinned release; see `third_party/README.md`, currently **v11.0.2**).

```bash
git submodule update --init --depth 1 third_party/qemu
```

### Linux / Pi

```bash
# apt: libspice-server-dev ninja-build (recommended)
scripts/build_qemu_linux.sh
cmake -DAQEMU_BUNDLE_QEMU=ON \
      -DAQEMU_QEMU_PREFIX=$PWD/third_party/qemu-install \
      -DAQEMU_WITH_SPICE_GTK=ON \
      ..
```

### Windows

Prefer a **pure MSYS2 MinGW64** QEMU build (same CRT as QEMU’s glib/spice deps):

```bash
# Inside MSYS2 MinGW64, after packages + optional scripts/fix_msys2_gcc_admin.ps1
scripts/build_qemu_windows_msys.sh
```

Also available: `scripts/build_qemu_windows.ps1`, `scripts/build_qemu_windows.sh`, `scripts/build_qemu_windows_winlibs.sh`.

Then configure AQEMU with:

```text
-DAQEMU_BUNDLE_QEMU=ON
-DAQEMU_QEMU_PREFIX=<repo>/third_party/qemu-install
```

**Do not** mix WinLibs gcc with MSYS2 headers when compiling QEMU. Details: `third_party/README.md`.

Install trees `third_party/qemu-build*` / `qemu-install` are gitignored.

---

## Troubleshooting

### Wayland / embedded display (Linux / Pi)

```bash
QT_QPA_PLATFORM=wayland aqemu
QT_QPA_PLATFORM=xcb aqemu
```

### Polkit / QEMU missing on Linux

```bash
sudo apt install -y qemu-system qemu-utils
```

### Windows 11 ARM performance

On an **x86_64 Windows host**, aarch64 guests use **TCG only** (no KVM/WHPX for ARM). Expect much slower than Pi + KVM. Use 4+ vCPUs.

### Embedded session / no guest picture

AQEMU starts QEMU with `-display none`. Without LibVNC or spice-client-glib, the session toolbar still works (CD/FD via QMP); use **Exit View** to return to the VM list while the guest keeps running.

### Win98 / stuck mouse cursor

On **General → Mouse**, use **USB Tablet** with **UHCI** and **USB 1.1**. Absolute tablet tracking is required for SPICE/VNC-style displays.

### Windows SPICE link / CRT errors

Build AQEMU with **WinLibs**, take SPICE only from **MSYS2 ucrt64** via `PKG_CONFIG_PATH`, and link by absolute `.dll.a` paths (CMake already does this). Do not add `C:\msys64\ucrt64\lib` as a global link directory.

### QEMU won’t die on Stop

AQEMU force-terminates leftover `qemu-system-*` after a failed monitor quit and clears orphans that still hold the disk before Start. If a guest disk stays locked, end stray `qemu-system-*` in Task Manager once, then restart AQEMU.
