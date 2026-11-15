# Snapshots, saved state and checkpoints

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

AQEMU has four different "undo" features. They are easy to mix up, so here they are side by side.

| Feature | Where | What it does | Needs |
| --- | --- | --- | --- |
| **Save VM state** | Session toolbar, or **VM → Save** | Saves the running guest's memory and stops it. The next **Start** resumes exactly where you were. | A **qcow2** disk |
| **Manage Snapshots** | **VM → Storage → Manage Snapshots** | Lists the snapshots inside the VM's disk. Start the VM from one, or delete one. | A **qcow2** disk ("This feature is supported on QCOW2 images only") |
| **Snapshot mode** | Machine tab → **Options** → **Snapshot mode** | Writes changes to temporary files. When the VM stops, the disk is exactly as it was. | Any disk |
| **Recovery Checkpoint** | **VM → Recovery → Recovery Checkpoint…** | Puts a qcow2 overlay in front of each disk. Later you **Discard** (throw the changes away) or **Keep** them. | File-backed disks |

---

## Save VM state (suspend)

1. With the VM running, click **Save VM state** on the session toolbar.
2. AQEMU saves the guest's state inside the disk image (under the name `aqemu_save`) and the VM stops.
3. Click **Start** later. AQEMU loads the saved state and the guest carries on.

If you see "You must add HDD image in QCOW2 format to save virtual machine!", the VM's disk isn't qcow2. Convert it with **File → Disks → Convert HDD Image** (see [Storage and disks](storage-and-disks.md)).

**VM → Save** does not work while **Snapshot mode** is on. AQEMU says "QEMU running in snapshot mode. VM can not be saved in this mode."

## Manage Snapshots

Open **VM → Storage → Manage Snapshots**. The window lists the snapshots stored in the VM's qcow2 disk, with ID, size, date and VM clock.

- **Start** starts the VM from the selected snapshot. If the VM is already running, AQEMU asks: "This VM is Running now! Stop VM and Load Snapshot?"
- **Delete** removes the selected snapshot (the VM must be running).

> **v1 note:** in this source the **Create** button in this window doesn't save a new snapshot yet (the code that would do it is marked to-do). Use **Save VM state** instead.

## Snapshot mode (throw-away sessions)

Tick **Snapshot mode** on the Machine tab. QEMU writes to temporary files instead of your disk image, so nothing you do in the guest is kept after it stops. Great for testing something risky. Untick it to go back to normal.

## Recovery Checkpoint

**VM → Recovery → Recovery Checkpoint…** says: "A recovery checkpoint puts a qcow2 overlay in front of each source disk. Original source disks stay the backing files and are not modified."

- **Enable Recovery Checkpoint** creates the overlays. The VM now writes to them.
- **Discard** "deletes the recovery overlays. Original source disks are unaffected." Use it to throw away everything since the checkpoint.
- **Keep** makes the overlays the VM's disks for good. The originals stay unchanged.

---

[Documentation Hub](../../README.md) · [Storage and disks](storage-and-disks.md) · [Session toolbar](session-toolbar.md)
