#!/usr/bin/env python3
"""Regenerate qemu_probe_full_v3 from live QEMU help output.

Help queries only. Never passes -accel whpx and never starts a guest.
Windows binaries are launched with cwd set to their own directory so the
bundled DLLs load (a bare working directory returns 0xC000007B).

Usage:
  python scripts/probe_qemu_full.py --qemu-dir third_party/qemu-install-arm64/bin
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple

QUERIES: List[Tuple[str, List[str]]] = [
    ("machines", ["-machine", "help"]),
    ("cpus", ["-cpu", "help"]),
    ("devices", ["-device", "help"]),
    ("accelerators", ["-accel", "help"]),
    ("netdev", ["-netdev", "help"]),
    ("drive_formats", ["-drive", "format=help"]),
    ("chardev_backends", ["-chardev", "help"]),
    ("display", ["-display", "help"]),
    ("audio", ["-audiodev", "help"]),
    ("objects", ["-object", "help"]),
]

TOP_LEVEL = {
    "machines": "machines",
    "cpus": "cpus",
    "devices": "devices",
    "accelerators": "accelerators",
    "netdev": "netdev",
    "drive_formats": "drive_formats",
    "chardev_backends": "chardev_backends",
    "display": "display_backends",
    "audio": "audio_backends",
    "objects": "object_types",
}

SECTIONS = [
    ("machines", "1. Supported Machines (`-machine help`)"),
    ("cpus", "2. Supported CPUs (`-cpu help`)"),
    ("devices", "3. Supported Devices & Busses (`-device help`)"),
    ("drive_formats", "4. Storage Image Formats (`-drive format=help`)"),
    ("netdev", "5. Network Backends (`-netdev help`)"),
    ("chardev_backends", "6. Character Device Backends (`-chardev help`)"),
    ("display", "7. Display Backends (`-display help`)"),
    ("audio", "8. Audio Drivers (`-audiodev help`)"),
    ("accelerators", "9. Accelerators (`-accel help`)"),
    ("objects", "10. User Objects (`-object help`)"),
]


def target_from_binary(path: Path) -> str:
    name = path.stem
    prefix = "qemu-system-"
    return name[len(prefix):] if name.startswith(prefix) else name


def nonempty_lines(text: str) -> List[str]:
    return [ln.rstrip("\r") for ln in text.replace("\r\n", "\n").split("\n") if ln.strip()]


def machine_names(lines: List[str]) -> List[str]:
    names = []
    for ln in lines:
        m = re.match(r"^(\S+)\s+", ln)
        if not m:
            continue
        token = m.group(1)
        if token.lower() in {"supported", "machines"}:
            continue
        names.append(token)
    return names


def pick_fallback(names: List[str], previous: Optional[str]) -> str:
    if previous and previous in names:
        return previous
    for pref in ("virt", "q35", "pc", "none", "t8030", "s8000"):
        if pref in names:
            return pref
    return names[0] if names else "none"


def previous_fallback(out_dir: Path, arch: str) -> Optional[str]:
    path = out_dir / f"{arch}.json"
    if not path.is_file():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None
    value = data.get("fallback_machine")
    return value if isinstance(value, str) and value else None


def run_help(binary: Path, args: List[str], timeout: float) -> Tuple[int, str]:
    joined = " ".join(args).lower()
    if "whpx" in joined and "help" not in joined:
        raise RuntimeError(f"refusing accelerator launch: {binary} {args}")
    try:
        proc = subprocess.run(
            [str(binary)] + args,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
            cwd=str(binary.parent),
        )
        out = (proc.stdout or "") + (("\n" + proc.stderr) if proc.stderr else "")
        return proc.returncode, out
    except subprocess.TimeoutExpired:
        return -1, f"[timeout] {binary.name} {' '.join(args)}\n"
    except OSError as exc:
        return -2, f"[error] {exc}\n"


def write_markdown(path: Path, arch: str, binary: Path, probed_at: str,
                   fallback: str, raw: Dict[str, str]) -> None:
    lines = [
        f"# QEMU Complete Architecture capability map: `{arch}`",
        "",
        f"- **Architecture Target:** `{arch}`",
        f"- **Binary Executable:** `{binary}`",
        f"- **Probed At:** {probed_at}",
        f"- **Fallback Machine Used for Context:** `{fallback}`",
        "",
        "> **INSTRUCTIONS FOR CURSOR AI:** This document contains the verified whitelist of supported flags, boards, CPUs, devices, storage drivers, audio backends, and display renderers for `qemu-system-{arch}`. Use this data as the absolute ground truth to construct and validate VM configuration parameters.",
        "",
    ]
    for key, title in SECTIONS:
        body = raw.get(key, "").replace("\r\n", "\n").rstrip("\n")
        count = len(nonempty_lines(body))
        lines += [
            f"## {title}",
            "",
            "```text",
            body,
            "```",
            f"Total items listed: {count}",
            "",
        ]
    lines += [
        "## AQEMU Wizard Rules for this Architecture",
        "",
        "1. **Machine Whitelist:** Only select machines that match strings in Section 1.",
        "2. **Bus Attachment Matching:** Devices in Section 3 specify their required bus (e.g. `bus PCI`, `bus ISA`, `bus virtio-bus`). Do not place PCI devices on ISA-only machines like `isapc`.",
        "3. **Network Backend Validation:** Use Section 5 backends (e.g. `user`, `tap`, `socket`).",
        "4. **Storage Drivers:** Only attach block drivers matching Section 4 formats.",
        "",
    ]
    path.write_text("\n".join(lines), encoding="utf-8")


def _quoted_names(lines: List[str]) -> List[str]:
    names = []
    rx = re.compile(r'name\s+"([^"]+)"')
    for line in lines:
        match = rx.search(line)
        if match:
            names.append(match.group(1))
    return names


def _bare_names(lines: List[str]) -> List[str]:
    out = []
    for line in lines:
        token = line.strip()
        if not token or " " in token or token.endswith(":"):
            continue
        out.append(token)
    return out


def probe_properties(qemu_dir: Path, out_dir: Path, timeout: float, workers: int) -> int:
    """Record -device/-machine/-object help. Always -accel tcg, never whpx."""
    binaries = sorted(p for p in qemu_dir.glob("qemu-system-*.exe") if not p.name.lower().endswith("w.exe"))
    if not binaries:
        print(f"No qemu-system-*.exe in {qemu_dir}", file=sys.stderr)
        return 1

    jobs = []
    for binary in binaries:
        arch = target_from_binary(binary)
        catalog = out_dir / f"{arch}.json"
        if not catalog.is_file():
            print(f"skip {arch}: no catalog", flush=True)
            continue
        doc = json.loads(catalog.read_text(encoding="utf-8"))
        side_path = out_dir / f"{arch}.properties.json"
        side = {"devices": {}, "machines": {}, "objects": {}}
        if side_path.is_file():
            try:
                loaded = json.loads(side_path.read_text(encoding="utf-8"))
                for bucket in side:
                    side[bucket].update(loaded.get(bucket) or {})
            except json.JSONDecodeError:
                pass
        fallback = doc.get("fallback_machine") or "none"
        for name in _quoted_names(doc.get("devices") or []):
            if name not in side["devices"]:
                jobs.append((binary, arch, "devices", name, fallback))
        for name in machine_names(doc.get("machines") or []):
            if name not in side["machines"]:
                jobs.append((binary, arch, "machines", name, fallback))
        for name in _bare_names(doc.get("object_types") or []):
            if name not in side["objects"]:
                jobs.append((binary, arch, "objects", name, fallback))
        side_path.write_text(json.dumps(side, indent=2) + "\n", encoding="utf-8")

    print(f"Property help jobs: {len(jobs)}", flush=True)
    if not jobs:
        return 0

    def one(job):
        binary, arch, bucket, name, fallback = job
        tcg = ["-machine", fallback, "-accel", "tcg"]
        if bucket == "devices":
            _rc, text = run_help(binary, tcg + ["-device", f"{name},help"], timeout)
        elif bucket == "machines":
            _rc, text = run_help(binary, ["-machine", f"{name},help", "-accel", "tcg"], timeout)
        else:
            _rc, text = run_help(binary, tcg + ["-object", f"{name},help"], timeout)
        return arch, bucket, name, text

    pending: Dict[str, Dict[str, Dict[str, str]]] = {}
    done = 0
    with ThreadPoolExecutor(max_workers=max(1, workers)) as pool:
        futures = [pool.submit(one, job) for job in jobs]
        for fut in as_completed(futures):
            arch, bucket, name, text = fut.result()
            pending.setdefault(arch, {}).setdefault(bucket, {})[name] = text
            done += 1
            if done % 40 == 0 or done == len(jobs):
                for arch_name, buckets in pending.items():
                    side_path = out_dir / f"{arch_name}.properties.json"
                    side = {"devices": {}, "machines": {}, "objects": {}}
                    if side_path.is_file():
                        try:
                            loaded = json.loads(side_path.read_text(encoding="utf-8"))
                            for key in side:
                                side[key].update(loaded.get(key) or {})
                        except json.JSONDecodeError:
                            pass
                    for key, values in buckets.items():
                        side.setdefault(key, {}).update(values)
                    side_path.write_text(json.dumps(side, indent=2) + "\n", encoding="utf-8")
                pending.clear()
                print(f"  properties {done}/{len(jobs)}", flush=True)
    print("Property help done", flush=True)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--qemu-dir", type=Path, required=True)
    ap.add_argument("--out-dir", type=Path, default=None)
    ap.add_argument("--timeout", type=float, default=60.0)
    ap.add_argument("--properties", action="store_true",
                    help="Probe -device/-machine/-object help with -accel tcg")
    ap.add_argument("--workers", type=int, default=6)
    args = ap.parse_args()

    if args.properties:
        qemu_dir = args.qemu_dir.resolve()
        repo = Path(__file__).resolve().parents[1]
        out_dir = (args.out_dir or (repo / "qemu_probe_full_v3")).resolve()
        return probe_properties(qemu_dir, out_dir, min(args.timeout, 12.0), args.workers)

    qemu_dir = args.qemu_dir.resolve()
    repo = Path(__file__).resolve().parents[1]
    out_dir = (args.out_dir or (repo / "qemu_probe_full_v3")).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    binaries = sorted(qemu_dir.glob("qemu-system-*.exe"))
    binaries = [p for p in binaries if not p.name.lower().endswith("w.exe")]
    if not binaries:
        print(f"No qemu-system-*.exe in {qemu_dir}", file=sys.stderr)
        return 1

    print(f"Probing {len(binaries)} binaries in {qemu_dir}", flush=True)
    failures = 0
    for index, binary in enumerate(binaries, 1):
        arch = target_from_binary(binary)
        print(f"[{index}/{len(binaries)}] {binary.name}", flush=True)
        raw: Dict[str, str] = {}
        bad = False
        for key, query in QUERIES:
            rc, text = run_help(binary, query, args.timeout)
            raw[key] = text if text.endswith("\n") or not text else text + "\n"
            if rc != 0 and key in {"machines", "devices", "accelerators"} and "[timeout]" in text:
                bad = True
                print(f"  {key} failed rc={rc}", flush=True)
        if bad or not nonempty_lines(raw.get("machines", "")):
            failures += 1
            print(f"  skipped write: no machine list", flush=True)
            continue
        names = machine_names(nonempty_lines(raw["machines"]))
        fallback = pick_fallback(names, previous_fallback(out_dir, arch))
        for key, query in QUERIES:
            if key not in {"netdev", "drive_formats", "chardev_backends"}:
                continue
            if "No machine specified" not in raw.get(key, ""):
                continue
            rc, text = run_help(
                binary,
                ["-machine", fallback, "-accel", "tcg"] + query,
                args.timeout,
            )
            if "No machine specified" in text or "[timeout]" in text:
                print(f"  {key} still failed rc={rc} with machine {fallback}", flush=True)
                continue
            raw[key] = text if text.endswith("\n") or not text else text + "\n"
        probed_at = datetime.now().isoformat()
        doc = {
            "binary": str(binary),
            "architecture": arch,
            "probed_at": probed_at,
            "fallback_machine": fallback,
        }
        for key, _query in QUERIES:
            doc[TOP_LEVEL[key]] = nonempty_lines(raw[key])
        doc["raw"] = raw
        json_path = out_dir / f"{arch}.json"
        md_path = out_dir / f"{arch}.md"
        json_path.write_text(json.dumps(doc, indent=2) + "\n", encoding="utf-8")
        write_markdown(md_path, arch, binary, probed_at, fallback, raw)
        accels = [ln.strip() for ln in nonempty_lines(raw["accelerators"]) if ln.strip() and "supported" not in ln.lower()]
        print(f"  machines={len(names)} accel={','.join(accels) or 'none'} -> {json_path.name}", flush=True)

    print(f"Done. failures={failures} out={out_dir}", flush=True)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
