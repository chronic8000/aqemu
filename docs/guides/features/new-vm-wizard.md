# The New VM Wizard: All 7 Ways to Make a VM

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Open it with **Add New VM** (toolbar), **VM → Add VM → Add New VM**, or **Ctrl+N**. The first page, **Creation Method**, asks "How do you want to create this virtual machine?" and offers seven methods.

| # | Method | Pick it when… |
| --- | --- | --- |
| 1 | **Guest Operating System** | You know which OS you want (most people) |
| 2 | **System / Machine Platform** | You want a particular computer or board (Raspberry Pi, PowerMac, SPARC…) |
| 3 | **CPU Architecture** | You want a CPU family, then a machine |
| 4 | **Custom / Advanced** | You want full manual control |
| 5 | **Import Existing Disk** | You already have a disk image |
| 6 | **Import Virtual Appliance (OVA / OVF)** | You have an appliance exported from VirtualBox, VMware or a cloud |
| 7 | **Recover storage** | You want to read a set of NAS disks in a read-only TrueNAS VM |

---

## 1. Guest Operating System

> "Pick Windows, Linux, macOS, DOS, BSD… — AQEMU selects the optimal QEMU target and machine."

OSes are grouped by vendor: **Microsoft**, **Linux** (split into ARM64, x86_64 and 32-bit x86), **BSD**, **Apple**, **IBM**, **Sun**, **SGI**, **HP**, **DEC**, **SCO**, **Other**, **TrueNAS / Storage**, **Consoles & Retro** and **Lab**. Each entry has a **profile**: a starting target, machine, CPU, RAM, disk size and devices, plus a tip. See the [OS catalog](../os-catalog.md) and the [guest OS guides](../guest-os/README.md).

## 2. System / Machine Platform

> "Pick board category (ARM Virt, Apple PowerMac, Sun SPARCstation, SBC, Retro…) — locks in target & machine."

Categories include IBM PC Compatible (Q35, i440FX, ISA PC, MicroVM), ARM Virtual Platforms, Raspberry Pi (Zero, 1–4), Apple Macintosh (Mac99, G3, Quadra 800), Sun, IBM Power and Mainframe, MIPS, DEC Alpha, HP PA-RISC, RISC-V, Embedded (Arduino, STM32, micro:bit…) and Consoles & Retro. **All QEMU machines…** lists every board your QEMU knows.

## 3. CPU Architecture

> "Pick any of the 29 QEMU targets (x86_64, aarch64, ppc64, riscv64, s390x, sparc…) — then pick machine."

After the architecture, the **Select Machine** page lists boards from AQEMU's machine catalog. If the catalog has nothing for a target, AQEMU asks QEMU itself (`-machine help`).

## 4. Custom / Advanced

> "Full manual control over QEMU binary, machine type, RAM, and hardware controller configurations."

1. **Wizard Mode:** **Typical** ("Create a virtual machine with the most common devices…") or **Custom** (choose everything).
2. **VM Hardware Template:** **Template** (start from an OS type) or **Generate VM (any QEMU arch)**: "choose any configured qemu-system-* plus Machine Type. You can change both later in VM settings."

## 5. Import Existing Disk

> "Select an existing qcow2, raw, vmdk, or vhdx disk image — AQEMU resolves format & default controller."

This takes you through the Typical flow with **Use an existing disk image** selected on the **Virtual Hard Disk** page.

## 6. Import Virtual Appliance (OVA / OVF)

> "Import a multi-disk virtual appliance (.ova or .ovf) from VirtualBox, VMware, or cloud."

This opens a separate dialog (also at **File → Import and Export → Import Appliance (OVA / OVF)…**, **Ctrl+I**):

- **Appliance Source:** the `.ova` or `.ovf` file.
- **Appliance Settings:** VM name, guest OS, vCPU cores, RAM.
- **Target Disk Format:** **QCOW2 (Recommended - Dynamic & Compressed)**, **QCOW2 with 4K Cluster Alignment (TrueNAS / 4Kn / ZFS)** or **Keep Original Format (.vmdk)**.
- **Appliance Storage Disks:** a table of disks (name, capacity, format, serial, sector size). Disk serials are kept.
- **Destination Storage**, then **Import Appliance**.

To go the other way, use **File → Import and Export → Export Appliance (OVA)…** (**Ctrl+E**).

## 7. Recover storage

> "Read labels from a disk set or physical disks, show a recovery report, then build a read-only TrueNAS VM. The guest decides whether a pool imports."

This opens **Storage Recovery** (also at **VM → Recovery → Storage Recovery…**):

1. Add sources: **Add disk files…**, **Add folder…** or **Add physical disks…**. The host's boot disk is never offered.
2. Optionally choose a **Boot ISO**.
3. Click **Analyze**. "The recovery report appears here before any VM is created."
4. Click **Create Recovery VM**.

AQEMU doesn't promise your pool will import. That's up to the guest OS. See [TrueNAS and storage](../guest-os/truenas.md).

---

## The pages after the first choice

Most methods continue through the same pages:

1. **Confirm QEMU system:** the QEMU binary (Computer Type) and Machine Type. Both stay editable.
2. **Accelerator:** **TCG (software emulation)** or **WHPX / KVM (native acceleration)**. See [Accelerators](accelerators.md).
3. **VM Name and CPU Type:** name, CPU type, CPU cores.
4. **Memory:** RAM size. Tick "Remove limitation on maximum amount of memory" for more than the slider allows.
5. **Virtual Hard Disk:**
   - **Create a new disk image** (with **Disk Size in GB**) or **Use an existing disk image**.
   - **Install media:** **Local ISO / image file**, **Download ISO from URL** (downloads into your VM folder and uses it as the CD-ROM) or **Network install (kernel + initrd URLs)**.
6. **Devices:** guest-aware defaults. **Show all QEMU options (power user — may offer devices this guest cannot use)** unlocks the full lists.
7. **Network:**
   - **User Mode / NAT (works on Windows & Linux — recommended)**
   - **Bridge (LAN — needs a host bridge interface)**
   - **TAP (host virtual interface — advanced)**
   - **No Network (no NIC)**

Some guests get extra pages, for example **Windows 11 ARM Install** and **Intel macOS Setup**.

---

[Documentation Hub](../../README.md) · [Wizard internals](../../reference/new-vm-wizard-paths.md) · [Your first VM](../getting-started/your-first-vm.md)
