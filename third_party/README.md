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

**Windows notes:** Meson needs Windows-style `PKG_CONFIG=C:/msys64/mingw64/bin/pkg-config.exe`. Do not mix WinLibs gcc with MSYS2 headers. If `cc1.exe` fails with entry-point errors, remove a wrong `C:\WINDOWS\libwinpthread-1.dll` (see `scripts/fix_msys2_gcc_admin.ps1`).

Then configure AQEMU:

```
-DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=<repo>/third_party/qemu-install
```

Install dirs `qemu-build*` / `qemu-install` are gitignored.
