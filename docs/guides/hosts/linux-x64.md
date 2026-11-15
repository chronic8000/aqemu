# AQEMU on Linux x64

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This page is for a 64-bit Intel or AMD PC running Linux (Ubuntu, Debian, Fedora, Arch, openSUSE…). On Linux you build AQEMU from the [open-source repository](https://github.com/chronic8000/aqemu). The Microsoft Store version is Windows-only.

---

## 1. Get the source

```sh
git clone --recursive https://github.com/chronic8000/aqemu.git
cd aqemu
```

QEMU lives in the `third_party/qemu` submodule (pinned to QEMU 11.0.2). If you cloned without `--recursive`, run `git submodule update --init third_party/qemu`.

## 2. Install build tools

The build script prints these if anything is missing:

```sh
# Debian / Ubuntu
sudo apt install -y build-essential cmake ninja-build pkg-config qtbase5-dev libqt5widgets5 libqt5network5 libqt5dbus5 libvncserver-dev libspice-client-glib-2.0-dev
# Fedora
sudo dnf install -y gcc-c++ cmake ninja-build pkgconf qt5-qtbase-devel libvncserver-devel spice-glib-devel
# Arch Linux
sudo pacman -S --needed base-devel cmake ninja pkgconf qt5-base libvncserver spice-gtk
```

## 3. Build QEMU (optional)

You can use the QEMU from your distro (`sudo apt install qemu-system` on Debian/Ubuntu) or build the pinned one:

```sh
./scripts/build_qemu_linux_x86_64.sh            # all system emulators
./scripts/build_qemu_linux_x86_64.sh x86_64     # just qemu-system-x86_64
```

If `third_party/qemu-install/bin` exists when you build AQEMU, the AQEMU build bundles it automatically.

## 4. Build AQEMU

```sh
./scripts/build_aqemu_linux_x86_64.sh             # builds into build_linux_x86_64/
./scripts/build_aqemu_linux_x86_64.sh --install   # also installs to ~/.local/bin/aqemu
```

Run it with `./build_linux_x86_64/aqemu` (or `aqemu` after `--install`, if `~/.local/bin` is on your PATH).

> The root README mentions a `build_linux` folder. The script actually builds into `build_linux_x86_64`.

## 5. First start

The [First Start Wizard](../getting-started/first-start.md) suggests `~/.aqemu/` for your VMs. Pick **Use system-installed QEMU** if you're using your distro's QEMU, or the bundled option if you built it.

## Turn on KVM

KVM is hardware acceleration for 64-bit x86 guests (`qemu-system-x86_64`).

1. Check that `/dev/kvm` exists (virtualization must be on in your BIOS/UEFI).
2. Make sure your user can open it. Usually that means being in the `kvm` group (`sudo usermod -aG kvm $USER`, then log out and back in).
3. In the VM's accelerator list, AQEMU shows **KVM** with the tooltip "Linux KVM. The guest matches this PC and /dev/kvm is usable." If not, it says "KVM needs /dev/kvm on this PC. This guest uses TCG until then."

KVM only accelerates guests that match your CPU and use `qemu-system-x86_64`. AQEMU treats the 32-bit `qemu-system-i386` binary as TCG only, and other architectures use TCG too. **Xen** appears in the list too, and is usable only when Xen is present and the guest matches the host.

## Linux-only extras

- **Shared folders:** 9p (VirtIO) and Samba sharing work on Linux hosts. See [File transfer](../features/file-transfer.md).
- **Command line:** `aqemu start "My VM"` and friends. See [Command line](../getting-started/command-line.md).
- **Remote hosts / virt-manager:** **File → Host → Remote Hosts…** ("SSH tunnels to remote QEMU, or open libvirt/Xen/LXC in virt-manager").
- **Audio:** ALSA when the system has it.

---

Other hosts: [Linux ARM64](linux-arm64.md) · [Windows x64](windows-x64.md) · [Windows on ARM](windows-arm64.md) · [Documentation Hub](../../README.md)
