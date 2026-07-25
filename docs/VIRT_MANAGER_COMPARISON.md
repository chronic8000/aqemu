# AQEMU vs virt-manager

Reference clone (sibling checkout): `../virt-manager` from
[virt-manager/virt-manager](https://github.com/virt-manager/virt-manager.git).

## Short answer

**No — we do not yet do everything virt-manager does, and we should not claim that on Reddit.**

We also **already beat** virt-manager in several areas Reddit cares about (Windows host,
obscure/retro guests, deeper QEMU knobs, in-app SPICE). The stacks are different:

| | **AQEMU** | **virt-manager** |
|--|-----------|------------------|
| Backend | Talks to **QEMU** (args + QMP/HMP) | Talks to **libvirt** (XML API) |
| Host focus | Windows + Linux (+ WSL/KVM recipes) | Linux-first; remote libvirt URIs |
| Philosophy | “Make every QEMU guest easy” | “Stable UI for common libvirt tasks” ([DESIGN.md](https://github.com/virt-manager/virt-manager/blob/main/DESIGN.md)) |

virt-manager’s own design doc says it is **not** trying to be VirtualBox/VMware-class.
AQEMU *is* aiming closer to that for QEMU.

## Feature matrix (honest)

### Where AQEMU is ahead (or unique)

| Area | Notes |
|------|--------|
| **Guest OS breadth** | Huge wizard trees + profiles (Win9x TCG, XP, OS/2, classic Mac, Intel macOS, Win11 ARM, AIX/pseries, IRIX, …) |
| **Guest-aware devices** | Capability matrix: no VirtIO on Win98, no AdLib on mac99, etc. |
| **Windows host** | First-class packaging, WHPX/TCG, Store/portable |
| **Embedded SPICE session** | Guest display + toolbar inside the app |
| **Deep QEMU options** | NUMA, TPM, fw_cfg, blockdev graph, audiodev, migrate cancel, … |
| **Machine catalog** | Full `-M` lists / probe, not just libvirt “recommended” x86 |

### Where virt-manager is ahead (remaining gaps)

| Area | virt-manager | AQEMU | Notes |
|------|--------------|-------|-------|
| **Virtual networks** | NAT/bridge/macvtap UI | User/bridge/TAP/none in wizard | Still weaker than libvirt virtual nets |
| **Snapshots UX** | Domain snapshots | Present, less polished | QMP `savevm` / external snapshots |
| **In-process Xen/LXC** | Native via libvirt | External only | **File → Remote Hosts** opens virt-manager/virsh |
| **Fleet / multi-host fabric** | Connection list + migrate | Helpers only | SSH tunnels + migrate URI UI — not a libvirt cluster |

### Borrowed / shipped (QEMU-native)

| Area | Status |
|------|--------|
| **OS detection from ISO** | Filename + ISO9660 volume ID; optional `osinfo-detect` (libosinfo) on Linux |
| **URL / network install** | Wizard: download ISO URL, or kernel+initrd URLs → `-kernel`/`-initrd`/`-append` |
| **Import existing disk** | Wizard method **Import Existing Disk** |
| **Storage browser** | **File → Storage Browser…** (VM folder as pool) |
| **Network modes** | User/NAT, Bridge, TAP, None |
| **Clone VM** | **Clone VM…** + disk copy options |
| **Serial console** | Session toolbar → TCP serial |
| **Remote hosts** | **File → Remote Hosts…** — SSH port forwards; libvirt/Xen/LXC via external tools |
| **Live migrate** | Session / Emulator Control → migrate URI dialog (`tcp:host:port`, optional `blk`) |

### Roughly similar

Lifecycle start/stop/pause, CD/USB hotplug, memory/CPU edits, SPICE/VNC graphics,
UEFI, VirtIO when appropriate, add hardware after create.

## What “everything virt-manager does with QEMU” means

Do **not** clone libvirt wholesale. Reimplement the **user-visible QEMU workflows**:

1. **Create** — ISO / existing disk / URL / manual (we’re strong on OS/arch/platform paths).
2. **Install media detection** — suggest OS from ISO when possible.
3. **Network** — usernet, bridge, tap with clear defaults.
4. **Storage** — named folders (“pools”), browse volumes, resize.
5. **Clone / snapshot / delete** — one-click, safe disk handling.
6. **Console** — graphical SPICE (have) + serial (have).
7. **Hardware add** — disk/NIC/USB/filesystem (virtiofs/9p) with guest filters.

**Out of scope as in-process backends:** Xen, LXC, bhyve — use Remote Hosts → libvirt URI.

## Reddit talking points (accurate)

> AQEMU isn’t a virt-manager clone — it’s a QEMU-native manager aimed at Windows and
> obscure guests virt-manager never prioritized. We borrow virt-manager UX (import, URL
> install, storage browser, remote helpers) without requiring libvirt. Full Xen/LXC and
> multi-host fabric still live in virt-manager when you need them.

## Implementation status

| Priority | Status |
|----------|--------|
| Wizard: Import + ISO → OS guess | **done** |
| Serial console in session | **done** |
| Storage browser + clone polish | **done** |
| Network page clarity | **done** |
| URL / network install | **done** |
| Deeper media detect (ISO9660 + osinfo-detect) | **done** |
| Remote hosts + live migrate URI | **done** (QEMU-native + external libvirt) |
| In-app Xen/LXC | **won’t** — launch virt-manager instead |

### Where to click

| Feature | Where |
|---------|--------|
| Import existing disk | New VM wizard → **Import Existing Disk** |
| ISO / URL / kernel+initrd | Typical disk page — install media radios |
| Storage browser | **File → Storage Browser…** |
| Remote hosts / Xen·LXC helper | **File → Remote Hosts…** |
| Serial console | Session toolbar |
| Live migrate | Session toolbar / Emulator Control → **Migrate…** |
| Clone | **Clone VM…** |
