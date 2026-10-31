# ARM, RISC-V, PowerPC, MIPS and other boards

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

AQEMU's **Consoles & Retro** list has 69 boards and machines: Raspberry Pi, Arduino, BBC micro:bit, ARM development boards, RISC-V, PowerPC, MIPS, Amiga-compatible and more. Most of these aren't desktop PCs. Many expect a firmware file, an SD card image or a kernel rather than an installer ISO, and their tips say so.

## Pick boards from the Platform path

1. Click **Add New VM** (or **Ctrl+N**).
2. Choose **System / Machine Platform**, then **Consoles & Retro**, then the board.
3. Continue through **Confirm QEMU system**, then the rest of the wizard.

The Platform path sets the exact board machine listed below.

> **Heads-up (v1):** the same list also appears under **Guest Operating System → Consoles & Retro**. In the current source, 45 of those 69 OS-path profiles start on the generic `virt` machine instead of the named board. If you go that way, change **Machine Type** on the **Confirm QEMU system** page to the value below.

All boards run with TCG (emulation). The only exception is a generic ARM64 `virt` machine on an ARM64 PC, which can use KVM or WHPX. 32-bit ARM boards use `qemu-system-arm`, which AQEMU runs with TCG only.

## The boards

| Board | Emulator | Machine | Speed |
| --- | --- | --- | --- |
| **440 Bamboo** | `qemu-system-ppc` | `bamboo` | TCG |
| **ARM Integrator/CP** | `qemu-system-arm` | `integratorcp` | TCG |
| **ARM Versatile AB** | `qemu-system-arm` | `versatileab` | TCG |
| **ARM Versatile PB** | `qemu-system-arm` | `versatilepb` | TCG |
| **ARM Virt (generic)** | `qemu-system-aarch64` | `virt` | TCG (KVM/WHPX possible for `virt` on an ARM64 PC) |
| **AmigaOne** | `qemu-system-ppc` | `amigaone` | TCG |
| **Arduino Duemilanove** | `qemu-system-avr` | `arduino-duemilanove` | TCG |
| **Arduino Mega** | `qemu-system-avr` | `arduino-mega` | TCG |
| **Arduino Uno** | `qemu-system-avr` | `arduino-uno` | TCG |
| **B-L475E-IOT01A** | `qemu-system-arm` | `b-l475e-iot01a` | TCG |
| **BBC micro:bit** | `qemu-system-arm` | `microbit` | TCG |
| **Cubieboard** | `qemu-system-arm` | `cubieboard` | TCG |
| **Freescale Sabre Lite** | `qemu-system-arm` | `sabrelite` | TCG |
| **IBM Power Series 6050/6070** | `qemu-system-ppc` | `40p` | TCG |
| **Kyoto Microcomputer KZM-ARM11** | `qemu-system-arm` | `kzm` | TCG |
| **Loongson-3 Virt** | `qemu-system-mips64el` | `loongson3-virt` | TCG |
| **MIPS Boston** | `qemu-system-mips64el` | `boston` | TCG |
| **MIPS Magnum** | `qemu-system-mips64` | `magnum` | TCG |
| **MIPS Malta** | `qemu-system-mips64el` | `malta` | TCG |
| **MIPS Pica 61** | `qemu-system-mips64` | `pica61` | TCG |
| **MPC8544DS** | `qemu-system-ppc` | `mpc8544ds` | TCG |
| **MPS2 AN385** | `qemu-system-arm` | `mps2-an385` | TCG |
| **MPS2 AN505** | `qemu-system-arm` | `mps2-an505` | TCG |
| **MPS2 AN521** | `qemu-system-arm` | `mps2-an521` | TCG |
| **MPS3 AN524** | `qemu-system-arm` | `mps3-an524` | TCG |
| **MPS3 AN547** | `qemu-system-arm` | `mps3-an547` | TCG |
| **Macintosh Quadra 800** | `qemu-system-m68k` | `q800` | TCG |
| **NeXT Cube** | `qemu-system-m68k` | `next-cube` | TCG |
| **Netduino 2** | `qemu-system-arm` | `netduino2` | TCG |
| **Netduino Plus 2** | `qemu-system-arm` | `netduinoplus2` | TCG |
| **Olimex STM32-H405** | `qemu-system-arm` | `olimex-stm32-h405` | TCG |
| **OpenTitan** | `qemu-system-riscv32` | `opentitan` | TCG |
| **Orange Pi PC** | `qemu-system-arm` | `orangepi-pc` | TCG |
| **Pegasos I** | `qemu-system-ppc` | `pegasos1` | TCG |
| **Pegasos II** | `qemu-system-ppc` | `pegasos2` | TCG |
| **Petalogix ML605** | `qemu-system-microblaze` | `petalogix-ml605` | TCG |
| **PowerMac G3 Beige** | `qemu-system-ppc` | `g3beige` | TCG |
| **PowerMac Mac99** | `qemu-system-ppc` | `mac99` | TCG |
| **PowerPC e500** | `qemu-system-ppc` | `ppce500` | TCG |
| **RISC-V Spike** | `qemu-system-riscv64` | `spike` | TCG |
| **Raspberry Pi 1** | `qemu-system-arm` | `raspi1ap` | TCG |
| **Raspberry Pi 2** | `qemu-system-arm` | `raspi2b` | TCG |
| **Raspberry Pi 3** | `qemu-system-aarch64` | `raspi3b` | TCG |
| **Raspberry Pi 3A+** | `qemu-system-aarch64` | `raspi3ap` | TCG |
| **Raspberry Pi 4** | `qemu-system-aarch64` | `raspi4b` | TCG |
| **Raspberry Pi Zero** | `qemu-system-arm` | `raspi0` | TCG |
| **RealView EB** | `qemu-system-arm` | `realview-eb` | TCG |
| **RealView EB MPCore** | `qemu-system-arm` | `realview-eb-mpcore` | TCG |
| **RealView PB-A8** | `qemu-system-arm` | `realview-pb-a8` | TCG |
| **RealView PBX-A9** | `qemu-system-arm` | `realview-pbx-a9` | TCG |
| **Renesas R2D** | `qemu-system-sh4` | `r2d` | TCG |
| **SBSA Reference** | `qemu-system-aarch64` | `sbsa-ref` | TCG |
| **STM32VLDISCOVERY** | `qemu-system-arm` | `stm32vldiscovery` | TCG |
| **Sam460ex** | `qemu-system-ppc` | `sam460ex` | TCG |
| **Sharp Zaurus SL-5500** | `qemu-system-arm` | `collie` | TCG |
| **SiFive E31** | `qemu-system-riscv32` | `sifive_e` | TCG |
| **SiFive HiFive Unleashed** | `qemu-system-riscv64` | `sifive_u` | TCG |
| **Siemens SX1** | `qemu-system-arm` | `sx1` | TCG |
| **Siemens SX1 v1** | `qemu-system-arm` | `sx1-v1` | TCG |
| **Stellaris LM3S6965** | `qemu-system-arm` | `lm3s6965evb` | TCG |
| **Stellaris LM3S811** | `qemu-system-arm` | `lm3s811evb` | TCG |
| **Versatile Express A15** | `qemu-system-arm` | `vexpress-a15` | TCG |
| **Versatile Express A9** | `qemu-system-arm` | `vexpress-a9` | TCG |
| **Xilinx Virtex ML507** | `qemu-system-ppc` | `virtex-ml507` | TCG |
| **Xilinx ZCU102** | `qemu-system-aarch64` | `xlnx-zcu102` | TCG |
| **Xilinx Zynq A9** | `qemu-system-arm` | `xilinx-zynq-a9` | TCG |
| **i.MX25 PDK** | `qemu-system-arm` | `imx25-pdk` | TCG |
| **i.MX6UL EVK** | `qemu-system-arm` | `mcimx6ul-evk` | TCG |
| **i.MX7D Sabre** | `qemu-system-arm` | `mcimx7d-sabre` | TCG |

## AQEMU's tip for each board

- **440 Bamboo:** 440 Bamboo → qemu-system-ppc -M bamboo. PowerPC 440 Bamboo board.
- **ARM Integrator/CP:** ARM Integrator/CP → qemu-system-arm -M integratorcp. ARM Integrator/CP development board.
- **ARM Versatile AB:** ARM Versatile AB → qemu-system-arm -M versatileab. ARM Versatile Application Baseboard.
- **ARM Versatile PB:** ARM Versatile PB → qemu-system-arm -M versatilepb. Classic ARM Versatile Platform Baseboard.
- **ARM Virt (generic):** ARM Virt (generic) → qemu-system-aarch64 -M virt. Generic ARM virt machine — best for Linux/Android-on-ARM VMs.
- **AmigaOne:** AmigaOne → qemu-system-ppc -M amigaone. AmigaOne PowerPC board.
- **Arduino Duemilanove:** Arduino Duemilanove → qemu-system-avr -M arduino-duemilanove. ATmega168 Arduino; load firmware ELF.
- **Arduino Mega:** Arduino Mega → qemu-system-avr -M arduino-mega. ATmega2560 Arduino; load firmware ELF.
- **Arduino Uno:** Arduino Uno → qemu-system-avr -M arduino-uno. ATmega328P Arduino Uno; load firmware ELF.
- **B-L475E-IOT01A:** B-L475E-IOT01A → qemu-system-arm -M b-l475e-iot01a. STM32L4 Discovery IoT node.
- **BBC micro:bit:** BBC micro:bit → qemu-system-arm -M microbit. Load .hex/.elf firmware; not a desktop OS.
- **Cubieboard:** Cubieboard → qemu-system-arm -M cubieboard. Allwinner A10 board.
- **Freescale Sabre Lite:** Freescale Sabre Lite → qemu-system-arm -M sabrelite. i.MX6 Sabre Lite.
- **IBM Power Series 6050/6070:** IBM Power Series 6050/6070 → qemu-system-ppc -M 40p. IBM RS/6000 40p (PREP).
- **Kyoto Microcomputer KZM-ARM11:** Kyoto Microcomputer KZM-ARM11 → qemu-system-arm -M kzm. KZM-ARM11-01 board.
- **Loongson-3 Virt:** Loongson-3 Virt → qemu-system-mips64el -M loongson3-virt. Loongson-3 virtual platform.
- **MIPS Boston:** MIPS Boston → qemu-system-mips64el -M boston. MIPS Boston board.
- **MIPS Magnum:** MIPS Magnum → qemu-system-mips64 -M magnum. MIPS Magnum R4000.
- **MIPS Malta:** MIPS Malta → qemu-system-mips64el -M malta. MIPS Malta development board.
- **MIPS Pica 61:** MIPS Pica 61 → qemu-system-mips64 -M pica61. MIPS Jazz Pica 61.
- **MPC8544DS:** MPC8544DS → qemu-system-ppc -M mpc8544ds. Freescale MPC8544DS.
- **MPS2 AN385:** MPS2 AN385 → qemu-system-arm -M mps2-an385. ARM MPS2 FPGA (AN385).
- **MPS2 AN505:** MPS2 AN505 → qemu-system-arm -M mps2-an505. ARM MPS2 FPGA (AN505).
- **MPS2 AN521:** MPS2 AN521 → qemu-system-arm -M mps2-an521. ARM MPS2 FPGA (AN521).
- **MPS3 AN524:** MPS3 AN524 → qemu-system-arm -M mps3-an524. ARM MPS3 FPGA (AN524).
- **MPS3 AN547:** MPS3 AN547 → qemu-system-arm -M mps3-an547. ARM MPS3 FPGA (AN547).
- **Macintosh Quadra 800:** Macintosh Quadra 800 → qemu-system-m68k -M q800. Macintosh Quadra 800 (m68k).
- **NeXT Cube:** NeXT Cube → qemu-system-m68k -M next-cube. NeXT Cube hardware for NeXTSTEP.
- **Netduino 2:** Netduino 2 → qemu-system-arm -M netduino2. Netduino 2 (STM32).
- **Netduino Plus 2:** Netduino Plus 2 → qemu-system-arm -M netduinoplus2. Netduino Plus 2.
- **Olimex STM32-H405:** Olimex STM32-H405 → qemu-system-arm -M olimex-stm32-h405. Olimex STM32-H405 board.
- **OpenTitan:** OpenTitan → qemu-system-riscv32 -M opentitan. OpenTitan Earl Grey.
- **Orange Pi PC:** Orange Pi PC → qemu-system-arm -M orangepi-pc. Allwinner H3 board.
- **Pegasos I:** Pegasos I → qemu-system-ppc -M pegasos1. Genesi/bPlan Pegasos I.
- **Pegasos II:** Pegasos II → qemu-system-ppc -M pegasos2. Genesi/bPlan Pegasos II.
- **Petalogix ML605:** Petalogix ML605 → qemu-system-microblaze -M petalogix-ml605. Petalogix ML605.
- **PowerMac G3 Beige:** PowerMac G3 Beige → qemu-system-ppc -M g3beige. OldWorld Power Macintosh G3 (Beige).
- **PowerMac Mac99:** PowerMac Mac99 → qemu-system-ppc -M mac99. PowerMac G4-ish mac99 machine.
- **PowerPC e500:** PowerPC e500 → qemu-system-ppc -M ppce500. Freescale e500 platform.
- **RISC-V Spike:** RISC-V Spike → qemu-system-riscv64 -M spike. RISC-V Spike ISA simulator machine.
- **Raspberry Pi 1:** Raspberry Pi 1 → qemu-system-arm -M raspi1ap. SD image required.
- **Raspberry Pi 2:** Raspberry Pi 2 → qemu-system-arm -M raspi2b. SD image required.
- **Raspberry Pi 3:** Raspberry Pi 3 → qemu-system-aarch64 -M raspi3b. SD image; experimental in many builds.
- **Raspberry Pi 3A+:** Raspberry Pi 3A+ → qemu-system-aarch64 -M raspi3ap. SD image; experimental in many builds.
- **Raspberry Pi 4:** Raspberry Pi 4 → qemu-system-aarch64 -M raspi4b. SD image; experimental in many builds.
- **Raspberry Pi Zero:** Raspberry Pi Zero → qemu-system-arm -M raspi0. SD image required; machine support varies by QEMU build.
- **RealView EB:** RealView EB → qemu-system-arm -M realview-eb. ARM RealView Emulation Baseboard.
- **RealView EB MPCore:** RealView EB MPCore → qemu-system-arm -M realview-eb-mpcore. RealView EB with MPCore.
- **RealView PB-A8:** RealView PB-A8 → qemu-system-arm -M realview-pb-a8. RealView Platform Baseboard for Cortex-A8.
- **RealView PBX-A9:** RealView PBX-A9 → qemu-system-arm -M realview-pbx-a9. RealView PBX for Cortex-A9.
- **Renesas R2D:** Renesas R2D → qemu-system-sh4 -M r2d. Renesas SH4 R2D-PLUS.
- **SBSA Reference:** SBSA Reference → qemu-system-aarch64 -M sbsa-ref. ARM SBSA Reference Platform.
- **STM32VLDISCOVERY:** STM32VLDISCOVERY → qemu-system-arm -M stm32vldiscovery. ST STM32VLDISCOVERY board.
- **Sam460ex:** Sam460ex → qemu-system-ppc -M sam460ex. ACube Sam460ex Amiga-compatible.
- **Sharp Zaurus SL-5500:** Sharp Zaurus SL-5500 → qemu-system-arm -M collie. Sharp Zaurus Collie PDA.
- **SiFive E31:** SiFive E31 → qemu-system-riscv32 -M sifive_e. SiFive E31 / HiFive1-class.
- **SiFive HiFive Unleashed:** SiFive HiFive Unleashed → qemu-system-riscv64 -M sifive_u. SiFive U54 / HiFive Unleashed.
- **Siemens SX1:** Siemens SX1 → qemu-system-arm -M sx1. Siemens SX1 smartphone.
- **Siemens SX1 v1:** Siemens SX1 v1 → qemu-system-arm -M sx1-v1. Siemens SX1 (v1).
- **Stellaris LM3S6965:** Stellaris LM3S6965 → qemu-system-arm -M lm3s6965evb. TI Stellaris LM3S6965EVB.
- **Stellaris LM3S811:** Stellaris LM3S811 → qemu-system-arm -M lm3s811evb. TI Stellaris LM3S811EVB.
- **Versatile Express A15:** Versatile Express A15 → qemu-system-arm -M vexpress-a15. ARM Versatile Express with Cortex-A15.
- **Versatile Express A9:** Versatile Express A9 → qemu-system-arm -M vexpress-a9. ARM Versatile Express with Cortex-A9.
- **Xilinx Virtex ML507:** Xilinx Virtex ML507 → qemu-system-ppc -M virtex-ml507. Xilinx Virtex ML507.
- **Xilinx ZCU102:** Xilinx ZCU102 → qemu-system-aarch64 -M xlnx-zcu102. ZynqMP ZCU102 evaluation kit.
- **Xilinx Zynq A9:** Xilinx Zynq A9 → qemu-system-arm -M xilinx-zynq-a9. Xilinx Zynq Platform Baseboard for Cortex-A9.
- **i.MX25 PDK:** i.MX25 PDK → qemu-system-arm -M imx25-pdk. Freescale i.MX25 Product Development Kit.
- **i.MX6UL EVK:** i.MX6UL EVK → qemu-system-arm -M mcimx6ul-evk. NXP i.MX6UL evaluation kit.
- **i.MX7D Sabre:** i.MX7D Sabre → qemu-system-arm -M mcimx7d-sabre. NXP i.MX7D Sabre board.

## More machines

**System / Machine Platform → All QEMU machines…** lists every board your QEMU knows. AQEMU's generated catalog (`docs/qemu_machine_catalog.json`) lists **1,175 machine types across 37 QEMU emulators**. See the [QEMU machine catalog](../../reference/qemu-machine-catalog.md).

---

More: [All guest OS guides](README.md) · [Linux on ARM64](linux.md) · [Linux ARM64 hosts](../hosts/linux-arm64.md) · [Documentation Hub](../../README.md)
