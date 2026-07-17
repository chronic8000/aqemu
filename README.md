# AQEMU (Optimized for Raspberry Pi 5 & Wayland)

AQEMU is a graphical user interface (GUI) frontend for the QEMU emulator, written in C++ using Qt5. It allows users to easily manage, configure, and execute virtual machines.

This fork is specifically modernized and optimized to target the **Raspberry Pi 5** running **Raspbian OS Trixie (Debian 13)** with native **Wayland** display compositors and **16KB page size** kernel support.

---

## 🚀 Key Features & Pi 5 Optimizations

- **Cortex-A76 Hardware Tuning**: The compiler leverages target-specific flags (`-mcpu=cortex-a76 -mtune=cortex-a76 -O3`) to optimize instructions for the Broadcom BCM2712 processor, resulting in improved performance.
- **16KB & 64KB Page Size Support**: Binary segment loading is aligned to 64KB boundaries (`-Wl,-z,max-page-size=65536`), preventing load crashes on modern 16KB page kernels (standard on newer Raspberry Pi 5 operating systems).
- **Native Wayland Execution**: Seamless desktop integration using Qt5's Wayland platform backend.
- **Automated QEMU Installer**: If QEMU isn't found during setup, AQEMU will prompt to install it and execute `pkexec apt-get install -y qemu-system qemu-utils` to securely authorize package installation.
- **Auto-locating Resources**: Scans directory paths relative to the running binary (e.g. `../resources/`), preventing data folder warnings when running directly from the build directory.
- **Window Geometry Persistence**: Resolves a long-standing TODO item by saving and restoring the size and coordinates of the **Emulator Control Window** via `aqemu.cfg`.

---

## 📦 How to Install (.deb Release)

You can download and install the pre-compiled `.deb` package directly from our **GitHub Releases** page:

1. **Install the package**:
   ```bash
   sudo apt update
   sudo apt install ./aqemu_0.9.6_arm64.deb
   ```
2. **Launch AQEMU**:
   Simply run:
   ```bash
   aqemu
   ```

---

## 🛠️ Building from Source on Raspberry Pi 5

If you prefer to compile manually, follow these steps:

### 1. Install Dependencies
```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev libqt5widgets5 qtwayland5 libvncserver-dev extra-cmake-modules
```

### 2. Configure & Build (CMake)
```bash
mkdir -p build && cd build
cmake -DPI5_OPTIMIZATIONS=ON ..
make -j$(nproc)
```

### 3. Build a Debian Package (Optional)
To build a custom `.deb` package of your compiled code using CPack:
```bash
cd build
cpack
```

---

## ⚠️ Potential Issues & Troubleshooting

### Wayland Rendering Anomalies
AQEMU features an embedded VNC viewer (`libvncclient`) that draws emulator outputs inside the application. If you experience mouse grabbing issues or rendering quirks running natively under Wayland, run AQEMU in XWayland compatibility mode instead:

* **Wayland Native (Default):**
  ```bash
  QT_QPA_PLATFORM=wayland aqemu
  ```
* **XWayland Fallback (Safer VNC grab behavior):**
  ```bash
  QT_QPA_PLATFORM=xcb aqemu
  ```

### Polkit/Authentication Dialog Missing
The automated QEMU installer relies on `pkexec` (Polkit) to request superuser permissions. If you are running on a headless setup or custom environment without a polkit agent running, the installer command will fail. You can install QEMU manually in your terminal if this occurs:
```bash
sudo apt install -y qemu-system qemu-utils
```
