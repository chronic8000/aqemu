#!/usr/bin/env python3
import json, subprocess, pathlib

raw = subprocess.check_output(
    ["gh", "issue", "list", "-R", "tobimensch/aqemu",
     "--state", "open", "--limit", "100", "--json", "number,title"],
    text=True)
issues = sorted(json.loads(raw), key=lambda x: x["number"])

# Manual triage map: number -> (bucket, wave, notes)
# bucket: done | wontfix | wave1 | wave2 | wave3 | wave4
DONE = {
    6: "Wizard rewritten + wizard_trees.json",
    10: "Logo present in resources/icons",
    15: "Screenshots in README/screenshots/",
    63: "UEFI/OVMF supported in modern flows",
    71: "qemu-img format probe + filters",
    102: "VMDK filter (re-verify wmdk typo gone)",
    107: "qemu-system-aarch64 discovery",
    112: "Windows portable Releases / Store",
    118: "Raspberry Pi OS template/icon",
    126: "Full arch discovery in First Start",
    137: "Prebuilt Windows zip + MSIX Store path",
}
WONT = {
    45: "Meta: project idle (historical)",
    48: "Donations / Bountysource — out of scope",
    67: "Meta: is it alive? — yes, this fork",
    70: "Snap packaging — not pursuing now",
    73: "GPLv3 relicensing — stay GPLv2",
    79: "VirtualBox comparison FAQ",
    81: "Software Freedom Conservancy — org/policy",
    84: "Host Gtk theme warnings",
    88: "Vague 'sync interfaces'",
    90: "Meta rant",
}
WAVE1 = {
    75: "QEMU version recognition",
    109: "Deprecated QEMU options",
    121: "QEMU not detected though installed",
    131: "Detected as QEMU 2.0 (display/parse)",
    132: "Sound disabled due to bad version detect",
    136: "disable-ticketing short form",
}
WAVE2 = {
    17: "Bridge support",
    19: "TFTP/SAMBA vs network tab",
    44: "Deprecated vlan",
    54: "hostfwd ellipsis ...",
    58: "vlan=0 breaks start",
    59: "Multiple -net user with redirections",
    123: "Basic network UI hide after add",
    134: "hub 0 not connected",
}
WAVE3 = {
    76: "USB passthrough / bus naming",
    87: "Folder sharing",
    106: "Disable/enable media",
    110: "CD-ROM becomes Disk",
    111: "USB add breaks start",
    120: "Shared folder DOS guest",
    122: "Changing floppies",
    124: "USB stick not visible",
    125: "Shared folder not visible",
}
WAVE4 = {
    18: "Screenshot crash",
    23: "Fullscreen",
    26: "Three patches (review)",
    30: "CPU-count bug",
    35: "BIOS ROM",
    36: "Mouse grab",
    40: "FreeBSD power/snapshots",
    41: "-append custom args",
    42: "Memory leak",
    46: "Translations not enabled",
    55: "Qt4 cmake confusion",
    57: "QEMU 3.0 era",
    72: "doesn't start (vague)",
    74: "GCC 10 compile",
    82: "German language package",
    83: "Help links broken",
    85: "Fullscreen broken",
    86: "CPU count changes silently",
    89: "Segfault / deps",
    92: "AUR segfault",
    105: "VM comment field",
    108: "TCP_KEEPIDLE build",
    113: "Arch segfault GUI",
    114: "Arch AUR segfault",
    115: "Debian running VMs",
    119: "Operator precedence",
    130: "CPU type list PowerPC",
    133: "compatmonitor0 / serial",
}

maps = [
    ("done", DONE, "Already addressed in chronic8000/aqemu"),
    ("wontfix", WONT, "Out of scope / meta"),
    ("wave1", WAVE1, "Wave 1 — launch/discovery"),
    ("wave2", WAVE2, "Wave 2 — network"),
    ("wave3", WAVE3, "Wave 3 — USB/media"),
    ("wave4", WAVE4, "Wave 4 — polish"),
]
lookup = {}
for b, d, _ in maps:
    for n, note in d.items():
        lookup[n] = (b, note)

lines = []
lines.append("# Tobimensch issue triage")
lines.append("")
lines.append("Upstream: https://github.com/tobimensch/aqemu/issues")
lines.append("We cannot close issues there. Fixes land in **chronic8000/aqemu**.")
lines.append("")
lines.append("| Upstream | Title | Bucket | Notes |")
lines.append("|----------|-------|--------|-------|")
unmapped = []
for i in issues:
    n = i["number"]
    title = i["title"].replace("|", "/")
    link = f"[#{n}](https://github.com/tobimensch/aqemu/issues/{n})"
    if n in lookup:
        b, note = lookup[n]
        lines.append(f"| {link} | {title} | `{b}` | {note} |")
    else:
        unmapped.append(i)
        lines.append(f"| {link} | {title} | `wave4` | Needs reproduce on current tree |")

lines.append("")
lines.append("## Summary counts")
from collections import Counter
c = Counter()
for i in issues:
    n = i["number"]
    c[lookup[n][0] if n in lookup else "wave4"] += 1
for k, v in sorted(c.items()):
    lines.append(f"- **{k}**: {v}")
lines.append(f"- **total open upstream**: {len(issues)}")
if unmapped:
    lines.append("")
    lines.append("## Auto-assigned to wave4 (not in hand map)")
    for i in unmapped:
        lines.append(f"- #{i['number']}: {i['title']}")

out = pathlib.Path("docs/TOBIMENSCH_ISSUE_TRIAGE.md")
out.write_text("\n".join(lines) + "\n", encoding="utf-8")
print(f"Wrote {out} ({len(issues)} issues)")
print(dict(c))
