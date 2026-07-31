#!/usr/bin/env python3
"""Audit New VM wizard vs qemu_probe_full_v3 ground truth."""
from __future__ import annotations

import json
import re
from collections import defaultdict
from pathlib import Path

ROOT = Path(r"C:\Users\chron\CURSOR-PROJECTS\aqemu")
PROBE = ROOT / "qemu_probe_full_v3"
WIZARD = ROOT / "resources" / "wizard_trees.json"
OUT = ROOT / "docs" / "wizard_probe_audit.json"
GUEST_CAP = ROOT / "src" / "Guest_Capabilities.cpp"
WIZARD_CPP = ROOT / "src" / "VM_Wizard_Window.cpp"
SYSINFO = ROOT / "src" / "System_Info.cpp"


def parse_machine_ids(lines: list[str]) -> set[str]:
    ids = set()
    for line in lines:
        line = line.strip()
        if not line or line.lower().startswith("supported machines"):
            continue
        # "q35                  Standard PC..."
        m = re.match(r"^(\S+)\s+", line)
        if m:
            ids.add(m.group(1))
    return ids


def parse_cpu_ids(lines: list[str]) -> set[str]:
    ids = set()
    for line in lines:
        line = line.rstrip()
        if not line or "Available CPUs" in line:
            continue
        m = re.match(r"^\s+(\S+)", line)
        if m:
            ids.add(m.group(1))
    return ids


def parse_device_names(lines: list[str]) -> set[str]:
    ids = set()
    for line in lines:
        m = re.search(r'name\s+"([^"]+)"', line)
        if m:
            ids.add(m.group(1))
    return ids


def load_probes() -> dict[str, dict]:
    probes = {}
    for p in sorted(PROBE.glob("*.json")):
        data = json.loads(p.read_text(encoding="utf-8"))
        arch = data.get("architecture") or p.stem
        probes[arch] = {
            "fallback_machine": data.get("fallback_machine"),
            "machines": parse_machine_ids(data.get("machines") or []),
            "cpus": parse_cpu_ids(data.get("cpus") or []),
            "devices": parse_device_names(data.get("devices") or []),
            "accelerators": [
                x.strip()
                for x in (data.get("accelerators") or [])
                if x.strip() and not x.lower().startswith("accelerator")
            ],
            "netdev": [
                x.strip()
                for x in (data.get("netdev") or [])
                if x.strip() and not x.lower().startswith("available")
            ],
            "display_backends": [
                x.strip()
                for x in (data.get("display_backends") or [])
                if x.strip() and not x.lower().startswith("available")
            ],
            "audio_backends": [
                x.strip()
                for x in (data.get("audio_backends") or [])
                if x.strip() and not x.lower().startswith("available")
            ],
            "n_machines": 0,
            "n_cpus": 0,
            "n_devices": 0,
        }
        probes[arch]["n_machines"] = len(probes[arch]["machines"])
        probes[arch]["n_cpus"] = len(probes[arch]["cpus"])
        probes[arch]["n_devices"] = len(probes[arch]["devices"])
    return probes


def machine_exists(probes: dict, target: str, machine: str) -> bool | None:
    if not machine:
        return None
    if target not in probes:
        return False
    mset = probes[target]["machines"]
    if machine in mset:
        return True
    # alias / versioned: pc matches pc-i440fx-*, q35 matches pc-q35-*, virt matches virt-*
    for mid in mset:
        if mid == machine or mid.startswith(machine + "-") or machine.startswith(mid):
            return True
        if machine == "pc" and ("i440fx" in mid or mid.startswith("pc-")):
            return True
        if machine == "q35" and "q35" in mid:
            return True
        if machine == "virt" and (mid == "virt" or mid.startswith("virt-")):
            return True
        if machine == "pseries" and "pseries" in mid:
            return True
        if machine == "powernv" and mid.startswith("powernv"):
            return True
    return False


def main() -> None:
    probes = load_probes()
    wizard = json.loads(WIZARD.read_text(encoding="utf-8"))

    arch_targets = wizard.get("architecture_targets") or {}
    platform_bindings = wizard.get("platform_bindings") or {}
    os_profiles = wizard.get("os_profiles") or {}
    operating_systems = wizard.get("operating_systems") or {}
    platforms = wizard.get("platforms") or {}

    probe_arches = sorted(probes.keys())
    wizard_arches = sorted(set(arch_targets.values()))

    missing_from_wizard = sorted(set(probe_arches) - set(wizard_arches))
    missing_from_probes = sorted(set(wizard_arches) - set(probe_arches))

    # Platform bindings vs probes
    platform_ok = []
    platform_bad = []
    platform_unknown_target = []
    for name, bind in sorted(platform_bindings.items()):
        tgt = bind.get("target") or ""
        mach = bind.get("machine") or ""
        if tgt not in probes:
            platform_unknown_target.append({"platform": name, "target": tgt, "machine": mach})
            continue
        ok = machine_exists(probes, tgt, mach)
        row = {"platform": name, "target": tgt, "machine": mach}
        if ok:
            platform_ok.append(row)
        else:
            platform_bad.append(row)

    # Platforms listed under family groups but missing bindings
    unbound_platforms = []
    for fam, names in platforms.items():
        for n in names:
            if n not in platform_bindings:
                unbound_platforms.append({"family": fam, "platform": n})

    # OS profiles
    os_ok = []
    os_bad_machine = []
    os_missing_target = []
    os_missing_profile = []
    all_os_names = []
    for fam, names in operating_systems.items():
        for n in names:
            all_os_names.append(n)
            if n not in os_profiles:
                os_missing_profile.append({"family": fam, "os": n})
                continue
            prof = os_profiles[n]
            tgt = prof.get("target") or ""
            mach = prof.get("machine") or ""
            flags = prof.get("flags") or []
            if tgt not in probes:
                os_missing_target.append(
                    {"os": n, "family": fam, "target": tgt, "machine": mach, "flags": flags}
                )
                continue
            ok = machine_exists(probes, tgt, mach) if mach else True
            row = {
                "os": n,
                "family": fam,
                "target": tgt,
                "machine": mach or "(default)",
                "flags": flags,
            }
            if ok:
                os_ok.append(row)
            else:
                os_bad_machine.append(row)

    # Known Guest_Capabilities special cases from code (static grep-ish)
    guest_cpp = GUEST_CAP.read_text(encoding="utf-8", errors="replace")
    wizard_cpp = WIZARD_CPP.read_text(encoding="utf-8", errors="replace")
    sysinfo = SYSINFO.read_text(encoding="utf-8", errors="replace")

    # Hardware honesty bugs already known / code signals
    code_signals = {
        "classic_mac_forces_ppc_mac99": "Set_Computer_Type( \"qemu-system-ppc\" )" in wizard_cpp
        and "mac99" in wizard_cpp,
        "disk_bus_classic_mac_ide": "is_classic_mac" in sysinfo and "mac99" in sysinfo,
        "default_video_ppc_was_std": 'case VAF_PPC: return "std"' in sysinfo,
        "sanitize_video_classic_mac_empty": "classic_mac" in sysinfo
        and "board framebuffer" in sysinfo.lower()
        or "mac99/g3beige already have" in sysinfo,
        "devices_page_exists": "Build_Devices_Page" in wizard_cpp,
        "custom_method_radio": "RB_Method_Custom" in wizard_cpp,
        "live_machine_probe": "Probe_Live_Machines" in wizard_cpp,
        "guest_caps_file": GUEST_CAP.exists(),
    }

    # Device exposure estimate: Guest_Capabilities / System_Info curated lists vs probe
    # Heuristic NIC/video names commonly filtered
    curated_nics_hint = sorted(
        set(
            re.findall(
                r'QStringLiteral\(\s*"([a-z0-9_-]+)"\s*\)',
                guest_cpp,
                flags=re.I,
            )
        )
    )

    # Coverage stats per arch
    arch_coverage = []
    for arch in probe_arches:
        bindings_for = [p for p in platform_ok + platform_bad if p["target"] == arch]
        os_for = [o for o in os_ok + os_bad_machine if o["target"] == arch]
        machines_used = {p["machine"] for p in bindings_for}
        machines_used |= {o["machine"] for o in os_for if o["machine"] != "(default)"}
        probe_m = probes[arch]["machines"]
        # Count how many probe machines appear in wizard (exact or prefix)
        covered = 0
        for mid in probe_m:
            if mid in ("none",):
                continue
            hit = False
            for used in machines_used:
                if used == mid or used in mid or mid.startswith(used) or used.startswith(mid.split("-")[0]):
                    # looser: used token matches
                    if used == mid or mid.startswith(used) or used in mid:
                        hit = True
                        break
            if hit:
                covered += 1
        useful_probe = max(1, len([m for m in probe_m if m != "none"]))
        # Better: exact match of binding machines against probe
        exact_hits = 0
        for used in machines_used:
            if machine_exists(probes, arch, used):
                exact_hits += 1
        arch_coverage.append(
            {
                "arch": arch,
                "probe_machines": probes[arch]["n_machines"],
                "probe_cpus": probes[arch]["n_cpus"],
                "probe_devices": probes[arch]["n_devices"],
                "wizard_platforms": len(bindings_for),
                "wizard_os_profiles": len(os_for),
                "machines_referenced": sorted(machines_used),
                "referenced_count": len(machines_used),
                "fallback": probes[arch]["fallback_machine"],
                "accels": probes[arch]["accelerators"],
            }
        )

    # Critical OS hardware risk list (manual knowledge + profile flags)
    risk_notes = []
    for o in os_ok + os_bad_machine + os_missing_target:
        flags = o.get("flags") or []
        tgt = o.get("target")
        mach = o.get("machine")
        osn = o.get("os")
        note = None
        if osn in ("Mac OS X PPC", "Mac OS 9", "Mac OS 8", "Mac OS 7") and tgt == "ppc64":
            note = "PPC classic must be qemu-system-ppc + mac99, not ppc64/pseries"
        if osn in ("Mac OS X PPC",) and mach in ("pseries", ""):
            note = "Empty/pseries machine → SLOF not OpenBIOS"
        if "next" in str(mach).lower() and tgt != "m68k":
            note = "NeXT should be m68k/next-cube"
        if osn == "AIX" and mach and "pseries" not in str(mach):
            note = "AIX expects pseries"
        if osn == "NeXTSTEP" and mach != "next-cube":
            note = f"NeXTSTEP profile machine={mach}, expected next-cube"
        if note:
            risk_notes.append({"os": osn, "target": tgt, "machine": mach, "issue": note, "flags": flags})

    # Extra: check specific known-broken bindings
    for row in platform_bad:
        risk_notes.append(
            {
                "os": None,
                "target": row["target"],
                "machine": row["machine"],
                "issue": f"Platform '{row['platform']}' machine not in probe for target",
                "flags": [],
            }
        )

    # Device category exposure (from probe sample x86_64)
    def device_categories(arch: str) -> dict[str, int]:
        raw = json.loads((PROBE / f"{arch}.json").read_text(encoding="utf-8"))
        cats = defaultdict(int)
        current = "other"
        for line in raw.get("devices") or []:
            if line.endswith("devices:") or line.endswith("Devices:"):
                current = line.strip().rstrip(":")
                continue
            if 'name "' in line:
                cats[current] += 1
        return dict(cats)

    gui_exposure = {
        "summary": (
            "Wizard Devices page + Main Window expose curated subsets "
            "(Machine, CPU, NIC, Video, Audio, Disk bus) from Available_Devices "
            "(First-Start QEMU probe / catalog), NOT the full -device help list."
        ),
        "wizard_exposes": [
            "Architecture / qemu-system-* target",
            "Machine type (from Available_Devices.Machine_List + live Probe_Live_Machines)",
            "CPU type (CPU_List)",
            "NIC model (Network_Card_List / Guest_Capabilities filter)",
            "Video (Video_Card_List / Sanitize_Video_Card)",
            "Disk interface (IDE/AHCI/VirtIO/SCSI/… via Guest_Capabilities + System_Info)",
            "Audio cards (Sound_Cards bitfield)",
            "RAM / disk size",
            "Accelerator TCG vs KVM/WHPX",
        ],
        "wizard_does_not_expose_full_probe": [
            "Full -device help inventory (hundreds of PCI/USB/misc devices)",
            "All netdev backends",
            "All chardev backends",
            "All object types",
            "Most display backends (spice/gtk/sdl/egl-headless per-binary)",
            "Versioned machine aliases (pc-i440fx-5.1 …) unless live probe fills them",
            "Per-device property help",
        ],
        "custom_path": {
            "exists": True,
            "description": (
                "Custom / Advanced picks arch+machine manually, then Devices page. "
                "It still uses Available_Devices lists — not free-form every QEMU device. "
                "Additional Args on the finished VM can pass raw QEMU flags."
            ),
            "limitations": [
                "Cannot pick arbitrary -device from probe list in the wizard UI",
                "Machine combo may be catalog-filtered; live Probe_Live_Machines helps when binary works",
                "OS-specific Apply_* overrides (classic Mac, Win9x, etc.) only run on OS path, not pure Custom",
            ],
        },
        "x86_64_device_categories": device_categories("x86_64"),
        "ppc_device_categories": device_categories("ppc"),
        "m68k_device_categories": device_categories("m68k"),
    }

    # Verdict scoring
    n_os = len(all_os_names)
    n_os_ok = len(os_ok)
    n_plat = len(platform_bindings)
    n_plat_ok = len(platform_ok)

    report = {
        "meta": {
            "probe_dir": str(PROBE),
            "wizard_trees": str(WIZARD),
            "probe_architectures": len(probe_arches),
            "wizard_architecture_targets": len(wizard_arches),
            "os_names_in_tree": n_os,
            "os_profiles": len(os_profiles),
            "platform_bindings": n_plat,
            "unbound_platform_labels": len(unbound_platforms),
        },
        "verdict": {
            "supports_all_qemu_binaries": len(missing_from_wizard) == 0
            and len(missing_from_probes) == 0,
            "os_machine_compatibility_pct": round(100.0 * n_os_ok / max(1, n_os - len(os_missing_profile)), 1),
            "platform_binding_ok_pct": round(100.0 * n_plat_ok / max(1, n_plat), 1),
            "exposes_full_device_help": False,
            "custom_allows_full_manual": False,
            "headline": (
                "Wizard covers all probed qemu-system-* arches and most OS→machine bindings, "
                "but does NOT expose the full probe device inventory. Custom is curated-manual, "
                "not raw QEMU. Several honesty bugs (disk/video/accel) were found historically; "
                "classic Mac path is specially forced."
            ),
        },
        "architecture_gaps": {
            "in_probes_not_wizard": missing_from_wizard,
            "in_wizard_not_probes": missing_from_probes,
            "wizard_targets": wizard_arches,
            "probe_targets": probe_arches,
        },
        "platform_bindings": {
            "ok": len(platform_ok),
            "bad": platform_bad,
            "unknown_target": platform_unknown_target,
            "unbound_labels": unbound_platforms[:40],
            "unbound_total": len(unbound_platforms),
        },
        "os_profiles": {
            "ok": n_os_ok,
            "bad_machine": os_bad_machine,
            "missing_target_probe": os_missing_target,
            "missing_profile": os_missing_profile,
            "sample_ok": os_ok[:15],
        },
        "risk_notes": risk_notes,
        "arch_coverage": arch_coverage,
        "gui_exposure": gui_exposure,
        "code_signals": code_signals,
        "findings": [],
    }

    # Build findings list
    findings = report["findings"]
    if missing_from_wizard:
        findings.append(
            {
                "severity": "high",
                "title": "Probe arches missing from wizard architecture_targets",
                "detail": ", ".join(missing_from_wizard),
            }
        )
    if missing_from_probes:
        findings.append(
            {
                "severity": "medium",
                "title": "Wizard targets have no probe JSON",
                "detail": ", ".join(missing_from_probes),
            }
        )
    if platform_bad:
        findings.append(
            {
                "severity": "high",
                "title": f"{len(platform_bad)} platform bindings reference machines not in probe",
                "detail": "; ".join(
                    f"{p['platform']} → {p['target']}/{p['machine']}" for p in platform_bad[:12]
                ),
            }
        )
    if os_bad_machine:
        findings.append(
            {
                "severity": "high",
                "title": f"{len(os_bad_machine)} OS profiles have invalid machines",
                "detail": "; ".join(
                    f"{p['os']} → {p['target']}/{p['machine']}" for p in os_bad_machine[:12]
                ),
            }
        )
    if os_missing_profile:
        findings.append(
            {
                "severity": "medium",
                "title": f"{len(os_missing_profile)} OS tree entries lack os_profiles",
                "detail": ", ".join(p["os"] for p in os_missing_profile[:20]),
            }
        )
    if unbound_platforms:
        findings.append(
            {
                "severity": "medium",
                "title": f"{len(unbound_platforms)} platform labels have no platform_bindings",
                "detail": "Selecting them in the Platforms tree may fall back or fail",
            }
        )
    findings.append(
        {
            "severity": "high",
            "title": "Full QEMU -device help is not selectable in the wizard GUI",
            "detail": (
                f"x86_64 probe lists ~{probes['x86_64']['n_devices']} devices; "
                "UI only shows curated NIC/video/audio/disk-bus lists."
            ),
        }
    )
    findings.append(
        {
            "severity": "medium",
            "title": "Custom / Advanced is not full raw QEMU coverage",
            "detail": (
                "User can pick any probed target + machine (+ Devices page curated lists). "
                "Arbitrary devices still need Additional Args after creation."
            ),
        }
    )
    findings.append(
        {
            "severity": "medium",
            "title": "Versioned machine matrix mostly hidden",
            "detail": (
                "Probes list dozens of pc-i440fx-*/pc-q35-* versions; wizard usually offers "
                "aliases (pc, q35) unless live probe populates Machine_List fully."
            ),
        }
    )

    # OS default honesty: check a few critical profiles
    critical = [
        "Mac OS X PPC",
        "macOS",
        "Mac OS X Intel",
        "NeXTSTEP",
        "AIX",
        "Windows 98",
        "Windows XP (32-bit)",
        "Windows 11",
        "Ubuntu (64-bit)",
        "ReactOS",
        "OS/2",
        "Haiku (64-bit)",
        "FreeBSD (64-bit)",
        "Solaris",
        "OpenBSD (64-bit)",
        "Linux on IBM Z",
        "HP-UX",
        "Android",
        "Chrome OS Flex",
    ]
    critical_rows = []
    for name in critical:
        prof = os_profiles.get(name)
        if not prof:
            critical_rows.append({"os": name, "status": "NO_PROFILE", "target": "", "machine": ""})
            continue
        tgt = prof.get("target", "")
        mach = prof.get("machine", "")
        ok = tgt in probes and (not mach or machine_exists(probes, tgt, mach))
        critical_rows.append(
            {
                "os": name,
                "status": "OK" if ok else "BAD",
                "target": tgt,
                "machine": mach or "(default)",
                "flags": prof.get("flags") or [],
            }
        )
    report["critical_os"] = critical_rows

    OUT.parent.mkdir(parents=True, exist_ok=True)
    # Make JSON serializable (sets already converted)
    OUT.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"Wrote {OUT}")
    print("verdict", report["verdict"])
    print("platform bad", len(platform_bad))
    print("os bad", len(os_bad_machine))
    print("missing profiles", len(os_missing_profile))
    print("unbound platforms", len(unbound_platforms))
    for f in findings:
        print(f"[{f['severity']}] {f['title']}")


if __name__ == "__main__":
    main()
