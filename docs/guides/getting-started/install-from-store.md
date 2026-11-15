# Install AQEMU from the Microsoft Store

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

The easiest way to get AQEMU on Windows is the Microsoft Store. The Store package includes everything you need: AQEMU itself and the QEMU emulators it drives. You don't need to install QEMU separately.

**[Get AQEMU on the Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs)**

---

## Price and trial

- AQEMU is a **paid app** on the Microsoft Store (**£12.99** in the UK Store; the Store shows the price for your region).
- You can try it first with a **7-day trial**.
- While the trial runs, the status bar at the bottom of the main window shows **Trial: _N_ days left** (or **Trial ends today** on the last day).
- When the trial has ended, AQEMU shows a box titled **AQEMU trial**: "Your 7-day trial has ended. Your virtual machines stay on this computer. You can still open this window and copy them." The buttons are **Buy AQEMU**, **Continue** and **Quit**.
- You can buy at any time from **Help → Buy AQEMU**, which opens AQEMU's page in the Microsoft Store app.

## Install

1. Open the [Store page](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs) and click the Store button to install it or start the trial.
2. When the install finishes, start **AQEMU** from the Start menu.
3. The **First Start Wizard** opens. Follow [First start](first-start.md).

## What is different about the Store version

The Store version is built with a "Store channel" switch in the source. It behaves a little differently from a version you compile yourself:

| | Store version | Self-built version |
| --- | --- | --- |
| QEMU | Always uses the QEMU shipped inside the package | Bundled QEMU, system QEMU, or a custom folder |
| Searches `PATH` / MSYS2 for QEMU | No | Yes |
| Settings and VM folder | Its own (`aqemu-store`), under your AppData folder | `aqemu` |
| **Help → Buy AQEMU** and trial status | Yes | No |
| Nitro Enclave, SEV-SNP and TDX profiles | Hidden | Shown |

If you used a self-built AQEMU before, the Store version reuses your old VM folder on first start (as long as it's a normal data folder), but it does **not** copy emulator binaries.

## Which PCs can run it?

AQEMU runs on **Windows x64** and **Windows on ARM (ARM64)**. See the host guides:

- [Windows x64 host](../hosts/windows-x64.md)
- [Windows on ARM (ARM64) host](../hosts/windows-arm64.md)

On Linux, AQEMU is built from source. See [Linux x64](../hosts/linux-x64.md) and [Linux ARM64](../hosts/linux-arm64.md).

## Bring your own operating systems

AQEMU doesn't ship operating systems, Windows licences, Apple software or console ROMs. You supply your own installer ISOs and licences.

---

Next: [First start](first-start.md) · [Documentation Hub](../../README.md)
