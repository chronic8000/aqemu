# Storage and disks

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

A virtual disk is just a file on your computer. This guide covers making disks, adding CDs and floppies, changing the disk type, converting images, and getting data back from broken disks.

---

## 1. Disk image formats in plain English

| Format | What it is | When to use it |
| --- | --- | --- |
| **qcow2** | QEMU's own format. Grows as you fill it, supports snapshots and saved state. | **Use this by default.** |
| **raw** | A plain byte-for-byte image. Simple and fast, no snapshots. | Swapping images with other tools |
| **vmdk** | VMware's format | Moving to or from VMware |
| **vhdx** / **vpc** | Hyper-V / Virtual PC formats | Moving to or from Hyper-V (convert only) |
| **qcow**, **cow**, **cloop** | Old formats | Only for old images |

**File → Disks → Create HDD Image** offers qcow2, qcow, raw, vmdk, cow and cloop. **Convert disk…** offers qcow2, raw, vmdk, vhdx and vpc.

> Saved state and **Manage Snapshots** need **qcow2**. See [Snapshots](snapshots.md).

## 2. Disks in the New VM Wizard

On the wizard's **Disk** page, choose **Create a new disk image** (pick a size) or **Use an existing disk image**. The install media page offers **Local ISO / image file**, **Download ISO from URL**, or **Network install (kernel + initrd URLs)**.

If you already have a disk, use the wizard's **Import Existing Disk** method instead (qcow2, raw, VHDX, VMDK…). See [New VM Wizard](new-vm-wizard.md).

## 3. The Media tab

Select a VM, then open **Media**. It has these sub-tabs:

| Sub-tab | What's there |
| --- | --- |
| **Device Manager** | The VM's hard disks, CD/DVD drives and floppies. **Add HDD**, **Add CD/DVD-ROM**, **Add Floppy**, **Add Storage Device**, **Attach Remote SAN Storage (iSCSI / NVMe-oF)…**, **Delete**, **Properties**, **Format HDD** and **Quick Format**. |
| **Folder Sharing** | Shared folders (Linux hosts). See [File transfer](file-transfer.md). |
| **Computer Ports** | Serial (COM), parallel (LPT) and USB ports |
| **Boot Kernel** | Boot a Linux or Multiboot kernel directly: **Kernel bzImage file**, **Initrd file**, **Append command line** |
| **Special Images** | Onboard flash, SecureDigital card, parallel flash and ROM images (mostly for ARM boards) |

> **Careful:** **Quick Format** asks "Format Virtual Hard Disk?" and **Format HDD** recreates the image. Both wipe what's on that disk.

While the VM runs, swap CDs and floppies from the [session toolbar](session-toolbar.md).

## 4. Disk interface (the "bus")

On the Machine tab, **Disk bus (HDA) → Interface** sets how the main disk is connected: **VirtIO**, **VirtIO SCSI**, **SCSI**, **IDE**, **AHCI (SATA)**, **SD** or **NVMe**.

The wizard picks one for your guest. Some examples from the source:

- Windows 11 and Server 2025 get **NVMe**.
- Vista, 7, 8, 10 and Server versions get **SATA (AHCI)**.
- Windows 9x/XP, DOS and OS/2 use **IDE**, because their setup programs can't see the newer disk types.
- Linux, BSD, Haiku, Android, ChromeOS, ARM and s390x guests get **VirtIO**.
- HP-UX gets **SCSI** and AIX gets **VirtIO SCSI**.
- Anything else falls back to **IDE**.

If the installer says "no hard disk found", the guest can't see the bus. Switch to **IDE** or **AHCI (SATA)** and try again.

## 5. Disk tools (File → Disks)

| Menu item | What it does |
| --- | --- |
| **Storage Browser…** | "Browse disk images and ISOs in your VM folder" |
| **Create HDD Image** | Makes a new image. **Use Base HDD Image** makes it a difference image on top of an existing one ("QEMU reads the base file and writes changes into the new file"). |
| **Convert HDD Image** | Converts an image, with optional compression |
| **Convert disk…** | Converts with `qemu-img convert` to qcow2, raw, vmdk, vhdx or vpc, with an optional 4K alignment |
| **qemu-nbd…** | Shares a disk image read-only on `127.0.0.1:10809` until you close the window |

The **VM → Lab** menu has more tools for power users: **Chain Studio…** (view a backing chain, create an overlay, commit, rebase), **Block jobs…** and **Linked clones…**.

## 6. Remote and network storage

- **VM → Storage → Attach Remote SAN Storage (iSCSI / NVMe-oF)…** connects a network disk to the selected VM.
- The **Storage** tab has **Enable TrueNAS Guest Integration**: enter your TrueNAS host's address, port and API token to see its pools. See also the [TrueNAS guide](../guest-os/truenas.md).

## 7. Getting data off a broken disk

**VM → Recovery → Storage Recovery…** reads disk labels and builds a **read-only** recovery VM. Add disk files, a folder or physical disks, pick a boot ISO, and click **Analyze**. The wizard has the same thing as its **Recover storage** method ([details](new-vm-wizard.md#7-recover-storage)).

---

[Documentation Hub](../../README.md) · [Snapshots](snapshots.md) · [New VM Wizard](new-vm-wizard.md)
