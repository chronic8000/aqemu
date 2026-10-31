# iOS and Apple SoC (Experimental)

> **Experimental research tooling.** This is not a supported or announced AQEMU feature, and it is not an "iPhone simulator". AQEMU drives the third-party [ChefKiss Inferno](https://chefkiss.dev/guides/inferno/file-setup/) QEMU fork (`qemu-system-applesoc`). It's slow (TCG only), it needs a lot of manual preparation, and it may not reach the home screen on your setup. AQEMU **does not ship** IPSWs, kernels, DeviceTrees, tickets or any Apple firmware. You bring those, and you're responsible for following Apple's licence terms.

The step-by-step walkthrough, with every field explained, is in [`extras/Inferno/iOS_Installation.md`](../../extras/Inferno/iOS_Installation.md). This page is the overview: what exists in AQEMU and where to find it.

---

## 1. What you need

| Item | Notes |
| --- | --- |
| **Windows 10/11 + WSL Ubuntu**, or Linux | On Windows, AQEMU always runs `qemu-system-applesoc` through **WSL** |
| **Inferno `qemu-system-applesoc`** | Installed in Linux or WSL |
| **A companion Linux VM** | Runs `idevicerestore` to restore the IPSW. See step 3. |
| **Your IPSW + extracted firmware** | The tooling targets the **iPhone 11 (t8030, n104ap)**. Tickets, SEP firmware and the IPSW must all come from the **same build**. |

---

## 2. Create the iOS VM

**Guest Operating System → Apple → iOS (ARM64)**. The wizard sets:

- Emulator `qemu-system-applesoc`, machine **`t8030`**, CPU `max`
- At least **4096 MB** RAM, **4** vCPUs, no PC VGA, a USB tablet
- USB bridge: `127.0.0.1:8030` on Windows (a UNIX socket on Linux)
- **NAND (root) size**: 16 / 32 / 64 / 128 / 256 GiB, or Custom (16 to 2048 GiB). The default is **32 GiB**.

## 3. Create the restore companion

**Guest Operating System → Apple → iPhone IPSW Restore Companion** (also listed under Linux as *IPSW Restore Companion (Ubuntu)*). This makes a normal AQEMU VM: Ubuntu Server, 2048 MB, 20 GB, `companion.qcow2`. When you click Finish, AQEMU downloads Ubuntu Server (or uses your ISO). You can SSH in with:

```
ssh -p 32222 user@127.0.0.1
```

---

## 4. The Apple tools (`VM → Apple`)

| Menu item | Window | What it's for |
| --- | --- | --- |
| **iOS Firmware Tool…** | *iOS Firmware Tool* | **Unpack IPSW**, **Forge restore + SEP tickets**, **Decrypt + pack SEP firmware**, **Run pyimg4**, **Copy locations** |
| **Apple SoC Restore…** | *Apple SoC Restore (Inferno companion)* | **Apply localhost preset (127.0.0.1:8030)**, **Start companion in WSL** / **Stop companion**, **Diagnose USB bridge**, **Restore IPSW via SSH…** |
| **Apply iOS filesystem patches…** | | Patches the guest `root` disk after a restore (needed or the screen stays black) |
| **Guest Internet / iOS Device Tools…** | *iOS Device Tools / Guest Internet* | **Internet** tab: **Enable guest internet (reverse-tether)**. **Device** tab: **Device info**, **List apps**, **Install IPA...**, **Screenshot** |
| **Wipe Inferno disks…** | | Start over with clean NAND images |

> Some older text (the wizard's help panel and `extras/Inferno/iOS_Installation.md`) still says **File →** for these items. They now live under **VM → Apple**.

## 5. The order of operations

1. **iOS Firmware Tool:** unpack your IPSW, forge the restore + SEP tickets, and pack the SEP firmware.
2. Fill in the firmware paths on the VM's Machine settings, then grow `root` to the size you want **before** the first restore.
3. **Apple SoC Restore:** set the companion disk and SSH login, click **Start companion in WSL**, and wait for SSH on port 32222.
4. Start the iOS VM. You should see the restore screen, not the home screen.
5. Click **Restore IPSW via SSH…**. When the restore finishes, QEMU exits. That's normal.
6. Power off and run **Apply iOS filesystem patches…** on the guest `root` disk (not the companion).
7. Power on again.

> **Build mismatch warning.** If the IPSW is iOS 14.0 beta 5 (`18A5351d`) but your tickets and SEP pack were made from a different build, the restore fails after NORData with `-256`. AQEMU warns you about this.

## 6. Hardware buttons

While the VM runs, click **Pad** on the session toolbar ("Show / hide floating iOS button pad") to open the **iOS buttons** window:

| Button | Inferno key | Action |
| --- | --- | --- |
| **Vol−** | F3 | Volume down |
| **Vol+** | F4 | Volume up |
| **Side** | F5 | Click: sleep/wake. Hold about 2 s: power menu |
| **Home** | F6 | Click: Home screen. Double-click: App Switcher |
| **SOS** | | The Vol-up + Side combo. Use it on the Welcome / "Swipe up" screen. |

---

[Documentation Hub](../README.md) · [Inferno file setup](https://chefkiss.dev/guides/inferno/file-setup/) · [Inferno filesystem patches](https://chefkiss.dev/guides/inferno/fs-patches/)
