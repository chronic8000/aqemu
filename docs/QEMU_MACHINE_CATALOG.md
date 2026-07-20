# AQEMU New VM Wizard — Human-Readable Catalog

Generated: `2026-07-19T14:05:17Z`  
QEMU: `C:\Program Files\qemu`  
System binaries found: **28**  
Machine definitions probed: **480** (internal)

UI lists use **friendly names only**. QEMU IDs are backend mappings.

Regenerate: `python scripts/probe_qemu_machines.py --qemu-dir "C:/Program Files/qemu"`

---

## How the three lists work

| Wizard path | What the user sees | What AQEMU stores |
|-------------|--------------------|-------------------|
| Operating System | Windows 11, Ubuntu, IRIX… | Profile → binary + machine + devices |
| CPU Architecture | x86-64, ARM64, SPARC64… | `qemu-system-*` target |
| System / Machine | Raspberry Pi 3, SGI Indy, PC (Q35)… | `qemu_machine` id for that target |
| Custom | Advanced picker | User-chosen binary + machine |

---

## List 1 — Guest Operating Systems

### Microsoft

- MS-DOS
- PC DOS
- DR-DOS
- Windows 1.x
- Windows 2.x
- Windows 3.x
- Windows 95
- Windows 98
- Windows ME
- Windows NT 3.x
- Windows NT 4.0
- Windows 2000
- Windows XP
- Windows Vista
- Windows 7
- Windows 8
- Windows 8.1
- Windows 10
- Windows 11
- Windows Server 2000
- Windows Server 2003
- Windows Server 2008
- Windows Server 2012
- Windows Server 2016
- Windows Server 2019
- Windows Server 2022
- Windows Server 2025

### Linux

- Generic Linux
- Ubuntu
- Debian
- Fedora
- Red Hat Enterprise Linux
- Rocky Linux
- AlmaLinux
- SUSE Linux
- openSUSE
- Arch Linux
- Gentoo
- Slackware
- Kali Linux
- Linux Mint
- Alpine Linux
- Tiny Core Linux

### BSD

- FreeBSD
- OpenBSD
- NetBSD
- DragonFly BSD

### Apple

- Mac OS 7
- Mac OS 8
- Mac OS 9
- Mac OS X 10.0–10.4 (PPC)
- Mac OS X 10.5–10.6
- macOS 10.12+
- Darwin

### Sun

- Solaris x86
- Solaris SPARC
- OpenSolaris
- illumos
- OmniOS

### SGI

- IRIX 5.x
- IRIX 6.x

### IBM

- AIX
- OS/2
- eComStation
- ArcaOS

### HP

- HP-UX

### DEC

- Tru64 UNIX
- OpenVMS

### Other

- ReactOS
- Haiku
- BeOS
- KolibriOS
- SerenityOS
- TempleOS
- MenuetOS
- MorphOS
- AmigaOS
- RISC OS
- Android
- ChromeOS
- Other

---

## List 2 — CPU Architectures

### Mainstream

- **x86** — ✓ installed
- **x86-64** — ✓ installed
- **ARM** — ✓ installed
- **ARM64 / AArch64** — ✓ installed
- **RISC-V 32** — ✓ installed
- **RISC-V 64** — ✓ installed

### PowerPC

- **PowerPC 32** — ✓ installed
- **PowerPC 64** — ✓ installed
- **PowerPC 64 LE** — ○ not in this QEMU build

### MIPS

- **MIPS** — ✓ installed
- **MIPS64** — ✓ installed
- **MIPSEL** — ✓ installed
- **MIPS64EL** — ✓ installed

### SPARC

- **SPARC32** — ✓ installed
- **SPARC64** — ✓ installed

### IBM

- **s390x** — ✓ installed

### DEC

- **Alpha** — ✓ installed

### HP

- **PA-RISC** — ✓ installed

### Motorola

- **m68k** — ✓ installed

### Embedded

- **AVR** — ✓ installed
- **MicroBlaze** — ✓ installed
- **OpenRISC** — ✓ installed
- **Xtensa** — ✓ installed
- **RX** — ✓ installed
- **TriCore** — ✓ installed
- **SH4** — ✓ installed
- **LoongArch** — ✓ installed

---

## List 3 — System / Machine Platforms (wizard)

Friendly names for the UI. Rows marked **available** match your installed QEMU.

### IBM PC Compatible

- **PC (i440FX)** — available (`qemu-system-x86_64` → `pc`)
- **PC (Q35)** — available (`qemu-system-x86_64` → `q35`)
- **Modern UEFI PC** — available (`qemu-system-x86_64` → `q35`)
- **Legacy BIOS PC** — available (`qemu-system-x86_64` → `pc`)
- **Legacy ISA PC** — available (`qemu-system-x86_64` → `isapc`)

### ARM Virtual Platforms

- **ARM Virt** — available (`qemu-system-aarch64` → `virt`)
- **SBSA Reference Platform** — available (`qemu-system-aarch64` → `sbsa-ref`)
- **Generic ARM Board** — available (`qemu-system-arm` → `virt`)

### Raspberry Pi

- **Raspberry Pi 1** — available (`qemu-system-aarch64` → `raspi1ap`)
- **Raspberry Pi 2** — available (`qemu-system-aarch64` → `raspi2b`)
- **Raspberry Pi 3** — available (`qemu-system-aarch64` → `raspi3b`)
- **Raspberry Pi 4** — available (`qemu-system-aarch64` → `raspi4b`)

### Apple Macintosh

- **Power Macintosh G3** — available (`qemu-system-ppc64` → `g3beige`)
- **Macintosh (Mac99)** — available (`qemu-system-ppc64` → `mac99`)
- **Macintosh Quadra 800** — available (`qemu-system-m68k` → `q800`)

### Sun Microsystems

- **SPARCstation (sun4m)** — not present in this QEMU install
- **UltraSPARC (sun4u)** — available (`qemu-system-sparc64` → `sun4u`)
- **Sun Niagara** — available (`qemu-system-sparc64` → `niagara`)

### Silicon Graphics

- **SGI Indy** — not present in this QEMU install
- **SGI Magnum** — available (`qemu-system-mips64el` → `magnum`)

### DEC

- **Alpha Clipper** — available (`qemu-system-alpha` → `clipper`)
- **Alpha SX164** — not present in this QEMU install
- **Alpha LX164** — not present in this QEMU install
- **Alpha DP264** — not present in this QEMU install

### HP

- **HP PA-RISC B160L** — not present in this QEMU install
- **HP PA-RISC C3700** — not present in this QEMU install

### IBM Power

- **IBM pSeries** — available (`qemu-system-ppc64` → `pseries`)
- **IBM PowerNV** — available (`qemu-system-ppc64` → `powernv`)
- **POWER8 (PowerNV)** — available (`qemu-system-ppc64` → `powernv8`)
- **POWER9 (PowerNV)** — available (`qemu-system-ppc64` → `powernv9`)
- **POWER10 (PowerNV)** — available (`qemu-system-ppc64` → `powernv10`)

### IBM Mainframe

- **IBM Z (s390-ccw-virtio)** — available (`qemu-system-s390x` → `s390-ccw-virtio`)

### MIPS

- **MIPS Malta** — available (`qemu-system-mips64el` → `malta`)
- **Fuloong 2E** — available (`qemu-system-mips64el` → `fuloong2e`)
- **Loongson 3 Virtual** — available (`qemu-system-mips64el` → `loongson3-virt`)

### RISC-V

- **RISC-V Virt** — available (`qemu-system-riscv64` → `virt`)
- **RISC-V Spike** — available (`qemu-system-riscv64` → `spike`)
- **SiFive E Series** — available (`qemu-system-riscv64` → `sifive_e`)
- **SiFive U Series** — available (`qemu-system-riscv64` → `sifive_u`)
- **OpenTitan** — available (`qemu-system-riscv32` → `opentitan`)

### Embedded

- **Arduino Mega** — available (`qemu-system-avr` → `arduino-mega`)
- **Arduino Duemilanove** — available (`qemu-system-avr` → `arduino-duemilanove`)
- **STM32 VL Discovery** — available (`qemu-system-arm` → `stm32vldiscovery`)
- **Netduino 2** — available (`qemu-system-arm` → `netduino2`)

### Generic

- **Generic Virtual Machine** — available (`qemu-system-aarch64` → `virt`)
- **Empty Machine** — available (`qemu-system-x86_64` → `none`)
- **Custom Machine** — User picks any probed machine
- **Import Existing QEMU Command Line** — Paste -M / full argv later

---

## All platforms on this PC (readable names)

Every non-versioned machine, shown by friendly name. Obscure `virt-9.2` aliases are hidden.

### ARM Virtual Platforms

- **ARM Virtual Machine** (ARM)
- **ARM64 / AArch64 Virtual Machine** (ARM64 / AArch64)
- **SBSA Reference Platform** (ARM64 / AArch64)

### Apple Macintosh

- **Macintosh (Mac99)** (PowerPC 32)
- **Macintosh (Mac99)** (PowerPC 64)
- **Macintosh Quadra 800** (m68k)
- **Power Macintosh G3 (Beige)** (PowerPC 32)
- **Power Macintosh G3 (Beige)** (PowerPC 64)

### DEC

- **Alpha Clipper** (Alpha)
- **Empty Machine (none)** (Alpha)

### Embedded

- **Arduino Duemilanove** (AVR)
- **Arduino Mega** (AVR)
- **Arduino UNO (ATmega328P)** (AVR)
- **Netduino 2** (ARM)
- **Netduino 2** (ARM64 / AArch64)
- **Netduino Plus 2** (ARM)
- **Netduino Plus 2** (ARM64 / AArch64)
- **Olimex STM32-H405 (Cortex-M4)** (ARM)
- **Olimex STM32-H405 (Cortex-M4)** (ARM64 / AArch64)
- **STM32 VL Discovery** (ARM)
- **STM32 VL Discovery** (ARM64 / AArch64)

### Game Consoles

- **ARM MPS2 with AN385 FPGA image for Cortex-M3** (ARM)
- **ARM MPS2 with AN385 FPGA image for Cortex-M3** (ARM64 / AArch64)
- **ARM MPS2 with AN386 FPGA image for Cortex-M4** (ARM)
- **ARM MPS2 with AN386 FPGA image for Cortex-M4** (ARM64 / AArch64)
- **ARM MPS2 with AN500 FPGA image for Cortex-M7** (ARM)
- **ARM MPS2 with AN500 FPGA image for Cortex-M7** (ARM64 / AArch64)
- **ARM MPS2 with AN505 FPGA image for Cortex-M33** (ARM)
- **ARM MPS2 with AN505 FPGA image for Cortex-M33** (ARM64 / AArch64)
- **ARM MPS2 with AN511 DesignStart FPGA image for Cortex-M3** (ARM)
- **ARM MPS2 with AN511 DesignStart FPGA image for Cortex-M3** (ARM64 / AArch64)
- **ARM MPS2 with AN521 FPGA image for dual Cortex-M33** (ARM)
- **ARM MPS2 with AN521 FPGA image for dual Cortex-M33** (ARM64 / AArch64)

### HP

- **Empty Machine (none)** (PA-RISC)
- **HP 715/64 workstation** (PA-RISC)
- **HP A400-44 server** (PA-RISC)
- **HP B160L workstation (default)** (PA-RISC)
- **HP C3700 workstation** (PA-RISC)

### IBM Mainframe

- **IBM Z (s390-ccw-virtio)** (s390x)

### IBM PC Compatible

- **Legacy ISA PC** (x86)
- **Legacy ISA PC** (x86-64)
- **MicroVM** (x86)
- **MicroVM** (x86-64)
- **PC (i440FX)** (x86)
- **PC (i440FX)** (x86-64)
- **PC (Q35)** (x86)
- **PC (Q35)** (x86-64)

### IBM Power

- **IBM PowerNV** (PowerPC 64)
- **IBM PowerNV (Non-Virtualized) POWER10 Rainier** (PowerPC 64)
- **IBM PowerNV (Non-Virtualized) Power11** (PowerPC 64)
- **IBM PowerNV POWER10** (PowerPC 64)
- **IBM PowerNV POWER8** (PowerPC 64)
- **IBM PowerNV POWER9** (PowerPC 64)
- **IBM pSeries** (PowerPC 64)

### MIPS

- **Fuloong 2E** (MIPS64EL)
- **Loongson 3 Virtual** (MIPS64EL)
- **MIPS Malta** (MIPS)
- **MIPS Malta** (MIPS64)
- **MIPS Malta** (MIPS64EL)
- **MIPS Malta** (MIPSEL)

### Other Platforms

- **a minimal TriCore board** (TriCore)
- **Acer Pica 61** (MIPS64)
- **Acer Pica 61** (MIPS64EL)
- **aCube Sam460ex** (PowerPC 32)
- **aCube Sam460ex** (PowerPC 64)
- **AMD Versal Gen 2 Virtual development board** (ARM64 / AArch64)
- **AMD Versal Virtual development board** (ARM64 / AArch64)
- **Arduino Duemilanove (ATmega168)** (AVR)
- **Arduino Mega (ATmega1280)** (AVR)
- **Arduino Mega 2560 (ATmega2560)** (AVR)
- **Arduino UNO (ATmega328P)** (AVR)
- **ARM i.MX25 PDK board (ARM926)** (ARM)
- **ARM i.MX25 PDK board (ARM926)** (ARM64 / AArch64)
- **ARM Integrator/CP (ARM926EJ-S)** (ARM)
- **ARM Integrator/CP (ARM926EJ-S)** (ARM64 / AArch64)
- **ARM KZM Emulation Baseboard (ARM1136)** (ARM)
- **ARM KZM Emulation Baseboard (ARM1136)** (ARM64 / AArch64)
- **ARM MPS3 with AN524 FPGA image for dual Cortex-M33** (ARM)
- **ARM MPS3 with AN524 FPGA image for dual Cortex-M33** (ARM64 / AArch64)
- **ARM MPS3 with AN536 FPGA image for Cortex-R52** (ARM)
- **ARM MPS3 with AN536 FPGA image for Cortex-R52** (ARM64 / AArch64)
- **ARM MPS3 with AN547 FPGA image for Cortex-M55** (ARM)
- **ARM MPS3 with AN547 FPGA image for Cortex-M55** (ARM64 / AArch64)
- **ARM Musca-A board (dual Cortex-M33)** (ARM)
- **ARM Musca-A board (dual Cortex-M33)** (ARM64 / AArch64)
- **ARM Musca-B1 board (dual Cortex-M33)** (ARM)
- **ARM Musca-B1 board (dual Cortex-M33)** (ARM64 / AArch64)
- **ARM RealView Emulation Baseboard (ARM11MPCore)** (ARM)
- **ARM RealView Emulation Baseboard (ARM11MPCore)** (ARM64 / AArch64)
- **ARM RealView Emulation Baseboard (ARM926EJ-S)** (ARM)
- **ARM RealView Emulation Baseboard (ARM926EJ-S)** (ARM64 / AArch64)
- **ARM RealView Platform Baseboard Explore for Cortex-A9** (ARM)
- **ARM RealView Platform Baseboard Explore for Cortex-A9** (ARM64 / AArch64)
- **ARM RealView Platform Baseboard for Cortex-A8** (ARM)
- **ARM RealView Platform Baseboard for Cortex-A8** (ARM64 / AArch64)
- **ARM Versatile Express for Cortex-A15** (ARM)
- **ARM Versatile Express for Cortex-A15** (ARM64 / AArch64)
- **ARM Versatile Express for Cortex-A9** (ARM)
- **ARM Versatile Express for Cortex-A9** (ARM64 / AArch64)
- **ARM Versatile/AB (ARM926EJ-S)** (ARM)
- **ARM Versatile/AB (ARM926EJ-S)** (ARM64 / AArch64)
- **ARM Versatile/PB (ARM926EJ-S)** (ARM)
- **ARM Versatile/PB (ARM926EJ-S)** (ARM64 / AArch64)
- **Arnewsh 5206** (m68k)
- **Aspeed AST1030 MiniBMC (Cortex-M4)** (ARM)
- **Aspeed AST1030 MiniBMC (Cortex-M4)** (ARM64 / AArch64)
- **Aspeed AST1060 Platform Root of Trust (Cortex-M4)** (ARM)
- **Aspeed AST1060 Platform Root of Trust (Cortex-M4)** (ARM64 / AArch64)
- **Aspeed AST2500 EVB (ARM1176)** (ARM)
- **Aspeed AST2500 EVB (ARM1176)** (ARM64 / AArch64)
- **Aspeed AST2600 EVB (Cortex-A7)** (ARM)
- **Aspeed AST2600 EVB (Cortex-A7)** (ARM64 / AArch64)
- **Aspeed AST2700 A1 EVB (Cortex-A35)** (ARM64 / AArch64)
- **Aspeed AST2700 A2 EVB (Cortex-A35)** (ARM64 / AArch64)
- **ast2700 full core support** (ARM64 / AArch64)
- **B-L475E-IOT01A Discovery Kit (Cortex-M4)** (ARM)
- **B-L475E-IOT01A Discovery Kit (Cortex-M4)** (ARM64 / AArch64)
- **bamboo** (PowerPC 32)
- **bamboo** (PowerPC 64)
- **Bananapi M2U (Cortex-A7)** (ARM)
- **Bananapi M2U (Cortex-A7)** (ARM64 / AArch64)
- **BBC micro:bit (Cortex-M0)** (ARM)
- **BBC micro:bit (Cortex-M0)** (ARM64 / AArch64)
- **Bytedance G220A BMC (ARM1176)** (ARM)
- **Bytedance G220A BMC (ARM1176)** (ARM64 / AArch64)
- **Canon PowerShot A1100 IS (ARM946)** (ARM)
- **Canon PowerShot A1100 IS (ARM946)** (ARM64 / AArch64)
- **cubietech cubieboard (Cortex-A8)** (ARM)
- **cubietech cubieboard (Cortex-A8)** (ARM64 / AArch64)
- **Empty Machine (none)** (ARM)
- **Empty Machine (none)** (ARM64 / AArch64)
- **Empty Machine (none)** (AVR)
- **Empty Machine (none)** (LoongArch)
- **Empty Machine (none)** (MIPS)
- **Empty Machine (none)** (MIPS64)
- **Empty Machine (none)** (MIPS64EL)
- **Empty Machine (none)** (MIPSEL)
- **Empty Machine (none)** (MicroBlaze)
- **Empty Machine (none)** (OpenRISC)
- **Empty Machine (none)** (PowerPC 32)
- **Empty Machine (none)** (PowerPC 64)
- **Empty Machine (none)** (RISC-V 32)
- **Empty Machine (none)** (RISC-V 64)
- **Empty Machine (none)** (RX)
- **Empty Machine (none)** (SH4)
- **Empty Machine (none)** (SPARC32)
- **Empty Machine (none)** (SPARC64)
- **Empty Machine (none)** (TriCore)
- **Empty Machine (none)** (Xtensa)
- **Empty Machine (none)** (m68k)
- **Empty Machine (none)** (s390x)
- **Empty Machine (none)** (x86)
- **Empty Machine (none)** (x86-64)
- **Eyetech AmigaOne/Mai Logic Teron** (PowerPC 32)
- **Eyetech AmigaOne/Mai Logic Teron** (PowerPC 64)
- **Facebook Bletchley BMC (Cortex-A7)** (ARM)
- **Facebook Bletchley BMC (Cortex-A7)** (ARM64 / AArch64)
- **Facebook Catalina BMC (Cortex-A7)** (ARM)
- **Facebook Catalina BMC (Cortex-A7)** (ARM64 / AArch64)
- **Facebook fby35 BMC (Cortex-A7)** (ARM)
- **Facebook fby35 BMC (Cortex-A7)** (ARM64 / AArch64)
- **Facebook Fuji BMC (Cortex-A7)** (ARM)
- **Facebook Fuji BMC (Cortex-A7)** (ARM64 / AArch64)
- **Facebook Tiogapass BMC (ARM1176)** (ARM)
- **Facebook Tiogapass BMC (ARM1176)** (ARM64 / AArch64)
- **Facebook YosemiteV2 BMC (ARM1176)** (ARM)
- **Facebook YosemiteV2 BMC (ARM1176)** (ARM64 / AArch64)
- **Freescale i.MX6 Quad SABRE Lite Board (Cortex-A9)** (ARM)
- **Freescale i.MX6 Quad SABRE Lite Board (Cortex-A9)** (ARM64 / AArch64)
- **Freescale i.MX6UL Evaluation Kit (Cortex-A7)** (ARM)
- **Freescale i.MX6UL Evaluation Kit (Cortex-A7)** (ARM64 / AArch64)
- **Freescale i.MX7 DUAL SABRE (Cortex-A7)** (ARM)
- **Freescale i.MX7 DUAL SABRE (Cortex-A7)** (ARM64 / AArch64)
- **gdb simulator (R5F562N7 MCU and external RAM)** (RX)
- **gdb simulator (R5F562N8 MCU and external RAM)** (RX)
- **generic paravirt e500 platform** (PowerPC 32)
- **generic paravirt e500 platform** (PowerPC 64)
- **Genesi/bPlan Pegasos I** (PowerPC 32)
- **Genesi/bPlan Pegasos I** (PowerPC 64)
- **Genesi/bPlan Pegasos II** (PowerPC 32)
- **Genesi/bPlan Pegasos II** (PowerPC 64)
- **IBM Rainier BMC (Cortex-A7)** (ARM)
- **IBM Rainier BMC (Cortex-A7)** (ARM64 / AArch64)
- **IBM RS/6000 7020 (40p)** (PowerPC 32)
- **IBM RS/6000 7020 (40p)** (PowerPC 64)
- **Infineon AURIX TriBoard TC277 (D-Step)** (TriCore)
- **Inspur FP5280G2 BMC (ARM1176) (deprecated)** (ARM)
- **Inspur FP5280G2 BMC (ARM1176) (deprecated)** (ARM64 / AArch64)
- **kc705 EVB (dc232b)** (Xtensa)
- **kc705 EVB (fsf)** (Xtensa)
- **kc705 noMMU EVB (de212)** (Xtensa)
- **kc705 noMMU EVB (fsf)** (Xtensa)
- **Kudo BMC (Cortex-A9)** (ARM)
- **Kudo BMC (Cortex-A9)** (ARM64 / AArch64)
- **Leon-3 generic** (SPARC32)
- **LoongArch Virtual Machine** (LoongArch)
- **lx200 EVB (dc232b)** (Xtensa)
- **lx200 EVB (fsf)** (Xtensa)
- **lx200 noMMU EVB (de212)** (Xtensa)
- **lx200 noMMU EVB (fsf)** (Xtensa)
- **lx60 EVB (dc232b)** (Xtensa)
- **lx60 EVB (fsf)** (Xtensa)
- **lx60 noMMU EVB (de212)** (Xtensa)
- **lx60 noMMU EVB (fsf)** (Xtensa)
- **m68k Virtual Machine** (m68k)
- **Marvell 88w8618 / MusicPal (ARM926EJ-S)** (ARM)
- **Marvell 88w8618 / MusicPal (ARM926EJ-S)** (ARM64 / AArch64)
- **MAX78000FTHR Board (Cortex-M4 / (Unimplemented) RISC-V)** (ARM)
- **MAX78000FTHR Board (Cortex-M4 / (Unimplemented) RISC-V)** (ARM64 / AArch64)
- **MCF5208EVB (default)** (m68k)
- **Meta Platforms fby35 (deprecated)** (ARM)
- **Meta Platforms fby35 (deprecated)** (ARM64 / AArch64)
- **MIPS Boston** (MIPS64EL)
- **MIPS Boston-aia** (RISC-V 64)
- **ml605 EVB (dc232b)** (Xtensa)
- **ml605 EVB (fsf)** (Xtensa)
- **ml605 noMMU EVB (de212)** (Xtensa)
- **ml605 noMMU EVB (fsf)** (Xtensa)
- **Mori BMC (Cortex-A9)** (ARM)
- **Mori BMC (Cortex-A9)** (ARM64 / AArch64)
- **mpc8544ds** (PowerPC 32)
- **mpc8544ds** (PowerPC 64)
- **NeXT Cube** (m68k)
- **Nuvoton NPCM750 Evaluation Board (Cortex-A9)** (ARM)
- **Nuvoton NPCM750 Evaluation Board (Cortex-A9)** (ARM64 / AArch64)
- **Nuvoton NPCM845 Evaluation Board (Cortex-A35)** (ARM64 / AArch64)
- **Nvidia GB200NVL BMC (Cortex-A7)** (ARM)
- **Nvidia GB200NVL BMC (Cortex-A7)** (ARM64 / AArch64)
- **NXP i.MX 8M Plus EVK Board** (ARM64 / AArch64)
- **NXP i.MX 8MM EVK Board** (ARM64 / AArch64)
- **OCP SonoraPass BMC (ARM1176) (deprecated)** (ARM)
- **OCP SonoraPass BMC (ARM1176) (deprecated)** (ARM64 / AArch64)
- **OpenPOWER Palmetto BMC (ARM926EJ-S)** (ARM)
- **OpenPOWER Palmetto BMC (ARM926EJ-S)** (ARM64 / AArch64)
- **OpenPOWER Romulus BMC (ARM1176)** (ARM)
- **OpenPOWER Romulus BMC (ARM1176)** (ARM64 / AArch64)
- **OpenPOWER Witherspoon BMC (ARM1176)** (ARM)
- **OpenPOWER Witherspoon BMC (ARM1176)** (ARM64 / AArch64)
- **OpenRISC Virtual Machine** (OpenRISC)
- **or1k simulation (default)** (OpenRISC)
- **Orange Pi PC (Cortex-A7)** (ARM)
- **Orange Pi PC (Cortex-A7)** (ARM64 / AArch64)
- **PetaLogix linux refdesign for xilinx ml605 (little endian)** (MicroBlaze)
- **PPE42 Test Machine** (PowerPC 32)
- **PPE42 Test Machine** (PowerPC 64)
- **Qualcomm DC-SCM V1 BMC (Cortex A7) (deprecated)** (ARM)
- **Qualcomm DC-SCM V1 BMC (Cortex A7) (deprecated)** (ARM64 / AArch64)
- **Qualcomm DC-SCM V1/Firework BMC (Cortex A7) (deprecated)** (ARM)
- **Qualcomm DC-SCM V1/Firework BMC (Cortex A7) (deprecated)** (ARM64 / AArch64)
- **Quanta GBS (Cortex-A9)** (ARM)
- **Quanta GBS (Cortex-A9)** (ARM64 / AArch64)
- **Quanta GSJ (Cortex-A9)** (ARM)
- **Quanta GSJ (Cortex-A9)** (ARM64 / AArch64)
- **Quanta-Q71l BMC (ARM926EJ-S)** (ARM)
- **Quanta-Q71l BMC (ARM926EJ-S)** (ARM64 / AArch64)
- **r2d-plus board** (SH4)
- **RISC-V Board compatible with Shakti SDK** (RISC-V 64)
- **RISC-V Board compatible with the Xiangshan Kunminghu FPGA prototype platform** (RISC-V 64)
- **Samsung NURI board (Exynos4210)** (ARM)
- **Samsung NURI board (Exynos4210)** (ARM64 / AArch64)
- **Samsung SMDKC210 board (Exynos4210)** (ARM)
- **Samsung SMDKC210 board (Exynos4210)** (ARM64 / AArch64)
- **Sharp SL-5500 (Collie) PDA (SA-1110)** (ARM)
- **Sharp SL-5500 (Collie) PDA (SA-1110)** (ARM64 / AArch64)
- **Siemens SX1 (OMAP310) V1** (ARM)
- **Siemens SX1 (OMAP310) V1** (ARM64 / AArch64)
- **Siemens SX1 (OMAP310) V2** (ARM)
- **Siemens SX1 (OMAP310) V2** (ARM64 / AArch64)
- **sim machine (dc232b) (default)** (Xtensa)
- **sim machine (fsf) (default)** (Xtensa)
- **SmartFusion2 SOM kit from Emcraft (M2S010)** (ARM)
- **SmartFusion2 SOM kit from Emcraft (M2S010)** (ARM64 / AArch64)
- **Stellaris LM3S6965EVB (Cortex-M3)** (ARM)
- **Stellaris LM3S6965EVB (Cortex-M3)** (ARM64 / AArch64)
- **Stellaris LM3S811EVB (Cortex-M3)** (ARM)
- **Stellaris LM3S811EVB (Cortex-M3)** (ARM64 / AArch64)
- **Sun4m platform, SPARCbook** (SPARC32)
- **Sun4m platform, SPARCClassic** (SPARC32)
- **Sun4m platform, SPARCserver 600MP** (SPARC32)
- **Sun4m platform, SPARCstation 10** (SPARC32)
- **Sun4m platform, SPARCstation 20** (SPARC32)
- **Sun4m platform, SPARCstation 4** (SPARC32)
- **Sun4m platform, SPARCstation 5 (default)** (SPARC32)
- **Sun4m platform, SPARCstation LX** (SPARC32)
- **Sun4m platform, SPARCstation Voyager** (SPARC32)
- **Supermicro X11 BMC (ARM926EJ-S)** (ARM)
- **Supermicro X11 BMC (ARM926EJ-S)** (ARM64 / AArch64)
- **Xilinx Virtex ML507 reference design** (PowerPC 32)
- **Xilinx Virtex ML507 reference design** (PowerPC 64)
- **Xilinx Zynq 7000 Platform Baseboard for Cortex-A9** (ARM)
- **Xilinx Zynq 7000 Platform Baseboard for Cortex-A9** (ARM64 / AArch64)
- **Xilinx ZynqMP PMU machine (little endian)** (MicroBlaze)
- **Xilinx ZynqMP ZCU102 board with 4xA53s and 2xR5Fs based on the value of smp** (ARM64 / AArch64)
- **Xtensa Virtual Machine** (Xtensa)

### RISC-V

- **OpenTitan** (RISC-V 32)
- **RISC-V 32 Virtual Machine** (RISC-V 32)
- **RISC-V 64 Virtual Machine** (RISC-V 64)
- **RISC-V Spike** (RISC-V 32)
- **RISC-V Spike** (RISC-V 64)
- **SiFive E Series** (RISC-V 32)
- **SiFive E Series** (RISC-V 64)
- **SiFive U Series** (RISC-V 32)
- **SiFive U Series** (RISC-V 64)

### Raspberry Pi

- **Raspberry Pi 1** (ARM)
- **Raspberry Pi 1** (ARM64 / AArch64)
- **Raspberry Pi 2** (ARM)
- **Raspberry Pi 2** (ARM64 / AArch64)
- **Raspberry Pi 3** (ARM64 / AArch64)
- **Raspberry Pi 3A+** (ARM64 / AArch64)
- **Raspberry Pi 4** (ARM64 / AArch64)
- **Raspberry Pi Zero** (ARM)
- **Raspberry Pi Zero** (ARM64 / AArch64)

### Silicon Graphics

- **SGI Magnum** (MIPS64)
- **SGI Magnum** (MIPS64EL)

### Sun Microsystems

- **Sun Niagara** (SPARC64)
- **Sun Niagara (sun4v)** (SPARC64)
- **Sun UltraSPARC (sun4u)** (SPARC64)

---

## Developer appendix — raw QEMU IDs

Not for the wizard UI. Use only when debugging mappings.

### qemu-system-aarch64

- `xlnx-versal-virt` → AMD Versal Virtual development board
- `amd-versal-virt` → AMD Versal Virtual development board
- `amd-versal2-virt` → AMD Versal Gen 2 Virtual development board
- `ast1030-evb` → Aspeed AST1030 MiniBMC (Cortex-M4)
- `ast1060-evb` → Aspeed AST1060 Platform Root of Trust (Cortex-M4)
- `ast2500-evb` → Aspeed AST2500 EVB (ARM1176)
- `ast2600-evb` → Aspeed AST2600 EVB (Cortex-A7)
- `ast2700a1-evb` → Aspeed AST2700 A1 EVB (Cortex-A35)
- `ast2700-evb` → Aspeed AST2700 A2 EVB (Cortex-A35)
- `ast2700a2-evb` → Aspeed AST2700 A2 EVB (Cortex-A35)
- `ast2700fc` → ast2700 full core support
- `b-l475e-iot01a` → B-L475E-IOT01A Discovery Kit (Cortex-M4)
- `bletchley-bmc` → Facebook Bletchley BMC (Cortex-A7)
- `bpim2u` → Bananapi M2U (Cortex-A7)
- `canon-a1100` → Canon PowerShot A1100 IS (ARM946)
- `catalina-bmc` → Facebook Catalina BMC (Cortex-A7)
- `collie` → Sharp SL-5500 (Collie) PDA (SA-1110)
- `cubieboard` → cubietech cubieboard (Cortex-A8)
- `emcraft-sf2` → SmartFusion2 SOM kit from Emcraft (M2S010)
- `fby35-bmc` → Facebook fby35 BMC (Cortex-A7)
- `fby35` → Meta Platforms fby35 (deprecated)
- `fp5280g2-bmc` → Inspur FP5280G2 BMC (ARM1176) (deprecated)
- `fuji-bmc` → Facebook Fuji BMC (Cortex-A7)
- `g220a-bmc` → Bytedance G220A BMC (ARM1176)
- `gb200nvl-bmc` → Nvidia GB200NVL BMC (Cortex-A7)
- `imx25-pdk` → ARM i.MX25 PDK board (ARM926)
- `imx8mm-evk` → NXP i.MX 8MM EVK Board
- `imx8mp-evk` → NXP i.MX 8M Plus EVK Board
- `integratorcp` → ARM Integrator/CP (ARM926EJ-S)
- `kudo-bmc` → Kudo BMC (Cortex-A9)
- …and 64 more (see JSON)

### qemu-system-alpha

- `clipper` → Alpha Clipper
- `none` → Empty Machine (none)

### qemu-system-arm

- `ast1030-evb` → Aspeed AST1030 MiniBMC (Cortex-M4)
- `ast1060-evb` → Aspeed AST1060 Platform Root of Trust (Cortex-M4)
- `ast2500-evb` → Aspeed AST2500 EVB (ARM1176)
- `ast2600-evb` → Aspeed AST2600 EVB (Cortex-A7)
- `b-l475e-iot01a` → B-L475E-IOT01A Discovery Kit (Cortex-M4)
- `bletchley-bmc` → Facebook Bletchley BMC (Cortex-A7)
- `bpim2u` → Bananapi M2U (Cortex-A7)
- `canon-a1100` → Canon PowerShot A1100 IS (ARM946)
- `catalina-bmc` → Facebook Catalina BMC (Cortex-A7)
- `collie` → Sharp SL-5500 (Collie) PDA (SA-1110)
- `cubieboard` → cubietech cubieboard (Cortex-A8)
- `emcraft-sf2` → SmartFusion2 SOM kit from Emcraft (M2S010)
- `fby35-bmc` → Facebook fby35 BMC (Cortex-A7)
- `fby35` → Meta Platforms fby35 (deprecated)
- `fp5280g2-bmc` → Inspur FP5280G2 BMC (ARM1176) (deprecated)
- `fuji-bmc` → Facebook Fuji BMC (Cortex-A7)
- `g220a-bmc` → Bytedance G220A BMC (ARM1176)
- `gb200nvl-bmc` → Nvidia GB200NVL BMC (Cortex-A7)
- `imx25-pdk` → ARM i.MX25 PDK board (ARM926)
- `integratorcp` → ARM Integrator/CP (ARM926EJ-S)
- `kudo-bmc` → Kudo BMC (Cortex-A9)
- `kzm` → ARM KZM Emulation Baseboard (ARM1136)
- `lm3s6965evb` → Stellaris LM3S6965EVB (Cortex-M3)
- `lm3s811evb` → Stellaris LM3S811EVB (Cortex-M3)
- `max78000fthr` → MAX78000FTHR Board (Cortex-M4 / (Unimplemented) RISC-V)
- `mcimx6ul-evk` → Freescale i.MX6UL Evaluation Kit (Cortex-A7)
- `mcimx7d-sabre` → Freescale i.MX7 DUAL SABRE (Cortex-A7)
- `microbit` → BBC micro:bit (Cortex-M0)
- `mori-bmc` → Mori BMC (Cortex-A9)
- `mps2-an385` → ARM MPS2 with AN385 FPGA image for Cortex-M3
- …and 49 more (see JSON)

### qemu-system-avr

- `2009` → Arduino Duemilanove (ATmega168)
- `arduino-duemilanove` → Arduino Duemilanove
- `mega2560` → Arduino Mega 2560 (ATmega2560)
- `mega` → Arduino Mega (ATmega1280)
- `arduino-mega` → Arduino Mega
- `uno` → Arduino UNO (ATmega328P)
- `arduino-uno` → Arduino UNO (ATmega328P)
- `none` → Empty Machine (none)

### qemu-system-hppa

- `715` → HP 715/64 workstation
- `A400` → HP A400-44 server
- `B160L` → HP B160L workstation (default)
- `C3700` → HP C3700 workstation
- `none` → Empty Machine (none)

### qemu-system-i386

- `microvm` → MicroVM
- `pc` → PC (i440FX)
- `q35` → PC (Q35)
- `isapc` → Legacy ISA PC
- `none` → Empty Machine (none)

### qemu-system-loongarch64

- `none` → Empty Machine (none)
- `virt` → LoongArch Virtual Machine

### qemu-system-m68k

- `an5206` → Arnewsh 5206
- `mcf5208evb` → MCF5208EVB (default)
- `next-cube` → NeXT Cube
- `none` → Empty Machine (none)
- `q800` → Macintosh Quadra 800
- `virt` → m68k Virtual Machine

### qemu-system-microblaze

- `none` → Empty Machine (none)
- `petalogix-ml605` → PetaLogix linux refdesign for xilinx ml605 (little endian)
- `xlnx-zynqmp-pmu` → Xilinx ZynqMP PMU machine (little endian)

### qemu-system-mips

- `malta` → MIPS Malta
- `none` → Empty Machine (none)

### qemu-system-mips64

- `magnum` → SGI Magnum
- `malta` → MIPS Malta
- `none` → Empty Machine (none)
- `pica61` → Acer Pica 61

### qemu-system-mips64el

- `boston` → MIPS Boston
- `fuloong2e` → Fuloong 2E
- `loongson3-virt` → Loongson 3 Virtual
- `magnum` → SGI Magnum
- `malta` → MIPS Malta
- `none` → Empty Machine (none)
- `pica61` → Acer Pica 61

### qemu-system-mipsel

- `malta` → MIPS Malta
- `none` → Empty Machine (none)

### qemu-system-or1k

- `none` → Empty Machine (none)
- `or1k-sim` → or1k simulation (default)
- `virt` → OpenRISC Virtual Machine

### qemu-system-ppc

- `40p` → IBM RS/6000 7020 (40p)
- `amigaone` → Eyetech AmigaOne/Mai Logic Teron
- `bamboo` → bamboo
- `g3beige` → Power Macintosh G3 (Beige)
- `mac99` → Macintosh (Mac99)
- `mpc8544ds` → mpc8544ds
- `none` → Empty Machine (none)
- `pegasos1` → Genesi/bPlan Pegasos I
- `pegasos2` → Genesi/bPlan Pegasos II
- `ppce500` → generic paravirt e500 platform
- `ppe42_machine` → PPE42 Test Machine
- `sam460ex` → aCube Sam460ex
- `virtex-ml507` → Xilinx Virtex ML507 reference design

### qemu-system-ppc64

- `40p` → IBM RS/6000 7020 (40p)
- `amigaone` → Eyetech AmigaOne/Mai Logic Teron
- `bamboo` → bamboo
- `g3beige` → Power Macintosh G3 (Beige)
- `mac99` → Macintosh (Mac99)
- `mpc8544ds` → mpc8544ds
- `none` → Empty Machine (none)
- `pegasos1` → Genesi/bPlan Pegasos I
- `pegasos2` → Genesi/bPlan Pegasos II
- `powernv` → IBM PowerNV
- `powernv10` → IBM PowerNV POWER10
- `powernv` → IBM PowerNV
- `powernv10-rainier` → IBM PowerNV (Non-Virtualized) POWER10 Rainier
- `powernv11` → IBM PowerNV (Non-Virtualized) Power11
- `powernv8` → IBM PowerNV POWER8
- `powernv9` → IBM PowerNV POWER9
- `ppce500` → generic paravirt e500 platform
- `ppe42_machine` → PPE42 Test Machine
- `pseries` → IBM pSeries
- `sam460ex` → aCube Sam460ex
- `virtex-ml507` → Xilinx Virtex ML507 reference design

### qemu-system-riscv32

- `none` → Empty Machine (none)
- `opentitan` → OpenTitan
- `sifive_e` → SiFive E Series
- `sifive_u` → SiFive U Series
- `spike` → RISC-V Spike
- `virt` → RISC-V 32 Virtual Machine

### qemu-system-riscv64

- `boston-aia` → MIPS Boston-aia
- `none` → Empty Machine (none)
- `shakti_c` → RISC-V Board compatible with Shakti SDK
- `sifive_e` → SiFive E Series
- `sifive_u` → SiFive U Series
- `spike` → RISC-V Spike
- `virt` → RISC-V 64 Virtual Machine
- `xiangshan-kunminghu` → RISC-V Board compatible with the Xiangshan Kunminghu FPGA prototype platform

### qemu-system-rx

- `gdbsim-r5f562n7` → gdb simulator (R5F562N7 MCU and external RAM)
- `gdbsim-r5f562n8` → gdb simulator (R5F562N8 MCU and external RAM)
- `none` → Empty Machine (none)

### qemu-system-s390x

- `none` → Empty Machine (none)
- `s390-ccw-virtio` → IBM Z (s390-ccw-virtio)

### qemu-system-sh4

- `none` → Empty Machine (none)
- `r2d` → r2d-plus board

### qemu-system-sh4eb

- `none` → Empty Machine (none)
- `r2d` → r2d-plus board

### qemu-system-sparc

- `LX` → Sun4m platform, SPARCstation LX
- `SPARCClassic` → Sun4m platform, SPARCClassic
- `SPARCbook` → Sun4m platform, SPARCbook
- `SS-10` → Sun4m platform, SPARCstation 10
- `SS-20` → Sun4m platform, SPARCstation 20
- `SS-4` → Sun4m platform, SPARCstation 4
- `SS-5` → Sun4m platform, SPARCstation 5 (default)
- `SS-600MP` → Sun4m platform, SPARCserver 600MP
- `Voyager` → Sun4m platform, SPARCstation Voyager
- `leon3_generic` → Leon-3 generic
- `none` → Empty Machine (none)

### qemu-system-sparc64

- `niagara` → Sun Niagara
- `none` → Empty Machine (none)
- `sun4u` → Sun UltraSPARC (sun4u)
- `sun4v` → Sun Niagara (sun4v)

### qemu-system-tricore

- `KIT_AURIX_TC277_TRB` → Infineon AURIX TriBoard TC277 (D-Step)
- `none` → Empty Machine (none)
- `tricore_testboard` → a minimal TriCore board

### qemu-system-x86_64

- `microvm` → MicroVM
- `pc` → PC (i440FX)
- `q35` → PC (Q35)
- `isapc` → Legacy ISA PC
- `none` → Empty Machine (none)

### qemu-system-xtensa

- `kc705` → kc705 EVB (dc232b)
- `kc705-nommu` → kc705 noMMU EVB (de212)
- `lx200` → lx200 EVB (dc232b)
- `lx200-nommu` → lx200 noMMU EVB (de212)
- `lx60` → lx60 EVB (dc232b)
- `lx60-nommu` → lx60 noMMU EVB (de212)
- `ml605` → ml605 EVB (dc232b)
- `ml605-nommu` → ml605 noMMU EVB (de212)
- `none` → Empty Machine (none)
- `sim` → sim machine (dc232b) (default)
- `virt` → Xtensa Virtual Machine

### qemu-system-xtensaeb

- `kc705` → kc705 EVB (fsf)
- `kc705-nommu` → kc705 noMMU EVB (fsf)
- `lx200` → lx200 EVB (fsf)
- `lx200-nommu` → lx200 noMMU EVB (fsf)
- `lx60` → lx60 EVB (fsf)
- `lx60-nommu` → lx60 noMMU EVB (fsf)
- `ml605` → ml605 EVB (fsf)
- `ml605-nommu` → ml605 noMMU EVB (fsf)
- `none` → Empty Machine (none)
- `sim` → sim machine (fsf) (default)
- `virt` → Xtensa Virtual Machine

