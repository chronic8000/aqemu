# Windows 11 on ARM, host by host

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This guide covers running **Windows 11 for ARM64** on each of the four AQEMU hosts. For the Install / First boot / Normal modes and the OOBE helper, see the main [Windows 11 guide](../windows-11.md#1-windows-11-on-arm).

> You need your own Windows 11 ARM64 ISO and licence. AQEMU doesn't ship Windows.

## At a glance

| Host | Accelerator | CPU AQEMU passes | Speed |
| --- | --- | --- | --- |
| **Windows ARM64** | WHPX | `-cpu max` | Hardware-accelerated |
| **Linux ARM64** | KVM | `-cpu host` | Hardware-accelerated |
| **Windows x64** | TCG | `-cpu max,pauth-impdef=on` | Emulated, slow |
| **Linux x64** | TCG | `-cpu max,pauth-impdef=on` | Emulated, slow |

Why: hardware acceleration only works when the guest CPU family matches the host. An ARM guest on an x64 PC is always full emulation. On Windows on ARM, AQEMU deliberately uses `max` rather than `host` with WHPX; a comment in the source says `-cpu host` on Insider WHPX-ARM builds "has been tied to host crashes". Under TCG, AQEMU adds `pauth-impdef=on` because plain `max` is "too slow under TCG".

## What every host gets

These come from the **Windows 11 ARM** template:

| Setting | Value |
| --- | --- |
| Emulator / machine | `qemu-system-aarch64`, `virt` |
| vCPUs / memory | 4 / 8192 MB |
| Display | `ramfb` while installing, VirtIO GPU after |
| Interrupt controller | GICv3. With WHPX AQEMU uses `gic-version=3,msi=gicv2m` (WHPX can't do a GICv3 ITS); otherwise `gic-version=3,highmem=on,its=on`. |
| Firmware | AAVMF UEFI with a private VARS file per VM |
| TPM | **TPM 2.0 (start swtpm with this VM)**, ticked and available only when `swtpm` is found on the host |

## Before you start (all hosts)

1. Get a Windows 11 **ARM64** ISO. It's best to slipstream the ARM64 VirtIO drivers into it. If you don't, tick **Also attach virtio-win.iso (only if drivers were not slipstreamed)** on the wizard's install page.
2. **VM → Add VM → Add New VM** (`Ctrl+N`).
3. Either:
   - **Custom / Advanced → Template → Windows 11 ARM**, or
   - on an ARM64 host, **Guest Operating System → Microsoft → Windows 11**. The wizard says: "Windows 11 on ARM host → AArch64 + virt (VirtIO). Architecture remains editable."
4. On the **Windows 11 ARM Install** page, pick **Create a new disk image** or **Use an existing disk image**, choose your **Windows 11 ARM ISO**, and finish.
5. The VM starts in **Install Windows** mode. After Setup, switch to **First boot**, then **Normal** (Machine tab).

---

## Windows ARM64 (Snapdragon and other Windows-on-ARM PCs)

**Accelerator:** WHPX. **CPU:** `max`.

1. Make sure Windows Hypervisor Platform is on. AQEMU's **WHPX health** check offers to fix it (see [Accelerators](../features/accelerators.md)).
2. Create the VM as above. With WHPX selected, AQEMU keeps the CPU at `max`.
3. Start. If WHPX can't start, AQEMU offers to run with TCG instead (much slower).

**Limitations:**
- The source flags `-cpu host` with WHPX on ARM as crash-prone, so don't change the CPU to `host`.
- Audio: the audio tooltip says "Windows ARM has no safe speaker driver in this QEMU", so the guest sound card stays quiet.
- TPM needs `swtpm` installed on Windows. Without it the box is greyed out and the VM still boots.

See also: [Windows ARM64 host guide](../hosts/windows-arm64.md).

## Linux ARM64 (Raspberry Pi 5, ARM servers)

**Accelerator:** KVM. **CPU:** `host`.

1. Make sure `/dev/kvm` exists and your user can open it (see [Linux ARM64 host guide](../hosts/linux-arm64.md)).
2. Create the VM. On an ARM64 Linux host the wizard selects KVM and sets the CPU Type to `host`.
3. Start.

**Limitations:**
- 8 GB of guest RAM is the template default. On a board with 8 GB or less, lower it on the Machine tab.
- If `/dev/kvm` isn't usable, the VM runs under TCG and is far slower.

## Windows x64 (Intel/AMD PCs)

**Accelerator:** TCG. **CPU:** `max,pauth-impdef=on`.

1. Create the VM as above. WHPX can't run an ARM guest on an x64 CPU, so AQEMU uses TCG with `-accel tcg,thread=multi,tb-size=1024`.
2. Start, and be patient. Setup and OOBE take far longer than on an ARM host.

**Limitations:** full emulation. It's fine for testing, not for daily use. If you want Windows 11 on this PC, use the [x86_64 Windows 11 guide](../windows-11.md#2-windows-11-on-x86_64) with WHPX instead.

## Linux x64 (Intel/AMD PCs)

**Accelerator:** TCG. **CPU:** `max,pauth-impdef=on`.

1. Create the VM. AQEMU uses TCG for the same reason as on Windows x64.
2. Start, and be patient.

**Limitations:** full emulation, as on Windows x64. KVM on this host is for x86 guests only.

---

Related: [Windows 11 guide](../windows-11.md) · [Windows guests](windows.md) · [Guest OS index](README.md)
