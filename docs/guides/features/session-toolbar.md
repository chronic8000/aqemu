# The session toolbar (while a VM is running)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

When you start a VM with the embedded display, the guest's screen takes over the AQEMU window and a toolbar appears above it. This page explains every button, from left to right.

> **Hover first.** Most buttons are icons. Hover over one to see its name. The names below are the tooltips in the app.

---

## 1. Power

| Button | What it does |
| --- | --- |
| **Pause / Resume** | Freezes the guest. Click again to carry on. |
| **Save VM state** | Saves the running guest's memory into its disk and stops it, so the next **Start** carries on where you left off. Needs a **qcow2** disk (see [Snapshots](snapshots.md)). |
| **Migrate to URI…** | Live-migrates the running guest to another QEMU (advanced). |
| **Reset guest** | Like pressing the reset button on a real PC. Unsaved work in the guest is lost. |
| **ACPI shutdown** | Asks the guest OS to shut down nicely, like a short press of the power button. Old guests without ACPI ignore it. |
| **Power off** | Pulls the plug. Use it only when the guest is stuck. |

## 2. Removable media

| Button | What it does |
| --- | --- |
| **Insert CD/DVD image…** / **Eject CD/DVD** | Swap the ISO in the virtual CD drive without stopping the VM. The tooltip shows which image is loaded. |
| **Insert floppy A image…** / **Eject floppy A** | Floppy drive A. Only shown when the VM has floppy A turned on. |
| **Insert floppy B image…** / **Eject floppy B** | Floppy drive B. Only shown when the VM has floppy B turned on. |

## 3. Files

| Button | What it does |
| --- | --- |
| **Paste files** | Copies files from your host clipboard into the guest. They appear as floppy A or a USB drive named **AQEMU**. No guest software needed. |
| **To guest** | Copies one host file to a path inside the guest. Needs `qemu-guest-agent` in the guest. |
| **From guest** | Copies one file from a path inside the guest to your host. Needs `qemu-guest-agent`. |
| **Guest tools** | Inserts the AQEMU guest tools floppy. Run the installer once inside the guest. |

Full details: [File transfer](file-transfer.md) and [Copy and paste](../copy-and-paste.md).

## 4. Apple buttons (Apple SoC VMs only)

These appear only on experimental Apple SoC (iOS) VMs: **Restore IPSW…**, **Vol−**, **Vol+**, **Home**, **Side**, **SOS**, **More…**, **Pad** and **Net**. On every other VM they are hidden, and so are floppy, **Send Ctrl+Alt+Del** and **Send Shift+F10**. See [iOS (Apple SoC)](../ios-apple-soc.md).

## 5. Devices

| Button | What it does |
| --- | --- |
| **USB devices** | A menu of the host's USB devices. Click one to connect it to the guest (it disconnects from the host); click again to give it back. **Disconnect all from guest** returns everything. AQEMU warns you before passing through a keyboard or mouse, because your host may lose that input. |
| **Serial console (guest COM / ttyS0)** | Opens the guest's first serial port. |

## 6. Keyboard, mouse and screen

| Button | What it does |
| --- | --- |
| **Send Ctrl+Alt+Del** | Sends the three-finger salute to the guest (your host keeps its own). |
| **Send Shift+F10** | Opens a command prompt in Windows Setup / OOBE. |
| **Grab mouse into guest** | Locks the mouse inside the guest. You can also just click the guest screen. Press **Esc** or **Ctrl+Alt** to let go. |
| **Fullscreen** | Fills the screen. Hold the mouse at the top centre of the screen to bring the toolbar back. |
| **Back to VM list** | Returns to the main window. **The guest keeps running.** Use **VM → Connect** (`Ctrl+Shift+C`) to get back to it. |

## 7. Drive lights

On the right, **Drives** shows little activity lights for floppy A, floppy B, the CD and the hard disk, like an old PC case. Hover over them to see which image is in each drive.

---

## Tips

- **Seamless mouse:** if you'd rather not grab the mouse at all, set **Pointer mode** to **Seamless** on the Machine tab. See [Display](display.md).
- **Guest won't shut down?** Try **ACPI shutdown** first, wait a minute, and only then use **Power off**.
- **No toolbar?** You are probably using a separate QEMU window instead of the embedded display. See [Display](display.md).

---

[Documentation Hub](../../README.md) · [File transfer](file-transfer.md) · [Snapshots](snapshots.md) · [Display](display.md)
