# First Start

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

The first time you open AQEMU, the **First Start Wizard** runs. It has four pages and takes about a minute. You can run it again at any time from **File → First Start Wizard**.

---

## Page 1: Welcome

Choose the **interface language**, then click **Next**.

## Page 2: Virtual Machine Storage

Choose the folder where AQEMU keeps your virtual machines. Each VM is saved there as a `.aqemu` file.

- **Windows:** the default is a folder under your AppData. AQEMU never stores VMs next to the program itself.
- **Linux:** the default is `~/.aqemu/`.

Pick a drive with plenty of space. Virtual disks get big (a Windows 11 VM wants 40–64 GB).

## Page 3: QEMU Emulator Setup

Tell AQEMU which QEMU to use:

| Option | When to pick it |
| --- | --- |
| **Use the QEMU binaries that ship with AQEMU (Recommended)** | Almost always. This is the QEMU that was tested with your AQEMU. |
| **Use system-installed QEMU** | On Linux, if you installed QEMU with your package manager (`qemu-system` on Debian/Ubuntu) and want to use that. |
| **Specify a custom QEMU executable or folder** | You built QEMU yourself or keep several versions. Use **Browse File** or **Browse Folder**. |

The **Advanced configuration...** button opens the full emulator settings if you need them.

> The Microsoft Store version always uses the QEMU inside its package.

## Page 4: Ready to Start

Click **Finish**. The main window opens with an empty VM list.

If you cancel the wizard, AQEMU warns that you can configure it later. (The warning says *File → Settings*; the menu item is actually **File → Preferences…**.)

---

## Changing things later

Everything from the First Start Wizard lives in **File → Preferences…**. See [Preferences](../../reference/settings/preferences.md).

Next: [Create your first VM](your-first-vm.md) · [Documentation Hub](../../README.md)
