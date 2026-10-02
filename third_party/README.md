# Bundled QEMU for AQEMU

Pin (current): **v11.0.2** via submodule `third_party/qemu`.

That tree stays unmodified upstream QEMU. Guest and host choices (which binary, which audiodev, which machine) are made in AQEMU. A person building QEMU from this checkout gets the same source as the v11.0.2 tag.

```bash
git submodule update --init --depth 1 third_party/qemu
# bump: cd third_party/qemu && git fetch --tags && git checkout vX.Y.Z
# tip of the 11.0 stable line (moving): git checkout origin/staging-11.0
```

## Host-Specific QEMU Build Scripts

| Target Host | Architecture | Script | Notes |
|:---|:---|:---|:---|
| **Raspberry Pi 5** | ARM64 / AArch64 | [`scripts/build_qemu_pi5.sh`](../scripts/build_qemu_pi5.sh) | Cortex-A76 tuning, 64KB page alignment for 16KB kernels, KVM |
| **Linux ARM64** | Generic ARM64 | [`scripts/build_qemu_linux_arm64.sh`](../scripts/build_qemu_linux_arm64.sh) | KVM hardware acceleration, full AQEMU flags |
| **Linux x86_64** | x86_64 | [`scripts/build_qemu_linux_x86_64.sh`](../scripts/build_qemu_linux_x86_64.sh) | KVM hardware acceleration, full AQEMU flags |
| **Windows on ARM (Snapdragon)** | ARM64 | [`scripts/build_qemu_win_arm64.sh`](../scripts/build_qemu_win_arm64.sh) / [`.ps1`](../scripts/build_qemu_win_arm64.ps1) | Native MSYS2 CLANGARM64, WHPX acceleration |
| **Windows x86_64** | x86_64 | [`scripts/build_qemu_win_x86_64.sh`](../scripts/build_qemu_win_x86_64.sh) / [`.ps1`](../scripts/build_qemu_win_x86_64.ps1) | MSYS2 UCRT64 / MinGW64, WHPX acceleration |
| **Universal Linux** | Any | [`scripts/build_qemu_linux.sh`](../scripts/build_qemu_linux.sh) | Auto-routes to Pi 5, Linux ARM64, or Linux x86_64 script |
| **Universal Windows** | Any | [`scripts/build_qemu_windows.sh`](../scripts/build_qemu_windows.sh) | Auto-routes to WoA ARM64 or Win x86_64 script |

Each script accepts an optional target and prefix:
```bash
./scripts/build_qemu_pi5.sh [TARGET] [PREFIX]
# Examples:
./scripts/build_qemu_pi5.sh aarch64
./scripts/build_qemu_win_arm64.sh x86_64
```

---

## Individual Architecture & Target Scripts (`scripts/qemu_targets/`)

Individual build scripts are provided for **all 31 supported target architectures**, including upstream QEMU emulators plus **ChefKiss Inferno** (`qemu-system-applesoc`) and **steelbrain Reims vGPU** (`qemu-system-reims3d` / `qemu-system-reimsvgpu`):

| Script | Emulated Target Binary | Architecture Description |
|:---|:---|:---|
| [`build_qemu_x86_64.sh`](../scripts/qemu_targets/build_qemu_x86_64.sh) | `qemu-system-x86_64` | Intel / AMD 64-bit PC |
| [`build_qemu_i386.sh`](../scripts/qemu_targets/build_qemu_i386.sh) | `qemu-system-i386` | Intel / AMD 32-bit PC |
| [`build_qemu_aarch64.sh`](../scripts/qemu_targets/build_qemu_aarch64.sh) | `qemu-system-aarch64` | ARM 64-bit (ARMv8-A / ARMv9-A) |
| [`build_qemu_arm.sh`](../scripts/qemu_targets/build_qemu_arm.sh) | `qemu-system-arm` | ARM 32-bit |
| [`build_qemu_applesoc.sh`](../scripts/qemu_targets/build_qemu_applesoc.sh) | `qemu-system-applesoc` | **ChefKiss Inferno** Apple Silicon (iOS / macOS A13/t8030) |
| [`build_qemu_reims.sh`](../scripts/qemu_targets/build_qemu_reims.sh) | `qemu-system-reims3d` | **steelbrain Reims vGPU** Metal → Vulkan paravirtualization |
| [`build_qemu_ppc64.sh`](../scripts/qemu_targets/build_qemu_ppc64.sh) | `qemu-system-ppc64` | PowerPC 64-bit (PowerNV, pSeries) |
| [`build_qemu_ppc.sh`](../scripts/qemu_targets/build_qemu_ppc.sh) | `qemu-system-ppc` | PowerPC 32-bit (Mac G3/G4, PREP) |
| [`build_qemu_riscv64.sh`](../scripts/qemu_targets/build_qemu_riscv64.sh) | `qemu-system-riscv64` | RISC-V 64-bit |
| [`build_qemu_riscv32.sh`](../scripts/qemu_targets/build_qemu_riscv32.sh) | `qemu-system-riscv32` | RISC-V 32-bit |
| [`build_qemu_mips64el.sh`](../scripts/qemu_targets/build_qemu_mips64el.sh) | `qemu-system-mips64el` | MIPS 64-bit Little Endian |
| [`build_qemu_mips64.sh`](../scripts/qemu_targets/build_qemu_mips64.sh) | `qemu-system-mips64` | MIPS 64-bit Big Endian |
| [`build_qemu_mipsel.sh`](../scripts/qemu_targets/build_qemu_mipsel.sh) | `qemu-system-mipsel` | MIPS 32-bit Little Endian |
| [`build_qemu_mips.sh`](../scripts/qemu_targets/build_qemu_mips.sh) | `qemu-system-mips` | MIPS 32-bit Big Endian |
| [`build_qemu_s390x.sh`](../scripts/qemu_targets/build_qemu_s390x.sh) | `qemu-system-s390x` | IBM S390x Mainframe |
| [`build_qemu_sparc64.sh`](../scripts/qemu_targets/build_qemu_sparc64.sh) | `qemu-system-sparc64` | SPARC 64-bit (Sun UltraSPARC) |
| [`build_qemu_sparc.sh`](../scripts/qemu_targets/build_qemu_sparc.sh) | `qemu-system-sparc` | SPARC 32-bit (Sun SPARCstation) |
| [`build_qemu_m68k.sh`](../scripts/qemu_targets/build_qemu_m68k.sh) | `qemu-system-m68k` | Motorola 68000 / ColdFire (Classic Mac / Amiga / NeXT) |
| [`build_qemu_loongarch64.sh`](../scripts/qemu_targets/build_qemu_loongarch64.sh) | `qemu-system-loongarch64` | LoongArch 64-bit |
| [`build_qemu_alpha.sh`](../scripts/qemu_targets/build_qemu_alpha.sh) | `qemu-system-alpha` | DEC Alpha Server |
| [`build_qemu_hppa.sh`](../scripts/qemu_targets/build_qemu_hppa.sh) | `qemu-system-hppa` | HP PA-RISC |
| [`build_qemu_sh4.sh`](../scripts/qemu_targets/build_qemu_sh4.sh) | `qemu-system-sh4` | SuperH SH-4 |
| [`build_qemu_sh4eb.sh`](../scripts/qemu_targets/build_qemu_sh4eb.sh) | `qemu-system-sh4eb` | SuperH SH-4 Big Endian |
| [`build_qemu_microblaze.sh`](../scripts/qemu_targets/build_qemu_microblaze.sh) | `qemu-system-microblaze` | Xilinx MicroBlaze |
| [`build_qemu_microblazeel.sh`](../scripts/qemu_targets/build_qemu_microblazeel.sh) | `qemu-system-microblazeel` | Xilinx MicroBlaze Little Endian |
| [`build_qemu_or1k.sh`](../scripts/qemu_targets/build_qemu_or1k.sh) | `qemu-system-or1k` | OpenRISC 1000 |
| [`build_qemu_rx.sh`](../scripts/qemu_targets/build_qemu_rx.sh) | `qemu-system-rx` | Renesas RX |
| [`build_qemu_avr.sh`](../scripts/qemu_targets/build_qemu_avr.sh) | `qemu-system-avr` | AVR Microcontroller |
| [`build_qemu_tricore.sh`](../scripts/qemu_targets/build_qemu_tricore.sh) | `qemu-system-tricore` | Infineon TriCore |
| [`build_qemu_xtensa.sh`](../scripts/qemu_targets/build_qemu_xtensa.sh) | `qemu-system-xtensa` | Tensilica Xtensa |
| [`build_qemu_xtensaeb.sh`](../scripts/qemu_targets/build_qemu_xtensaeb.sh) | `qemu-system-xtensaeb` | Tensilica Xtensa Big Endian |

To build all 31 individual targets in batch:
```bash
./scripts/qemu_targets/build_all_targets.sh [PREFIX]
```

---

## Features Built Into Every Target

All individual and host-level scripts configure QEMU with the **full AQEMU feature set** via `scripts/qemu_feature_flags.sh`:

- **User Networking:** `slirp` (`-netdev user`) required for unprivileged guest networking
- **Display Protocols:** Embedded SPICE server and VNC display
- **Peripherals:** `libusb`, `usbredir` (USB redirection), `curl`, `gnutls`, `zstd`
- **Native Accelerators:** KVM (Linux / Pi 5), WHPX (Windows x64 and Windows on ARM)
- **Headless Display:** `--disable-sdl --disable-gtk` (AQEMU embeds guest displays seamlessly via SPICE/VNC)
- **Firmware:** Bundles `share/` firmware (`bios-256k.bin`, `edk2-*`, etc.) into `${PREFIX}/share/`
