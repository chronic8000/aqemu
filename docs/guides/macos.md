# macOS (Experimental)

> **Experimental.** Intel macOS in AQEMU is an experimental profile, and the app labels it that way. It is not a supported or announced feature, and it may not boot on your hardware. Classic Mac OS on PowerPC uses standard QEMU emulation. AQEMU doesn't ship any Apple software, firmware or keys. You supply your own, and you're responsible for following Apple's licence terms.

AQEMU has two very different Mac paths:

1. **Classic Mac OS on PowerPC** (Mac OS 7 to 9, Mac OS X PPC), using `qemu-system-ppc`.
2. **Intel macOS (experimental)**, using `qemu-system-x86_64` + `q35` with OpenCore.

---

## 1. Classic Mac OS (PowerPC)

### What you need

- A QEMU build that includes `qemu-system-ppc`. If **Start** can't find the binary, install one and run the First Start Wizard again.
- Your own Mac OS install CD/ISO, or a prepared disk image.

### Create the VM

**Guest Operating System → Apple → Mac OS 9** (or Mac OS 7, Mac OS 8, Mac OS X PPC). Or use the **Mac OS Classic PPC** template under **Custom / Advanced → Template**.

| | Mac OS 9 profile | Mac OS X PPC profile | Mac OS Classic PPC template |
| --- | --- | --- | --- |
| Machine | `mac99` (or `g3beige`) | `mac99` (or `g3beige`) | `mac99` |
| CPU | G4 | G4 | default |
| RAM | 256 MB | 512 MB | 512 MB |
| Disk | 4 GB, IDE | 10 GB, IDE | 10 GB |
| Network | sungem | sungem | sungem |

When we ran AQEMU's argument builder on the template, it produced `-machine mac99 -accel tcg,thread=single -m 512 -net nic,model=sungem`. PowerPC is always emulated (TCG).

Attach your install media on the **Media** tab and click **Start**.

---

## 2. Intel macOS (experimental)

### What you need

| Item | Notes |
| --- | --- |
| **OpenCore** | A bootable OpenCore ISO or disk image you prepare yourself |
| **Installer image** | A macOS installer disk image (for example one made with MIST) |
| **Apple SMC OSK** | The 64-byte SMC key. AQEMU **never pre-fills it** and won't start this profile without one. Get it from Mac hardware you own. |
| **OVMF** | UEFI firmware (`OVMF CODE` / `OVMF VARS`) |
| **Acceleration** | Strongly recommended: KVM on Linux, or on Windows **Launch via WSL/KVM** ("Run Linux QEMU inside WSL with KVM when available") |

### Create the VM

**Guest Operating System → Apple → macOS** (or **Mac OS X Intel**). The wizard opens the **Intel macOS Setup** page:

- **OpenCore (ISO or disk image)**
- **System disk:** **Create a new disk image** or **Use an existing disk image**
- **Installer disk image (MIST APM/HFS .iso)**
- **Apple SMC OSK (required to start)**: paste your own key

The profile uses `q35`, 4096 MB RAM, a 40 GB AHCI (SATA) disk, an `e1000-82545em` NIC, and VMware SVGA graphics at the host's native resolution (see [Intel macOS graphics](../reference/intel-macos-graphics.md)).

After the VM is created, these settings live on the Machine tab's **Intel macOS** panel (**OpenCore ISO/disk**, **Installer image**, **Apple SMC OSK**, **Launch via WSL/KVM**) and in **Machine → Advanced** under **Enable Intel macOS profile (q35 + OpenCore + SMC)**.

### What AQEMU sets automatically

You don't need to pick a CPU model or add SMC flags yourself. With the Intel macOS profile on, we ran AQEMU's argument builder and got:

```
-cpu Skylake-Client-v4,vendor=GenuineIntel,+invtsc,tsc-frequency=2000000000
-device isa-applesmc,osk=<your key>
-smbios type=2
```

- AQEMU escapes any commas in the OSK for you. It doesn't check the key's length or contents, so paste it exactly.
- When QEMU runs inside WSL with KVM, AQEMU also adds `kvm=on,vmware-cpuid-freq=on` to the CPU.
- Under pure TCG, QEMU warns that `invtsc` isn't supported. Expect TCG to be too slow for real use.

### Install

1. Click **Start**. OpenCore's picker should appear.
2. Choose the installer, use Disk Utility to erase the system disk as **APFS**, then install.
3. After the reboots, choose the system disk in OpenCore.

This is experimental. If it doesn't boot, please open an [issue](https://github.com/chronic8000/aqemu/issues) with your QEMU arguments (`VM → Tools → Show QEMU Arguments`), with the OSK removed.

---

[Documentation Hub](../README.md) · [User Guide](user-guide.md)
