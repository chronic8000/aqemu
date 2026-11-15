# Retro guests and ATAPI CD-ROM timing (#100)

Research notes for AQEMU 1.7 (no QEMU rebuild in this release).

## Symptom

DOS guests using **OAKCDROM.SYS** often fail to see QEMU’s IDE/ATAPI CD-ROM. Switching to **XDVD2.SYS** (or **VIDE-CDD.SYS**) works for some users. Win9x / early NT installs from CD can feel “stuck” on i440fx IDE for a long time during BIOS INT13 reads.

## Causes (upstream / firmware)

1. **SeaBIOS ATAPI uses PIO**, not DMA, for CD-ROM even when IDE DMA is enabled for hard disks ([QEMU issue #1731](https://gitlab.com/qemu-project/qemu/-/work_items/1731)). Bootloaders that read the ISO through INT13 are therefore slow.
2. **DOS ATAPI drivers are picky** about power-on signatures, status bits after invalid ATA commands (e.g. IDENTIFY `0xEC` on a PACKET device), and command timing. Emulators that mishandle those signatures make OAKCDROM.SYS abort after INQUIRY (see VOGONS / UniPCemu discussions). QEMU is generally correct, but **TCG timing** can still trip drivers that busy-wait.
3. Older SeaBIOS builds (1.11.2 / 1.12.0 era) had CD access bugs; AQEMU’s bundled QEMU 11.x ships a newer SeaBIOS, so that specific regression is unlikely.

## What AQEMU does in 1.7

- Retro DOS / Win9x profiles stay on **Force TCG** + cirrus + SB16.
- First start of those profiles shows a **one-time tip**: prefer XDVD2.SYS / VIDE-CDD.SYS over OAKCDROM.SYS; keep the ISO on the secondary IDE channel when possible.
- No AQEMU-only ATAPI timing patch: a real fix belongs in QEMU/SeaBIOS.

## Suggested host-side experiments (when a test box is free)

| Knob | Why |
|--|--|
| `-machine pc` / `pc-i440fx-2.12` vs newer | Older chipset timing |
| CD on `ide1` (secondary) vs `ide0` | Driver probe order |
| `-icount shift=N` | Stabilise TCG timing for picky DOS drivers |
| Win9x install ISO via **AHCI/SATA CD** on q35 | Avoids SeaBIOS IDE INT13 path for modern installers (not for DOS) |

## Done when

Either upstream timing is good enough for OAKCDROM.SYS on DOS + Win9x, or the wizard tip remains the supported answer.
