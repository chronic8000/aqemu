# Expand Consoles & Retro in wizard_trees.json from qemu_machine_catalog.json
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
catalog = json.loads((ROOT / "docs/qemu_machine_catalog.json").read_text(encoding="utf-8"))
trees_path = ROOT / "resources/wizard_trees.json"
trees = json.loads(trees_path.read_text(encoding="utf-8"))

# Curated QEMU boards that are console / SBC / classic / hobbyist — not full desktop OS.
# (QEMU does not emulate NES/PS1/arcade ROMs; we only list real -M machines.)
CURATED = {
    # ARM / SBC / MCU
    "microbit": ("BBC micro:bit", "arm", 256, 1, "Load .hex/.elf firmware; not a desktop OS."),
    "raspi0": ("Raspberry Pi Zero", "arm", 512, 8, "SD image required; machine support varies by QEMU build."),
    "raspi1ap": ("Raspberry Pi 1", "arm", 512, 8, "SD image required."),
    "raspi2b": ("Raspberry Pi 2", "arm", 1024, 8, "SD image required."),
    "raspi3ap": ("Raspberry Pi 3A+", "aarch64", 512, 8, "SD image; experimental in many builds."),
    "raspi3b": ("Raspberry Pi 3", "aarch64", 1024, 16, "SD image; experimental in many builds."),
    "raspi4b": ("Raspberry Pi 4", "aarch64", 2048, 16, "SD image; experimental in many builds."),
    "cubieboard": ("Cubieboard", "arm", 1024, 8, "Allwinner A10 board."),
    "orangepi-pc": ("Orange Pi PC", "arm", 1024, 8, "Allwinner H3 board."),
    "versatilepb": ("ARM Versatile PB", "arm", 256, 4, "Classic ARM Versatile Platform Baseboard."),
    "versatileab": ("ARM Versatile AB", "arm", 256, 4, "ARM Versatile Application Baseboard."),
    "integratorcp": ("ARM Integrator/CP", "arm", 256, 4, "ARM Integrator/CP development board."),
    "sabrelite": ("Freescale Sabre Lite", "arm", 1024, 8, "i.MX6 Sabre Lite."),
    "mcimx6ul-evk": ("i.MX6UL EVK", "arm", 512, 4, "NXP i.MX6UL evaluation kit."),
    "mcimx7d-sabre": ("i.MX7D Sabre", "arm", 1024, 8, "NXP i.MX7D Sabre board."),
    "imx25-pdk": ("i.MX25 PDK", "arm", 256, 4, "Freescale i.MX25 Product Development Kit."),
    "kzm": ("Kyoto Microcomputer KZM-ARM11", "arm", 256, 4, "KZM-ARM11-01 board."),
    "realview-eb": ("RealView EB", "arm", 256, 4, "ARM RealView Emulation Baseboard."),
    "realview-eb-mpcore": ("RealView EB MPCore", "arm", 512, 4, "RealView EB with MPCore."),
    "realview-pb-a8": ("RealView PB-A8", "arm", 512, 4, "RealView Platform Baseboard for Cortex-A8."),
    "realview-pbx-a9": ("RealView PBX-A9", "arm", 1024, 8, "RealView PBX for Cortex-A9."),
    "vexpress-a9": ("Versatile Express A9", "arm", 1024, 8, "ARM Versatile Express with Cortex-A9."),
    "vexpress-a15": ("Versatile Express A15", "arm", 1024, 8, "ARM Versatile Express with Cortex-A15."),
    "xilinx-zynq-a9": ("Xilinx Zynq A9", "arm", 1024, 8, "Xilinx Zynq Platform Baseboard for Cortex-A9."),
    "xlnx-zcu102": ("Xilinx ZCU102", "aarch64", 2048, 16, "ZynqMP ZCU102 evaluation kit."),
    "sbsa-ref": ("SBSA Reference", "aarch64", 2048, 16, "ARM SBSA Reference Platform."),
    "virt": ("ARM Virt (generic)", "aarch64", 2048, 16, "Generic ARM virt machine — best for Linux/Android-on-ARM VMs."),
    "sx1": ("Siemens SX1", "arm", 128, 1, "Siemens SX1 smartphone."),
    "sx1-v1": ("Siemens SX1 v1", "arm", 128, 1, "Siemens SX1 (v1)."),
    "collie": ("Sharp Zaurus SL-5500", "arm", 64, 1, "Sharp Zaurus Collie PDA."),
    "b-l475e-iot01a": ("B-L475E-IOT01A", "arm", 128, 1, "STM32L4 Discovery IoT node."),
    "netduino2": ("Netduino 2", "arm", 128, 1, "Netduino 2 (STM32)."),
    "netduinoplus2": ("Netduino Plus 2", "arm", 128, 1, "Netduino Plus 2."),
    "olimex-stm32-h405": ("Olimex STM32-H405", "arm", 128, 1, "Olimex STM32-H405 board."),
    "stm32vldiscovery": ("STM32VLDISCOVERY", "arm", 128, 1, "ST STM32VLDISCOVERY board."),
    "mps2-an385": ("MPS2 AN385", "arm", 256, 1, "ARM MPS2 FPGA (AN385)."),
    "mps2-an505": ("MPS2 AN505", "arm", 256, 1, "ARM MPS2 FPGA (AN505)."),
    "mps2-an521": ("MPS2 AN521", "arm", 256, 1, "ARM MPS2 FPGA (AN521)."),
    "mps3-an524": ("MPS3 AN524", "arm", 256, 1, "ARM MPS3 FPGA (AN524)."),
    "mps3-an547": ("MPS3 AN547", "arm", 256, 1, "ARM MPS3 FPGA (AN547)."),
    "lm3s6965evb": ("Stellaris LM3S6965", "arm", 64, 1, "TI Stellaris LM3S6965EVB."),
    "lm3s811evb": ("Stellaris LM3S811", "arm", 64, 1, "TI Stellaris LM3S811EVB."),
    # AVR / Arduino
    "arduino-duemilanove": ("Arduino Duemilanove", "avr", 2, 1, "ATmega168 Arduino; load firmware ELF."),
    "arduino-mega": ("Arduino Mega", "avr", 8, 1, "ATmega2560 Arduino; load firmware ELF."),
    "arduino-uno": ("Arduino Uno", "avr", 2, 1, "ATmega328P Arduino Uno; load firmware ELF."),
    # Classic Mac / Amiga / NeXT / PowerPC
    "q800": ("Macintosh Quadra 800", "m68k", 128, 4, "Macintosh Quadra 800 (m68k)."),
    "next-cube": ("NeXT Cube", "m68k", 128, 4, "NeXT Cube hardware for NeXTSTEP."),
    "amigaone": ("AmigaOne", "ppc", 512, 8, "AmigaOne PowerPC board."),
    "pegasos1": ("Pegasos I", "ppc", 512, 8, "Genesi/bPlan Pegasos I."),
    "pegasos2": ("Pegasos II", "ppc", 512, 8, "Genesi/bPlan Pegasos II."),
    "mac99": ("PowerMac Mac99", "ppc", 512, 8, "PowerMac G4-ish mac99 machine."),
    "g3beige": ("PowerMac G3 Beige", "ppc", 256, 4, "OldWorld Power Macintosh G3 (Beige)."),
    "40p": ("IBM Power Series 6050/6070", "ppc", 256, 4, "IBM RS/6000 40p (PREP)."),
    "ppce500": ("PowerPC e500", "ppc", 256, 4, "Freescale e500 platform."),
    "mpc8544ds": ("MPC8544DS", "ppc", 256, 4, "Freescale MPC8544DS."),
    "bamboo": ("440 Bamboo", "ppc", 256, 4, "PowerPC 440 Bamboo board."),
    "sam460ex": ("Sam460ex", "ppc", 512, 8, "ACube Sam460ex Amiga-compatible."),
    "virtex-ml507": ("Xilinx Virtex ML507", "ppc", 256, 4, "Xilinx Virtex ML507."),
    "petalogix-ml605": ("Petalogix ML605", "ppc", 256, 4, "Petalogix ML605."),
    # MIPS classic
    "malta": ("MIPS Malta", "mips64el", 512, 8, "MIPS Malta development board."),
    "mipssim": ("MIPS Simulator", "mips", 256, 4, "MIPS MIPSsim."),
    "boston": ("MIPS Boston", "mips64el", 1024, 8, "MIPS Boston board."),
    "magnum": ("MIPS Magnum", "mips", 256, 4, "MIPS Magnum R4000."),
    "pica61": ("MIPS Pica 61", "mips", 256, 4, "MIPS Jazz Pica 61."),
    "loongson3-virt": ("Loongson-3 Virt", "mips64el", 1024, 16, "Loongson-3 virtual platform."),
    # SH4 / other MCU-ish
    "r2d": ("Renesas R2D", "sh4", 128, 2, "Renesas SH4 R2D-PLUS."),
    # RISC-V boards (hobbyist / SBC class)
    "sifive_u": ("SiFive HiFive Unleashed", "riscv64", 1024, 8, "SiFive U54 / HiFive Unleashed."),
    "sifive_e": ("SiFive E31", "riscv32", 128, 1, "SiFive E31 / HiFive1-class."),
    "spike": ("RISC-V Spike", "riscv64", 1024, 8, "RISC-V Spike ISA simulator machine."),
    "opentitan": ("OpenTitan", "riscv32", 256, 1, "OpenTitan Earl Grey."),
}

# Index available machines by name -> best target
available = {}
for b in catalog["binaries"]:
    for m in b["machines"]:
        available.setdefault(m["name"], b["target"])

# Prefer certain targets when aliases collide
prefer_target = {
    "virt": "aarch64",
    "malta": "mips64el",
    "spike": "riscv64",
}

entries = []
profiles = {}
bindings = {}
seen_display = set()

for machine, (display, default_target, ram, hdd, tip) in sorted(CURATED.items(), key=lambda x: x[1][0].lower()):
    if machine not in available:
        # try alias without version noise
        continue
    target = prefer_target.get(machine, available[machine])
    # If curated default target exists for this machine name on that binary, prefer it
    for b in catalog["binaries"]:
        if b["target"] == default_target and any(m["name"] == machine for m in b["machines"]):
            target = default_target
            break
    if display in seen_display:
        # keep first
        continue
    seen_display.add(display)
    entries.append(display)
    profiles[display] = {
        "target": target,
        "machine": machine,
        "ram_mb": ram,
        "hdd_gb": hdd,
        "nic": "none",
        "sound": "none",
        "tip": f"{display} → qemu-system-{target} -M {machine}. {tip}",
        "flags": ["console_retro"],
    }
    bindings[display] = {"target": target, "machine": machine}

entries = sorted(entries)

trees["operating_systems"]["Consoles & Retro"] = entries
trees["platforms"]["Consoles & Retro"] = entries
# merge platform_bindings
pb = trees.setdefault("platform_bindings", {})
# remove old console bindings that we no longer list
# keep non-console bindings; replace console ones
for k in list(pb.keys()):
    if k in bindings or (isinstance(pb[k], dict) and pb[k].get("machine") in CURATED):
        # drop if not in new set and was console
        pass
for disp, bind in bindings.items():
    pb[disp] = bind

# profiles
op = trees.setdefault("os_profiles", {})
# remove previous console_retro flagged or old console names
for k in list(op.keys()):
    flags = op[k].get("flags") or []
    if "console_retro" in flags or k in bindings:
        # will rewrite
        if k not in profiles:
            # keep if still referenced elsewhere? drop console_retro only
            if "console_retro" in flags:
                del op[k]
for disp, prof in profiles.items():
    op[disp] = prof

# architecture_targets: ensure targets exist (already should)
trees_path.write_text(json.dumps(trees, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
print(f"Consoles & Retro: {len(entries)} entries")
for e in entries:
    print(" -", e, "->", profiles[e]["target"], profiles[e]["machine"])
