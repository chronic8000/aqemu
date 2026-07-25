# New VM Wizard — four human-readable paths

Users never need raw QEMU IDs (`q35`, `raspi3b`, `sun4u`) to start.
Those stay as internal mappings in [`qemu_machine_catalog.json`](qemu_machine_catalog.json)
and [`../resources/wizard_trees.json`](../resources/wizard_trees.json).

Friendly catalog: [`QEMU_MACHINE_CATALOG.md`](QEMU_MACHINE_CATALOG.md)

## First step

```
How do you want to create this VM?

  ○ Guest Operating System     Windows 11, Ubuntu, IRIX…
  ○ System / Machine Platform  Raspberry Pi 3, SGI Indy, PC (Q35)…
  ○ CPU Architecture           x86-64 → machines; ARM64 → machines…
  ○ Custom / Advanced          Typical/Custom disk flow + any qemu-system-*
```

Every path ends on **Confirm QEMU system** (Computer Type **and Machine Type** are always editable), then
Accelerator → Name/CPU → Disk → **Devices (guest-aware)** → Network → Finish.

## Guest-aware Devices page

AQEMU computes a capability profile from the OS / platform / arch:

| Guest class | Defaults | Hidden unless “Show all” |
|-------------|----------|---------------------------|
| Windows 9x / DOS | IDE, NE2000, SB16, Cirrus, **TCG only** | VirtIO, GPU passthrough, AdLib on non-PC |
| Classic Mac (mac99) | sungem, board video, Screamer | AdLib/SB16, VirtIO-GPU |
| Modern Linux/BSD/ARM | VirtIO disk/net/GPU, KVM/WHPX when native | — |
| Modern Windows | e1000 + IDE (easy install); VirtIO optional | — |

Optional **GPU passthrough (NVIDIA/AMD)** appears only when the guest can use KVM/IOMMU-style assign.
Power users can check **Show all QEMU options** to unlock the full lists.

## Flows

| Path | Steps |
|------|--------|
| **Guest OS** | OS tree → confirm arch/machine → accelerator → … |
| **Platform** | Platform tree (curated + **All QEMU machines…**) → confirm → … |
| **Architecture** | Arch list (curated + any configured binary) → machine tree (bindings + full catalog / live `-M help`) → confirm → … |
| **Custom** | Typical/Custom mode → template or generate any arch + machine → … |

## Mapping examples

| User sees | AQEMU resolves |
|-----------|----------------|
| **Windows 11** | Profile → ARM64 Virt or x86-64 Q35 + UEFI/VirtIO |
| **ARM64 / AArch64** | `qemu-system-aarch64` + recommended / catalog machines |
| **Raspberry Pi 3** | matching ARM binary + `raspi3b` |
| **SGI Indy** | matching MIPS binary + `indy` |
| **PC (Q35)** | `qemu-system-x86_64` + `q35` |

## Catalog + live probe

`qemu_machine_catalog.json` is copied next to `aqemu.exe` on build. If missing for a target,
the wizard runs `qemu-system-<arch> -machine help` and lists those boards.

## Regenerate after QEMU updates

```powershell
python scripts/probe_qemu_machines.py --qemu-dir "C:\Program Files\qemu"
```

```bash
python3 scripts/probe_qemu_machines.py --qemu-dir /usr/bin
```
