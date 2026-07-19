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
| Windows (MSYS2) | `scripts/build_qemu_windows.sh` / `.ps1` |

Then configure AQEMU:

```
-DAQEMU_BUNDLE_QEMU=ON -DAQEMU_QEMU_PREFIX=<repo>/third_party/qemu-install
```

Install dirs `qemu-build*` / `qemu-install` are gitignored.
