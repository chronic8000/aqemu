# Troubleshooting and FAQ

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Something not working? Find the message or symptom below. Messages in quotes are copied from AQEMU itself.

---

## 1. "Cannot start emulator!"

> "Cannot start emulator! AQEMU did not find %1 next to the app. This package uses the QEMU shipped with AQEMU."

The QEMU program for this guest (the `%1` part, for example `qemu-system-x86_64`) is missing from AQEMU's install folder. On Windows, repair or reinstall AQEMU from the Microsoft Store.

> "Cannot start emulator! Binary path is empty for '%1'! Please check binary path under Advanced Settings."

AQEMU doesn't know where that QEMU program is. Open **File → Preferences…** and check the **Emulator** tab. See [Preferences](../../reference/settings/preferences.md).

## 2. Black screen, or "Guest Display Failure"

- **"The virtual machine exited before the guest display was ready"**: QEMU stopped straight away. Open **VM → Tools → Show QEMU Error Log Window** to see why. Common causes are a missing ISO or disk file, or a setting the board doesn't support.
- **"Falling back to VNC display…"**: the embedded viewer couldn't use SPICE, so it switched to VNC. You don't need to do anything.
- **Old Windows (XP family) black after "inspecting hardware"**: the source notes that Cirrus graphics with SPICE turns black here. Use **standard VGA** with a separate QEMU window. The wizard does this for you, and **Auto** in **How to show this VM** opens legacy guests in a separate window. See [Display](display.md).
- **Still stuck?** Set **How to show this VM** to **Separate QEMU window — SDL / GTK** and try again.

## 3. It's very slow (Windows host)

AQEMU uses **WHPX** (Windows Hypervisor Platform) to run guests fast when the guest and host have the same kind of CPU. Without it, everything runs in **TCG** (software emulation), which is much slower. See [Accelerators](accelerators.md).

Open **Help → Diagnostics → WHPX health…**. It runs QEMU's accelerator check and reads the Windows virtualization-based security setting. It has buttons for **Optional Features** (Hyper-V, Virtual Machine Platform, Windows Hypervisor Platform) and **Core isolation** (Memory Integrity).

If the last check failed, AQEMU asks at Start:

> "The last Hyper-V / WHPX probe failed. Start with Force TCG, or start anyway."

Choose **Force TCG** to start slowly but safely, or **Start anyway**.

Some guests always use TCG, for example Intel guests on an ARM64 PC, all 32-bit (`qemu-system-i386`) and 32-bit ARM guests, and anything the wizard marks **Force TCG** (like ReactOS).

## 4. The installer can't find a hard disk

The guest has no driver for the disk type. On the Machine tab, set **Disk bus (HDA) → Interface** to **IDE** (oldest guests) or **AHCI (SATA)**, then start again. See [Storage and disks](storage-and-disks.md#4-disk-interface-the-bus).

## 5. No network in the guest

- Check the **Network** tab: **Enable network** must be ticked.
- Try a different card model that the guest has a driver for. See [Networking](networking.md#3-which-network-card).

## 6. My mouse is stuck inside the guest

Press **Esc** (or **Ctrl+Alt**). In a separate QEMU window the default is **Ctrl+Alt**, or **Ctrl+Alt+Shift** if you ticked that option. To avoid this, set **Pointer mode** to **Seamless**. See [Display](display.md#2-mouse-seamless-or-relative).

## 7. Snapshots don't work

**Save VM state** and **Manage Snapshots** need a **qcow2** disk. Convert raw disks with **File → Disks → Convert disk…**. See [Snapshots](snapshots.md).

## 8. Getting help: collect the details

Everything is in the **Help** menu:

| Item | What it does |
| --- | --- |
| **Copy setup for a GitHub issue** | Puts the AQEMU, Qt, host, CPU architecture and QEMU path on your clipboard as a table, ready to paste |
| **Open crash dumps folder** | Opens the folder where AQEMU saves crash dumps |
| **Diagnostics → Support bundle…** | Saves `aqemu-support.zip` with your settings and the selected VM's QEMU command line. Lines and arguments that look like tokens, passwords or other secrets are redacted. |
| **Diagnostics → QEMU bundle auditor…** | Counts the `qemu-system` programs next to AQEMU ("expect at least 25 in a full bundle") and checks that `qemu-img` and the firmware are there |
| **Diagnostics → QEMU catalog…** | Lists the machines, CPUs, network cards and graphics cards the selected VM's QEMU program supports |

Also useful: **VM → Tools → Show QEMU Arguments** (the exact command AQEMU runs) and **Show QEMU Error Log Window**.

Report problems on [GitHub Issues](https://github.com/chronic8000/aqemu/issues). Check the bundle before you share it.

## 9. FAQ

**Is AQEMU free?**
No. AQEMU on the Microsoft Store costs £12.99 and has a 7-day free trial. [Get it from the Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs).

**What happens when the trial ends?**
AQEMU shows "Your 7-day trial has ended." with **Buy AQEMU**, **Continue** and **Quit** buttons. "Your virtual machines stay on this computer. You can still open this window and copy them." You can buy any time from **Help → Buy AQEMU**.

**Can AQEMU run macOS or iOS?**
Both are **experimental**, and we don't promise they work. You must supply any Apple software yourself. We can't distribute it or tell you where to get it. See [macOS](../macos.md) and [iOS (Apple SoC)](../ios-apple-soc.md).

**Does it run on ARM64 Windows (Snapdragon)?**
Yes. ARM64 guests (like Windows 11 ARM and ARM Linux) can use WHPX. Intel guests run in TCG. See [Windows on ARM64](../hosts/windows-arm64.md).

**Is there a WSL2 accelerator?**
No. On Windows you can choose WHPX or TCG, or choose **KVM**, which runs Linux KVM inside WSL (set it up in **File → Host → Configure WSL…**). WSL is only where KVM runs. See [Accelerators](accelerators.md).

**How many operating systems does it know?**
The wizard's OS lists name 144 operating systems, plus 69 Consoles & Retro entries and 13 Lab entries. See the [guest OS guides](../guest-os/README.md).

---

[Documentation Hub](../../README.md) · [Accelerators](accelerators.md) · [Display](display.md)
