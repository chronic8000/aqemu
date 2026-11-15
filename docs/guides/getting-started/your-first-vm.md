# Your First Virtual Machine

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This walkthrough makes a 64-bit Linux VM (Ubuntu or Debian) from start to finish. The same steps work for almost any guest; only the OS you pick changes.

**You need:** AQEMU installed ([Store](install-from-store.md) or built from source) and an installer ISO for the OS, downloaded from the OS's own website. AQEMU can also download an ISO for you from a URL you paste in.

---

## Step 1: Open the wizard

Click **Add New VM** on the toolbar, or use **VM → Add VM → Add New VM**, or press **Ctrl+N**.

## Step 2: Choose how to create it

The first page asks how you want to create the VM. Choose **Guest Operating System** and click **Next**. (The other six methods are explained in [The New VM Wizard](../features/new-vm-wizard.md).)

## Step 3: Pick the OS

The list is grouped by vendor. Open **Linux → x86_64 (64-bit PC)** and choose **Ubuntu (64-bit)** or **Debian (64-bit)**.

> On an ARM64 PC (Windows on ARM, or a Raspberry Pi 5 / ARM Linux box), pick from **Linux → ARM64 / AArch64 (Modern ARM)** instead, for example **Ubuntu (ARM64)**. A guest that matches your PC's CPU can use hardware acceleration; a guest that doesn't is fully emulated and much slower.

The wizard's starting profile for Ubuntu (64-bit) is: `x86_64` + `q35` machine, CPU `max`, **4096 MB** RAM, a **20 GB** disk, VirtIO disk and network, VirtIO VGA and Intel HDA audio. AQEMU's tip for this profile: "GNOME live/install fails on 2 GB under TCG." Keep the 4 GB.

## Step 4: Confirm the QEMU system

The **Confirm QEMU system** page shows the emulator (`qemu-system-x86_64`) and the machine type. You can leave them as they are.

## Step 5: Accelerator

Choose:

- **WHPX (native acceleration)** on Windows, or **KVM (native acceleration)** on Linux, if it's offered. This is much faster.
- **TCG (software emulation)** if nothing else is available. It always works, just slowly.

More detail: [Accelerators](../features/accelerators.md).

## Step 6: Name, CPU, disk and install media

1. **VM Name:** for example `Ubuntu`.
2. **CPU Type** and **CPU Cores:** the defaults are fine. Two to four cores is a good start.
3. **Disk:** keep **Create a new disk image** and the suggested size.
4. **Install media:** pick one:
   - **Local ISO / image file**: browse to the ISO you downloaded. AQEMU guesses the OS from the ISO's volume ID or file name.
   - **Download ISO from URL**: paste the ISO's link and click **Download**. AQEMU saves it in your VM folder and uses it as the CD-ROM.
   - **Network install (kernel + initrd URLs)**: for advanced network installs.

## Step 7: Devices and network

The **Devices** page already has the right devices for the OS you picked. Leave it.

On the **Network** page keep **User Mode / NAT (works on Windows & Linux — recommended)**. Your VM gets internet access with no setup.

## Step 8: Finish and start

Click **Finish**. Your VM appears in the list on the left. Select it and click **Start**.

The guest's screen opens inside AQEMU. Install the OS as you would on a real PC.

## Step 9: After the install

- Remove the installer ISO so the VM boots from its disk: use **Eject CD/DVD** on the session toolbar, or change it on the **Media** tab.
- Install the guest agent (`sudo apt install qemu-guest-agent` on Ubuntu/Debian) to get file copy with **To guest** / **From guest**. See [File transfer and the session toolbar](../features/session-toolbar.md).

---

## If something goes wrong

- **It's very slow:** you're probably on TCG. See [Accelerators](../features/accelerators.md).
- **"Cannot start emulator!":** AQEMU can't find the QEMU binary. See [Troubleshooting](../features/troubleshooting.md).
- **Want to see the exact QEMU command?** **VM → Tools → Show QEMU Arguments**.

Next: [Your host](../hosts/windows-x64.md) · [Guest OS guides](../guest-os/README.md) · [Documentation Hub](../../README.md)
