#!/usr/bin/env python3
"""
Probe every qemu-system-* binary for -machine help and build a human-readable
catalog for AQEMU's New VM Wizard.

Wizard lists show friendly names only (e.g. "Raspberry Pi 3", "PC (Q35)").
QEMU machine IDs (raspi3b, q35) are stored as backend mappings, not UI labels.

Usage:
  python scripts/probe_qemu_machines.py --qemu-dir "C:/Program Files/qemu" --cpus
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple


# ---------------------------------------------------------------------------
# Curated wizard UX lists (human-readable only)
# ---------------------------------------------------------------------------

CURATED_OPERATING_SYSTEMS = {
    'Apple': [
        'Darwin', 'Mac OS 6', 'Mac OS 7', 'Mac OS 8',
        'Mac OS 9', 'Mac OS X Intel', 'Mac OS X PPC', 'macOS',
        'NeXTSTEP', 'OPENSTEP',
    ],
    'BSD': [
        'DragonFly BSD', 'FreeBSD', 'GhostBSD', 'NetBSD',
        'OpenBSD',
    ],
    'DEC': [
        'OpenVMS', 'Tru64 UNIX',
    ],
    'HP': [
        'HP-UX',
    ],
    'IBM': [
        'AIX', 'ArcaOS', 'eComStation', 'Linux on IBM Z',
        'OS/2',
    ],
    'Linux': [
        'AlmaLinux', 'Alpine Linux', 'Arch Linux', 'CentOS Stream',
        'Debian', 'elementary OS', 'Fedora', 'Generic Linux',
        'Gentoo', 'Kali Linux', 'Linux Mint', 'NixOS',
        'openSUSE', 'Pop!_OS', 'RHEL', 'Rocky Linux',
        'Slackware', 'SUSE Linux', 'Tiny Core Linux', 'Ubuntu',
        'Void Linux',
    ],
    'Microsoft': [
        'DR-DOS', 'FreeDOS', 'MS-DOS', 'PC DOS',
        'Windows 1.x', 'Windows 2.x', 'Windows 3.x', 'Windows 95',
        'Windows 98', 'Windows ME', 'Windows NT 3.x', 'Windows NT 4.0',
        'Windows 2000', 'Windows XP', 'Windows Vista', 'Windows 7',
        'Windows 8', 'Windows 8.1', 'Windows 10', 'Windows 11',
        'Windows Server 2000', 'Windows Server 2003', 'Windows Server 2008', 'Windows Server 2012',
        'Windows Server 2016', 'Windows Server 2019', 'Windows Server 2022', 'Windows Server 2025',
    ],
    'Other': [
        '9front', 'AmigaOS', 'Android', 'BeOS',
        'Chrome OS Flex', 'ChromeOS', 'Fuchsia', 'Haiku',
        'KolibriOS', 'MenuetOS', 'Minix', 'MorphOS',
        'Other', 'Plan 9', 'QNX', 'ReactOS',
        'Redox OS', 'RISC OS', 'SerenityOS', 'SteamOS',
        'TempleOS',
    ],
    'SCO': [
        'OpenServer', 'UnixWare',
    ],
    'SGI': [
        'IRIX 5.x', 'IRIX 6.x',
    ],
    'Sun': [
        'illumos', 'OmniOS', 'OpenSolaris', 'Solaris SPARC',
        'Solaris x86', 'Solaris x86 (32-bit)',
    ],
}

CURATED_ARCHITECTURES = {
    "Mainstream": ["x86", "x86-64", "ARM", "ARM64 / AArch64", "RISC-V 32", "RISC-V 64"],
    "PowerPC": ["PowerPC 32", "PowerPC 64", "PowerPC 64 LE"],
    "MIPS": ["MIPS", "MIPS64", "MIPSEL", "MIPS64EL"],
    "SPARC": ["SPARC32", "SPARC64"],
    "IBM": ["s390x"],
    "DEC": ["Alpha"],
    "HP": ["PA-RISC"],
    "Motorola": ["m68k"],
    "Embedded": [
        "AVR", "MicroBlaze", "OpenRISC", "Xtensa", "RX", "TriCore", "SH4", "LoongArch",
    ],
}

ARCH_BINARY_MAP = {
    "i386": "x86",
    "x86_64": "x86-64",
    "arm": "ARM",
    "aarch64": "ARM64 / AArch64",
    "riscv32": "RISC-V 32",
    "riscv64": "RISC-V 64",
    "ppc": "PowerPC 32",
    "ppc64": "PowerPC 64",
    "mips": "MIPS",
    "mips64": "MIPS64",
    "mipsel": "MIPSEL",
    "mips64el": "MIPS64EL",
    "sparc": "SPARC32",
    "sparc64": "SPARC64",
    "s390x": "s390x",
    "alpha": "Alpha",
    "hppa": "PA-RISC",
    "m68k": "m68k",
    "avr": "AVR",
    "microblaze": "MicroBlaze",
    "or1k": "OpenRISC",
    "xtensa": "Xtensa",
    "xtensaeb": "Xtensa",
    "rx": "RX",
    "tricore": "TriCore",
    "sh4": "SH4",
    "sh4eb": "SH4",
    "loongarch64": "LoongArch",
}

# Explicit QEMU id → friendly display name (overrides description when set).
FRIENDLY_ALIASES: Dict[str, str] = {
    # PC
    "pc": "PC (i440FX)",
    "q35": "PC (Q35)",
    "isapc": "Legacy ISA PC",
    "microvm": "MicroVM",
    # ARM
    "virt": "Generic Virtual Machine (virt)",
    "sbsa-ref": "SBSA Reference Platform",
    "raspi0": "Raspberry Pi Zero",
    "raspi1ap": "Raspberry Pi 1",
    "raspi2b": "Raspberry Pi 2",
    "raspi3ap": "Raspberry Pi 3A+",
    "raspi3b": "Raspberry Pi 3",
    "raspi4b": "Raspberry Pi 4",
    # Apple
    "mac99": "Macintosh (Mac99)",
    "g3beige": "Power Macintosh G3 (Beige)",
    "prep": "PowerPC PREP",
    "q800": "Macintosh Quadra 800",
    # Sun
    "sun4m": "Sun SPARCstation (sun4m)",
    "sun4u": "Sun UltraSPARC (sun4u)",
    "sun4v": "Sun Niagara (sun4v)",
    "niagara": "Sun Niagara",
    "niagara-lc": "Sun Niagara LC",
    # SGI / MIPS
    "indy": "SGI Indy",
    "magnum": "SGI Magnum",
    "malta": "MIPS Malta",
    "mipssim": "MIPS Simulator",
    "fuloong2e": "Fuloong 2E",
    "loongson3-virt": "Loongson 3 Virtual",
    # DEC Alpha (names vary by QEMU version)
    "clipper": "Alpha Clipper",
    "sx164": "Alpha SX164",
    "lx164": "Alpha LX164",
    "dp264": "Alpha DP264",
    # HP
    "hppa": "HP PA-RISC",
    "b160l": "HP PA-RISC B160L",
    "c3700": "HP PA-RISC C3700",
    # Power
    "pseries": "IBM pSeries",
    "powernv": "IBM PowerNV",
    "powernv8": "IBM PowerNV POWER8",
    "powernv9": "IBM PowerNV POWER9",
    "powernv10": "IBM PowerNV POWER10",
    # s390
    "s390-ccw-virtio": "IBM Z (s390-ccw-virtio)",
    "s390-ccw": "IBM Z (s390-ccw)",
    # RISC-V
    "spike": "RISC-V Spike",
    "sifive_e": "SiFive E Series",
    "sifive_u": "SiFive U Series",
    "opentitan": "OpenTitan",
    # Embedded samples
    "arduino-duemilanove": "Arduino Duemilanove",
    "arduino-mega": "Arduino Mega",
    "arduino-mega-2560-v3": "Arduino Mega 2560",
    "netduino2": "Netduino 2",
    "netduinoplus2": "Netduino Plus 2",
    "stm32vldiscovery": "STM32 VL Discovery",
    "none": "Empty Machine (none)",
}

# Curated List 3 for the wizard — display only. Internal qemu ids / targets for binding.
# match: list of (target_regex, machine_id_regex) tried in order against probed data.
WIZARD_PLATFORMS: List[Dict] = [
    {"group": "IBM PC Compatible", "display": "PC (i440FX)", "match": [("x86_64|i386", r"^pc$")]},
    {"group": "IBM PC Compatible", "display": "PC (Q35)", "match": [("x86_64|i386", r"^q35$")]},
    {"group": "IBM PC Compatible", "display": "Modern UEFI PC", "match": [("x86_64", r"^q35$")], "notes": "Uses Q35 + UEFI firmware profile"},
    {"group": "IBM PC Compatible", "display": "Legacy BIOS PC", "match": [("i386|x86_64", r"^pc$")], "notes": "Uses classic i440FX + SeaBIOS"},
    {"group": "IBM PC Compatible", "display": "Legacy ISA PC", "match": [("i386|x86_64", r"^isapc$")]},
    {"group": "ARM Virtual Platforms", "display": "ARM Virt", "match": [("aarch64|arm", r"^virt$")]},
    {"group": "ARM Virtual Platforms", "display": "SBSA Reference Platform", "match": [("aarch64", r"^sbsa-ref$")]},
    {"group": "ARM Virtual Platforms", "display": "Generic ARM Board", "match": [("arm", r"^virt$")]},
    {"group": "Raspberry Pi", "display": "Raspberry Pi 1", "match": [("arm|aarch64", r"^raspi1")]},
    {"group": "Raspberry Pi", "display": "Raspberry Pi 2", "match": [("arm|aarch64", r"^raspi2")]},
    {"group": "Raspberry Pi", "display": "Raspberry Pi 3", "match": [("arm|aarch64", r"^raspi3")]},
    {"group": "Raspberry Pi", "display": "Raspberry Pi 4", "match": [("arm|aarch64", r"^raspi4")]},
    {"group": "Apple Macintosh", "display": "Power Macintosh G3", "match": [("ppc|ppc64", r"^g3beige$")]},
    {"group": "Apple Macintosh", "display": "Macintosh (Mac99)", "match": [("ppc|ppc64", r"^mac99$")]},
    {"group": "Apple Macintosh", "display": "Macintosh Quadra 800", "match": [("m68k", r"^q800$")]},
    {"group": "Sun Microsystems", "display": "SPARCstation (sun4m)", "match": [("sparc", r"^sun4m$")]},
    {"group": "Sun Microsystems", "display": "UltraSPARC (sun4u)", "match": [("sparc64", r"^sun4u$")]},
    {"group": "Sun Microsystems", "display": "Sun Niagara", "match": [("sparc64", r"^niagara")]},
    {"group": "Silicon Graphics", "display": "SGI Indy", "match": [("mips|mipsel|mips64|mips64el", r"^indy$")]},
    {"group": "Silicon Graphics", "display": "SGI Magnum", "match": [("mips|mipsel|mips64|mips64el", r"^magnum$")]},
    {"group": "DEC", "display": "Alpha Clipper", "match": [("alpha", r"^clipper$")]},
    {"group": "DEC", "display": "Alpha SX164", "match": [("alpha", r"^sx164$")]},
    {"group": "DEC", "display": "Alpha LX164", "match": [("alpha", r"^lx164$")]},
    {"group": "DEC", "display": "Alpha DP264", "match": [("alpha", r"^dp264$")]},
    {"group": "HP", "display": "HP PA-RISC B160L", "match": [("hppa", r"^b160l$|^hppa$")]},
    {"group": "HP", "display": "HP PA-RISC C3700", "match": [("hppa", r"^c3700$")]},
    {"group": "IBM Power", "display": "IBM pSeries", "match": [("ppc64|ppc", r"^pseries$")]},
    {"group": "IBM Power", "display": "IBM PowerNV", "match": [("ppc64|ppc", r"^powernv$")]},
    {"group": "IBM Power", "display": "POWER8 (PowerNV)", "match": [("ppc64|ppc", r"^powernv8$")]},
    {"group": "IBM Power", "display": "POWER9 (PowerNV)", "match": [("ppc64|ppc", r"^powernv9$")]},
    {"group": "IBM Power", "display": "POWER10 (PowerNV)", "match": [("ppc64|ppc", r"^powernv10$")]},
    {"group": "IBM Mainframe", "display": "IBM Z (s390-ccw-virtio)", "match": [("s390x", r"^s390-ccw-virtio$")]},
    {"group": "MIPS", "display": "MIPS Malta", "match": [("mips|mipsel|mips64|mips64el", r"^malta$")]},
    {"group": "MIPS", "display": "Fuloong 2E", "match": [("mips|mipsel|mips64|mips64el", r"^fuloong")]},
    {"group": "MIPS", "display": "Loongson 3 Virtual", "match": [("mips|mips64|loongarch64", r"^loongson")]},
    {"group": "RISC-V", "display": "RISC-V Virt", "match": [("riscv64|riscv32", r"^virt$")]},
    {"group": "RISC-V", "display": "RISC-V Spike", "match": [("riscv64|riscv32", r"^spike$")]},
    {"group": "RISC-V", "display": "SiFive E Series", "match": [("riscv64|riscv32", r"^sifive_e")]},
    {"group": "RISC-V", "display": "SiFive U Series", "match": [("riscv64|riscv32", r"^sifive_u")]},
    {"group": "RISC-V", "display": "OpenTitan", "match": [("riscv32|riscv64", r"^opentitan$")]},
    {"group": "Embedded", "display": "Arduino Mega", "match": [("avr", r"^arduino-mega")]},
    {"group": "Embedded", "display": "Arduino Duemilanove", "match": [("avr", r"^arduino-duemilanove$")]},
    {"group": "Embedded", "display": "STM32 VL Discovery", "match": [("arm", r"^stm32vldiscovery$")]},
    {"group": "Embedded", "display": "Netduino 2", "match": [("arm", r"^netduino2$")]},
    {"group": "Generic", "display": "Generic Virtual Machine", "match": [(".*", r"^virt$")], "notes": "Prefer aarch64/riscv virt when multiple"},
    {"group": "Generic", "display": "Empty Machine", "match": [(".*", r"^none$")]},
    {"group": "Generic", "display": "Custom Machine", "match": [], "notes": "User picks any probed machine"},
    {"group": "Generic", "display": "Import Existing QEMU Command Line", "match": [], "notes": "Paste -M / full argv later"},
]


def find_qemu_binaries(qemu_dir: Optional[Path]) -> List[Path]:
    search_dirs: List[Path] = []
    if qemu_dir:
        search_dirs.append(qemu_dir)
    else:
        env = os.environ.get("QEMU_DIR") or os.environ.get("QEMU_HOME")
        if env:
            search_dirs.append(Path(env))
        search_dirs.extend([
            Path(r"C:\Program Files\qemu"),
            Path(r"C:\Program Files (x86)\qemu"),
            Path("/usr/bin"),
            Path("/usr/local/bin"),
            Path("/opt/homebrew/bin"),
        ])
        for part in os.environ.get("PATH", "").split(os.pathsep):
            if part:
                search_dirs.append(Path(part))

    seen: Set = set()
    out: List[Path] = []
    for d in search_dirs:
        if not d.is_dir():
            continue
        pat = "qemu-system-*.exe" if os.name == "nt" else "qemu-system-*"
        for p in sorted(d.glob(pat)):
            name = p.name.lower()
            if name.endswith("w.exe"):
                continue
            if not name.startswith("qemu-system-"):
                continue
            key = p.resolve()
            if key in seen:
                continue
            if os.name != "nt" and not os.access(p, os.X_OK):
                continue
            seen.add(key)
            out.append(p)
    return out


def target_from_binary(path: Path) -> str:
    name = path.stem
    prefix = "qemu-system-"
    return name[len(prefix):] if name.startswith(prefix) else name


def run_help(binary: Path, args: List[str], timeout: float = 20.0) -> Tuple[int, str]:
    try:
        proc = subprocess.run(
            [str(binary)] + args,
            capture_output=True, text=True, encoding="utf-8", errors="replace",
            timeout=timeout, check=False,
        )
        out = (proc.stdout or "") + (("\n" + proc.stderr) if proc.stderr else "")
        return proc.returncode, out
    except subprocess.TimeoutExpired:
        return -1, f"[timeout] {binary} {' '.join(args)}"
    except OSError as e:
        return -2, str(e)


def parse_machine_help(text: str) -> List[Dict[str, str]]:
    machines: List[Dict[str, str]] = []
    started = False
    for raw in text.splitlines():
        line = raw.rstrip()
        if not started:
            if "Supported machines" in line or "Supported machine" in line:
                started = True
            continue
        if not line.strip():
            if machines:
                break
            continue
        if "driver" in line.lower() and line.lstrip().lower().startswith("machine"):
            continue
        m = re.match(r"^([A-Za-z0-9_.:\-]+)\s{2,}(.+)$", line)
        if m:
            name, desc = m.group(1), m.group(2).strip()
            if name.lower() not in {"type", "machine"}:
                machines.append({"name": name, "description": desc})
            continue
        m2 = re.match(r"^([A-Za-z0-9_.:\-]+)\s*$", line)
        if m2 and m2.group(1).lower() not in {"type", "machine"}:
            machines.append({"name": m2.group(1), "description": ""})
    return machines


def parse_cpu_help(text: str) -> List[str]:
    cpus: List[str] = []
    seen: Set[str] = set()
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.lower().startswith("available") or line.lower().startswith("cpu"):
            continue
        token = line.split()[0]
        if re.match(r"^[A-Za-z0-9_.:\-]+$", token) and token not in seen:
            seen.add(token)
            cpus.append(token)
    return cpus


def is_versioned_alias(name: str) -> bool:
    """Hide pc-i440fx-2.12 / virt-8.2 style noise from wizard lists."""
    if re.search(r"-\d+\.\d+", name):
        return True
    if re.search(r"-\d+$", name) and name.split("-")[0] in {"virt", "pseries", "powernv"}:
        # keep powernv8/9/10 — those are product names not version aliases
        if re.match(r"^powernv\d+$", name):
            return False
        return True
    return False


def strip_alias_noise(desc: str) -> str:
    desc = re.sub(r"\s*\(alias of [^)]+\)\s*", "", desc, flags=re.I).strip()
    return desc


def humanize_id(name: str) -> str:
    parts = re.split(r"[-_]", name)
    pretty = []
    for p in parts:
        if not p:
            continue
        if p.lower() in {"pc", "vm", "cpu", "bmc", "evb", "soc"}:
            pretty.append(p.upper())
        elif p.isdigit() or re.match(r"^\d", p):
            pretty.append(p)
        else:
            pretty.append(p[:1].upper() + p[1:])
    return " ".join(pretty)


def display_name_for(qemu_name: str, description: str, target: str) -> str:
    if qemu_name in FRIENDLY_ALIASES:
        label = FRIENDLY_ALIASES[qemu_name]
        # Disambiguate shared 'virt' by architecture
        if qemu_name == "virt":
            arch = ARCH_BINARY_MAP.get(target, target)
            return f"{arch} Virtual Machine"
        return label

    desc = strip_alias_noise(description or "")
    # Prefer a clean QEMU description over the raw id
    if desc and len(desc) >= 3 and not desc.lower().startswith("alias"):
        # Trim overly long versioned parentheticals already stripped
        return desc

    return humanize_id(qemu_name)


def platform_group(qemu_name: str, target: str, display: str) -> str:
    n = qemu_name.lower()
    if n in {"pc", "q35", "isapc", "microvm"} or n.startswith("pc-"):
        return "IBM PC Compatible"
    if n.startswith("raspi") or n.startswith("rpi"):
        return "Raspberry Pi"
    if n in {"mac99", "g3beige", "q800", "prep"}:
        return "Apple Macintosh"
    if n.startswith("sun4") or n.startswith("niagara"):
        return "Sun Microsystems"
    if n in {"indy", "magnum"}:
        return "Silicon Graphics"
    if target == "alpha" or n in {"clipper", "sx164", "lx164", "dp264"}:
        return "DEC"
    if target == "hppa" or n in {"b160l", "c3700", "hppa"}:
        return "HP"
    if n.startswith("pseries") or n.startswith("powernv"):
        return "IBM Power"
    if n.startswith("s390"):
        return "IBM Mainframe"
    if n in {"malta", "mipssim"} or n.startswith("fuloong") or n.startswith("loongson"):
        return "MIPS"
    if target.startswith("riscv") and (n == "virt" or n.startswith("spike") or n.startswith("sifive") or n == "opentitan"):
        return "RISC-V"
    if target in {"arm", "aarch64"} and (n == "virt" or n == "sbsa-ref"):
        return "ARM Virtual Platforms"
    if n.startswith("arduino") or "stm32" in n or n.startswith("netduino"):
        return "Embedded"
    if "xbox" in n or "ps2" in n or "dreamcast" in n or "dc" == n:
        return "Game Consoles"
    return "Other Platforms"


def resolve_wizard_platforms(probed: List[Dict]) -> List[Dict]:
    """Attach available qemu bindings to curated friendly platform rows."""
    index: List[Tuple[str, str]] = []  # target, qemu_name
    for b in probed:
        for m in b.get("machines", []):
            index.append((b["target"], m["name"]))

    # Prefer mainstream targets when several binaries share a machine name
    target_priority = {
        "x86_64": 0, "aarch64": 1, "ppc64": 2, "riscv64": 3, "sparc64": 4,
        "mips64el": 5, "i386": 10, "arm": 11, "ppc": 12, "riscv32": 13,
    }

    def best(cands: List[Tuple[str, str]]) -> Optional[Tuple[str, str]]:
        if not cands:
            return None
        return sorted(cands, key=lambda t: (target_priority.get(t[0], 50), t[0], t[1]))[0]

    resolved = []
    for preset in WIZARD_PLATFORMS:
        entry = {
            "group": preset["group"],
            "display": preset["display"],
            "notes": preset.get("notes", ""),
            "available": False,
            "qemu_target": "",
            "qemu_machine": "",
        }
        cands: List[Tuple[str, str]] = []
        for target_re, machine_re in preset.get("match", []):
            for target, qname in index:
                if re.search(target_re, target) and re.search(machine_re, qname):
                    cands.append((target, qname))
        # Prefer canonical board names over "ap" variants (raspi3b over raspi3ap)
        prefer = {
            "Raspberry Pi 3": ("raspi3b", "raspi3ap"),
            "Raspberry Pi 4": ("raspi4b",),
            "Raspberry Pi 2": ("raspi2b",),
            "PC (Q35)": ("q35",),
            "PC (i440FX)": ("pc",),
        }
        hit = None
        if preset["display"] in prefer:
            wanted = prefer[preset["display"]]
            for w in wanted:
                subset = [c for c in cands if c[1] == w]
                hit = best(subset)
                if hit:
                    break
        if hit is None:
            hit = best(cands)
        if hit:
            entry["available"] = True
            entry["qemu_target"], entry["qemu_machine"] = hit
        if preset["display"] == "Generic Virtual Machine":
            for target, qname in index:
                if qname == "virt" and target == "aarch64":
                    entry.update(available=True, qemu_target=target, qemu_machine=qname)
                    break
        resolved.append(entry)
    return resolved


def build_markdown(report: Dict) -> str:
    lines: List[str] = []
    lines += [
        "# AQEMU New VM Wizard — Human-Readable Catalog",
        "",
        f"Generated: `{report['generated_utc']}`  ",
        f"QEMU: `{report.get('qemu_dir', '')}`  ",
        f"System binaries found: **{report['binary_count']}**  ",
        f"Machine definitions probed: **{report['machine_count']}** (internal)",
        "",
        "UI lists use **friendly names only**. QEMU IDs are backend mappings.",
        "",
        "Regenerate: `python scripts/probe_qemu_machines.py --qemu-dir \"C:/Program Files/qemu\"`",
        "",
        "---",
        "",
        "## How the three lists work",
        "",
        "| Wizard path | What the user sees | What AQEMU stores |",
        "|-------------|--------------------|-------------------|",
        "| Operating System | Windows 11, Ubuntu, IRIX… | Profile → binary + machine + devices |",
        "| CPU Architecture | x86-64, ARM64, SPARC64… | `qemu-system-*` target |",
        "| System / Machine | Raspberry Pi 3, SGI Indy, PC (Q35)… | `qemu_machine` id for that target |",
        "| Custom | Advanced picker | User-chosen binary + machine |",
        "",
        "---",
        "",
        "## List 1 — Guest Operating Systems",
        "",
    ]
    for family, items in CURATED_OPERATING_SYSTEMS.items():
        lines.append(f"### {family}")
        lines.append("")
        for it in items:
            lines.append(f"- {it}")
        lines.append("")

    lines += ["---", "", "## List 2 — CPU Architectures", ""]
    for family, items in CURATED_ARCHITECTURES.items():
        lines.append(f"### {family}")
        lines.append("")
        for it in items:
            # availability
            targets = [t for t, label in ARCH_BINARY_MAP.items() if label == it or label.startswith(it)]
            if it == "x86":
                targets = ["i386"]
            elif it == "x86-64":
                targets = ["x86_64"]
            elif it == "ARM":
                targets = ["arm"]
            elif it == "ARM64 / AArch64":
                targets = ["aarch64"]
            elif it == "RISC-V 32":
                targets = ["riscv32"]
            elif it == "RISC-V 64":
                targets = ["riscv64"]
            elif it == "PA-RISC":
                targets = ["hppa"]
            elif it == "LoongArch":
                targets = ["loongarch64"]
            installed = {b["target"] for b in report["binaries"]}
            ok = any(t in installed for t in targets) if targets else False
            mark = "✓ installed" if ok else "○ not in this QEMU build"
            lines.append(f"- **{it}** — {mark}")
        lines.append("")

    lines += [
        "---",
        "",
        "## List 3 — System / Machine Platforms (wizard)",
        "",
        "Friendly names for the UI. Rows marked **available** match your installed QEMU.",
        "",
    ]

    by_group: Dict[str, List[Dict]] = {}
    for row in report.get("wizard_platforms", []):
        by_group.setdefault(row["group"], []).append(row)

    for group, rows in by_group.items():
        lines.append(f"### {group}")
        lines.append("")
        for row in rows:
            if row.get("available"):
                lines.append(
                    f"- **{row['display']}** — available "
                    f"(`qemu-system-{row['qemu_target']}` → `{row['qemu_machine']}`)"
                )
            else:
                note = f" — {row['notes']}" if row.get("notes") else ""
                if row["display"] in {"Custom Machine", "Import Existing QEMU Command Line"}:
                    lines.append(f"- **{row['display']}**{note}")
                else:
                    lines.append(f"- **{row['display']}** — not present in this QEMU install{note}")
        lines.append("")

    lines += [
        "---",
        "",
        "## All platforms on this PC (readable names)",
        "",
        "Every non-versioned machine, shown by friendly name. Obscure `virt-9.2` aliases are hidden.",
        "",
    ]

    grouped: Dict[str, List[Dict]] = {}
    for b in report["binaries"]:
        for m in b.get("machines", []):
            if m.get("hidden"):
                continue
            g = m.get("group", "Other Platforms")
            grouped.setdefault(g, []).append({
                "display": m["display_name"],
                "target": b["target"],
                "arch": b.get("arch_label", b["target"]),
                "qemu": m["name"],
            })

    for group in sorted(grouped.keys()):
        lines.append(f"### {group}")
        lines.append("")
        # unique by display+arch
        seen: Set[Tuple[str, str]] = set()
        rows = sorted(grouped[group], key=lambda r: (r["display"].lower(), r["arch"]))
        for r in rows:
            key = (r["display"], r["arch"])
            if key in seen:
                continue
            seen.add(key)
            lines.append(f"- **{r['display']}** ({r['arch']})")
        lines.append("")

    lines += [
        "---",
        "",
        "## Developer appendix — raw QEMU IDs",
        "",
        "Not for the wizard UI. Use only when debugging mappings.",
        "",
    ]
    for b in report["binaries"]:
        lines.append(f"### qemu-system-{b['target']}")
        lines.append("")
        shown = [m for m in b.get("machines", []) if not m.get("hidden")]
        for m in shown[:30]:
            lines.append(f"- `{m['name']}` → {m['display_name']}")
        more = len(shown) - 30
        if more > 0:
            lines.append(f"- …and {more} more (see JSON)")
        lines.append("")

    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--qemu-dir", type=Path, default=None)
    ap.add_argument("--cpus", action="store_true")
    ap.add_argument("--out-md", type=Path, default=None)
    ap.add_argument("--out-json", type=Path, default=None)
    ap.add_argument("--timeout", type=float, default=25.0)
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[1]
    out_md = args.out_md or (repo / "docs" / "QEMU_MACHINE_CATALOG.md")
    out_json = args.out_json or (repo / "docs" / "qemu_machine_catalog.json")
    out_md.parent.mkdir(parents=True, exist_ok=True)

    binaries = find_qemu_binaries(args.qemu_dir)
    if not binaries:
        print("No qemu-system-* found. Pass --qemu-dir.", file=sys.stderr)
        return 1

    report: Dict = {
        "generated_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "host": f"{os.name} {sys.platform}",
        "qemu_dir": str(args.qemu_dir or binaries[0].parent),
        "binary_count": 0,
        "machine_count": 0,
        "curated_operating_systems": CURATED_OPERATING_SYSTEMS,
        "curated_architectures": CURATED_ARCHITECTURES,
        "binaries": [],
        "wizard_platforms": [],
    }

    print(f"Found {len(binaries)} qemu-system binaries")
    for i, binary in enumerate(binaries, 1):
        target = target_from_binary(binary)
        print(f"[{i}/{len(binaries)}] {binary.name}", flush=True)
        info: Dict = {
            "target": target,
            "path": str(binary),
            "arch_label": ARCH_BINARY_MAP.get(target, target),
            "machines": [],
            "cpus": [],
            "version_line": "",
        }
        _rc, ver = run_help(binary, ["--version"], timeout=min(10.0, args.timeout))
        if ver.strip():
            info["version_line"] = ver.strip().splitlines()[0].strip()

        _rc, text = run_help(binary, ["-machine", "help"], timeout=args.timeout)
        for m in parse_machine_help(text):
            hidden = is_versioned_alias(m["name"])
            disp = display_name_for(m["name"], m.get("description", ""), target)
            entry = {
                "name": m["name"],
                "description": m.get("description", ""),
                "display_name": disp,
                "group": platform_group(m["name"], target, disp),
                "hidden": hidden,
            }
            info["machines"].append(entry)
            report["machine_count"] += 1

        if args.cpus:
            _rc, ctext = run_help(binary, ["-cpu", "help"], timeout=args.timeout)
            info["cpus"] = parse_cpu_help(ctext)

        report["binaries"].append(info)

    report["binary_count"] = len(report["binaries"])
    report["wizard_platforms"] = resolve_wizard_platforms(report["binaries"])

    out_json.write_text(json.dumps(report, indent=2), encoding="utf-8")
    out_md.write_text(build_markdown(report), encoding="utf-8")
    print(f"Wrote {out_md}")
    print(f"Wrote {out_json}")

    avail = sum(1 for p in report["wizard_platforms"] if p.get("available"))
    print(f"Wizard platforms available on this install: {avail}/{len(report['wizard_platforms'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
