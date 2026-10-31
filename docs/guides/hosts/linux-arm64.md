# AQEMU on Linux ARM64 (Raspberry Pi 5 and other ARM boards)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This page is for 64-bit ARM Linux: a **Raspberry Pi 5** on Raspberry Pi OS / Debian Bookworm, or another AArch64 machine. You build AQEMU from the [open-source repository](https://github.com/chronic8000/aqemu).

**You need a 64-bit OS.** The Pi 5 build script warns if `uname -m` isn't `aarch64`.

---

## 1. Get the source

```sh
git clone --recursive https://github.com/chronic8000/aqemu.git
cd aqemu
```

## 2. Install build tools (Raspberry Pi OS / Debian)

```sh
sudo apt update && sudo apt install -y build-essential cmake ninja-build pkg-config \
  qtbase5-dev libqt5widgets5 libqt5network5 libqt5dbus5 libvncserver-dev \
  libspice-client-glib-2.0-dev libslirp-dev libusb-1.0-0-dev
```

## 3. Build QEMU (optional)

```sh
./scripts/build_qemu_pi5.sh            # all system emulators, tuned for the Pi 5 (Cortex-A76)
./scripts/build_qemu_pi5.sh aarch64    # just qemu-system-aarch64
```

For other ARM64 boards there's a generic script: `./scripts/build_qemu_linux_arm64.sh [TARGET]`.

The Pi 5 QEMU build uses 64 KB ELF segment alignment, so it also works on Pi kernels that use 16 KB pages, and it enables KVM.

## 4. Build AQEMU

```sh
./scripts/build_aqemu_pi5.sh             # builds into build_pi5/
./scripts/build_aqemu_pi5.sh --install   # also installs to ~/.local/bin/aqemu
```

The script turns on Pi 5 tuning (`PI5_OPTIMIZATIONS`) and the embedded SPICE display client.

## Which guests are fast?

| Guest | Accelerator |
| --- | --- |
| **ARM64** guests (Debian ARM64, Ubuntu ARM64, Fedora ARM64…) | **KVM**, CPU `host` |
| 32-bit ARM guests (`qemu-system-arm`: older Raspberry Pi boards, RealView…) | **TCG**. AQEMU treats the `arm` binary as TCG only. |
| x86 / x86_64 guests | **TCG** (emulated, slow on a Pi) |

ARM64 Linux profiles in the wizard (**Linux → ARM64 / AArch64 (Modern ARM)**) use the `virt` machine, VirtIO devices and UEFI firmware, and prefer KVM on ARM hosts.

## Tips for a Raspberry Pi

- RAM is tight. The ARM64 Ubuntu profile starts at 4096 MB, so lower it on a 4 GB Pi and leave room for the host.
- Use a fast SSD for disk images, not the SD card.
- KVM setup is the same as on [Linux x64](linux-x64.md#turn-on-kvm): `/dev/kvm` must exist and be usable by your user.

---

Other hosts: [Linux x64](linux-x64.md) · [Windows on ARM](windows-arm64.md) · [Windows x64](windows-x64.md) · [Documentation Hub](../../README.md)
