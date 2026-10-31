# AQEMU on Windows on ARM (ARM64)

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This page is for ARM64 Windows PCs, such as Snapdragon laptops. AQEMU has a native ARM64 build, so it doesn't run under x64 emulation.

---

## Get AQEMU

**Recommended:** [AQEMU on the Microsoft Store](https://apps.microsoft.com/detail/9P0HGKHQ9W19?cid=docs) (£12.99, 7-day trial). See [Install from the Microsoft Store](../getting-started/install-from-store.md).

**From source:** `scripts/build_aqemu_woa.ps1` builds native ARM64 AQEMU in the **MSYS2 CLANGARM64** environment (same switches as the x64 script: `-InstallDeps`, `-SkipSpice`, `-BuildMsix`). QEMU for ARM64 Windows is built with `scripts/build_qemu_win_arm64.ps1`.

## Turn on WHPX

Same as on x64: enable **Hyper-V**, **Virtual Machine Platform** and **Windows Hypervisor Platform** in Windows Features, restart, then check **Help → Diagnostics → WHPX health…**.

## Which guests are fast?

| Guest | Accelerator |
| --- | --- |
| **ARM64** guests (Windows 11 ARM, Ubuntu ARM64, Debian ARM64…) | **WHPX** (fast) |
| 32-bit ARM guests (`qemu-system-arm`) | **TCG**. AQEMU treats the `arm` binary as TCG only. |
| **x86 / x86_64** guests (normal Windows, DOS, x86 Linux…) | **TCG** only (emulated, slow) |

- For ARM64 guests under WHPX, AQEMU uses the CPU model `max`. Under TCG it uses `max,pauth-impdef=on`.
- AQEMU's "WoA x86 via TCG" lab profile says it plainly: "Windows on ARM runs this x86 guest with TCG. WHPX is not offered."

**Tip:** on an ARM laptop, pick the **ARM64** version of your guest OS whenever one exists.

## Windows 11 ARM

The **Machine** tab has a **Windows 11 ARM** panel with three modes:

| Mode | What it does |
| --- | --- |
| **Install Windows** | ramfb only, USB installer ISO, CD boots first |
| **First boot** | virtio-gpu only, no installer ISO, boots from disk, `cache=none` |
| **Normal** | daily use: virtio-gpu only, no installer ISO, boots from disk |

There's also an **OOBE / UCPD…** helper. The full walkthrough is in [Windows 11 and Legacy Windows](../windows-11.md#1-windows-11-on-arm).

## Things that differ on Windows on ARM

- **Audio:** AQEMU's tooltip says: "Windows ARM has no safe speaker driver in this QEMU, so the card stays quiet instead of failing the launch." Expect no guest sound.
- **Shared folders** need a Linux host; use **Paste files** instead ([File transfer](../features/file-transfer.md)).
- **KVM through WSL** works the same way as on x64 if your WSL setup provides `/dev/kvm`. See [Windows x64](windows-x64.md#optional-kvm-through-wsl).

---

Other hosts: [Windows x64](windows-x64.md) · [Linux x64](linux-x64.md) · [Linux ARM64](linux-arm64.md) · [Documentation Hub](../../README.md)
