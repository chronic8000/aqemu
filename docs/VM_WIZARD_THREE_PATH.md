# New VM Wizard — three human-readable paths

Users never see raw QEMU IDs (`q35`, `raspi3b`, `sun4u`) in the lists.
Those stay as internal mappings in [`qemu_machine_catalog.json`](qemu_machine_catalog.json).

Friendly catalog: [`QEMU_MACHINE_CATALOG.md`](QEMU_MACHINE_CATALOG.md)

## First step

```
How do you want to create this VM?

  ○ Guest Operating System     Windows 11, Ubuntu, IRIX…
  ○ CPU Architecture           x86-64, ARM64, SPARC64…
  ○ System / Machine Platform  Raspberry Pi 3, SGI Indy, PC (Q35)…
  ○ Custom                     pick any platform yourself
```

## Mapping

| User sees | AQEMU resolves |
|-----------|----------------|
| **Windows 11** | Profile → ARM64 Virt or x86-64 Q35 + UEFI/VirtIO |
| **ARM64 / AArch64** | `qemu-system-aarch64` + recommended machine list (friendly names) |
| **Raspberry Pi 3** | `qemu-system-arm` / `aarch64` + `raspi3b` |
| **SGI Indy** | matching MIPS binary + `indy` |
| **PC (Q35)** | `qemu-system-x86_64` + `q35` |

## Regenerate after QEMU updates

```powershell
python scripts/probe_qemu_machines.py --qemu-dir "C:\Program Files\qemu"
```

```bash
python3 scripts/probe_qemu_machines.py --qemu-dir /usr/bin
```
