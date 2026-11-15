# NEC PC-98 support research (#124)

Requested via community feedback (#57). Research only for AQEMU 1.7 — no integration yet.

## Upstream QEMU

Stock QEMU has **no** PC-98 machine type. A plain `qemu-system-i386` cannot boot PC-98 firmware or disks.

## Options

### 1. Launch Neko Project 21/W (np21w) — recommended first product path

- Mature PC-9801 / PC-9821 emulator: https://simk98.github.io/np21w/
- Strong game/OS coverage through Windows 2000; optional HAXM accel on older Windows hosts.
- **Not QEMU**, so AQEMU would treat it like xemu/Android wraps: pick a binary, map disks/floppies, write a small launcher config.
- Licence / redistributable bits need a lawyer pass before Store packaging.

### 2. QEMU-based forks

| Tree | Notes |
|--|--|
| Historical **QEMU/9821** (TAKEDA) | PC-9801BX4 focus; needs real BIOS; UI is raw QEMU |
| **awemorris/qemu-pc98** (2026 modernisation) | `pc9801` / `pc9821` machines, PEGC/Core-Graph options, GPL, derived from QEMU/9821 with NP2 cross-checks |

Forking or bundling qemu-pc98 is the long-term path if AQEMU wants one QEMU binary family. Cost: separate Windows/ARM64 builds, firmware legality, and maintenance against upstream QEMU.

### 3. RetroArch / libretro NP2 cores

Useful for end users, awkward as a first-class AQEMU “VM” (no QMP, different disk model).

## Recommendation for AQEMU

1. **Short term:** document BYO np21w under Lab → Wraps (same pattern as xemu), no Store redistributable.
2. **Medium term:** evaluate awemorris/qemu-pc98 as an optional `qemu-system-pc98` companion binary once Windows CI can build it.
3. **Do not** pretend stock QEMU 11 can run PC-98.

## Open questions

- May we ship any PC-98 BIOS / font ROMs, or must users supply them?
- Store policy for a third-party emulator binary vs GitHub-only companion.
- Priority vs Inferno / Reims (likely lower).
