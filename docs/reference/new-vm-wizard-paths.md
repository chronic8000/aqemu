# New VM Wizard Paths

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

> For a step-by-step walkthrough of every method, see [The New VM Wizard](../guides/features/new-vm-wizard.md).

Users never need raw QEMU IDs (`q35`, `raspi3b`, `sun4u`) to start.
Those stay as internal mappings in [`docs/qemu_machine_catalog.json`](../qemu_machine_catalog.json)
and [`resources/wizard_trees.json`](../../resources/wizard_trees.json).

Friendly catalog: [QEMU machine catalog](qemu-machine-catalog.md)

## First step

```
How do you want to create this virtual machine?

  1. Guest Operating System                 Windows 11, Ubuntu, IRIX…
  2. System / Machine Platform              Raspberry Pi 3, SGI Indy, PC (Q35)…
  3. CPU Architecture                       x86-64 → machines; ARM64 → machines…
  4. Custom / Advanced                      Typical/Custom disk flow + any qemu-system-*
  5. Import Existing Disk                   start from a disk image you already have
  6. Import Virtual Appliance (OVA / OVF)   VirtualBox / VMware / cloud exports
  7. Recover storage                        read NAS disks in a read-only TrueNAS VM
```

The first four paths end on **Confirm QEMU system** (Computer Type **and Machine Type** are always editable), then
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
