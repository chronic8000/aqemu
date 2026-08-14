# Bundled QEMU for AQEMU

Pin (current): **v11.0.2** via submodule `third_party/qemu`.

```bash
git submodule update --init --depth 1 third_party/qemu
# bump: cd third_party/qemu && git fetch --tags && git checkout vX.Y.Z
# tip of the 11.0 stable line (moving): git checkout origin/staging-11.0
```

| Host | Script |
|------|--------|
| Linux / Pi | `scripts/build_qemu_linux.sh [PREFIX]` |
| Windows (MSYS2 MinGW64) | `scripts/build_qemu_windows_msys.sh` (after MSYS2 packages are installed) |

Builds **every** `*-softmmu` target (x86, aarch64, arm, ppc, sparc, mips, riscv, m68k, …) via `scripts/qemu_softmmu_targets.sh`, with the **full AQEMU feature set** via `scripts/qemu_feature_flags.sh`:

- **Required:** `slirp` (`-netdev user`), SPICE, VNC  
- **Enabled when present:** curl, libusb, usbredir, gnutls, zstd, WHPX/KVM/HVF  
- **Verify step** fails the build if `-netdev user` is missing

Windows MSYS2 deps (MINGW64):

```
pacman -S --needed mingw-w64-x86_64-libslirp mingw-w64-x86_64-spice \
  mingw-w64-x86_64-libusb mingw-w64-x86_64-usbredir \
  mingw-w64-x86_64-curl mingw-w64-x86_64-gnutls mingw-w64-x86_64-zstd \
  mingw-w64-x86_64-glib2 mingw-w64-x86_64-pixman mingw-w64-x86_64-pkgconf
```

**Windows notes:** Meson needs Windows-style `PKG_CONFIG=C:/msys64/mingw64/bin/pkg-config.exe`. Do not mix WinLibs gcc with MSYS2 headers. If `cc1.exe` fails with entry-point errors, remove a wrong `C:\WINDOWS\libwinpthread-1.dll` (see `scripts/fix_msys2_gcc_admin.ps1`).

Then configure AQEMU:

```
-DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=<repo>/third_party/qemu-install
```

That copies every `qemu-system-*`, `qemu-img*`, runtime DLLs (`libslirp`, `libusb`, …), and the `share/` firmware directory next to `aqemu.exe` (required for MSIX).

Install dirs `qemu-build*` / `qemu-install` are gitignored.

## Optional: pyimg4 (iOS firmware tool)

AQEMU’s **File → iOS Firmware Tool** unpacks IPSWs, forges restore/SEP tickets (bundled Python scripts), packs SEP firmware with external **`img4`**, and processes IM4P with **`pyimg4`**. AQEMU does **not** vendor `img4lib` / `pyimg4` under `third_party/` — put `img4.exe` / `pyimg4.exe` next to `aqemu.exe` or on PATH. Python 3 is only started from the GUI (pip-installs pyasn1 if needed).
