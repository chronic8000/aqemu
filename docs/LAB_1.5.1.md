# AQEMU 1.5.1 QEMU Lab

This page is the feature guide for **AQEMU 1.5.1**. Packages built from this tree use `VERSION.txt`, which is `1.5.1` (Store MSIX `1.5.1.0`).

Older releases stay in the root [`CHANGELOG`](../CHANGELOG). This file does not replace that history.

AQEMU does not ship Xbox firmware, Android images, IPSW files, OpenCore, macOS media, or an Apple OSK. Bring-your-own pages only point at files you already have.

## Where to find it

| Place | What it does |
|--|--|
| **VM → Lab** | Per-VM networking, storage, debug, device, display, and firmware controls |
| **Network** page | Port-forward table. SSH, RDP, and HTTP presets. Saves `hostfwd=` rules |
| **VM → Manage Snapshots** | Timeline: overlay that becomes the disk, internal `savevm`, and the classic snapshot list |
| **File** menu | Convert disk, Chain Studio, lab packs, linked clones, qemu-nbd |
| **Help** menu | Support bundle, QEMU catalog, bundle auditor, WHPX health |
| **New VM** wizard | First-run cards, extra guest profiles, Windows 11 ARM TPM and Secure Boot |

## Networking

The NIC page is a port table: protocol, host port, guest IP, and guest port. A saved rule such as `tcp::2222-10.0.2.15:22` reloads into those four columns. Duplicate host ports are noted. User-net air-gap (`restrict=on`) sits with the existing SMB fields.

Lab also has multi-NIC templates, and TAP or bridge notes. Linux can use `ip tuntap`. Windows looks for an OpenVPN TAP adapter and does not invent one.

## Storage

Disk IOPS and BPS are written on `-drive` as `throttling.iops-total` and `throttling.bps-total`. Cache and aio presets show the flags they will add.

**File → Convert disk** reuses the existing 4Kn control for qcow2, raw, vmdk, vhdx, and vpc.

**Chain Studio** shows the backing graph and can overlay, rebase, or commit. A broken open chain blocks start.

**Manage Snapshots** creates a qcow2 overlay and points HDA at it after `qemu-img` succeeds. Internal snapshots send `savevm` over QMP while the VM is running. The classic snapshot list is still on that dialog.

qemu-nbd export refuses a running read-write disk unless you take a snapshot first. Dirty bitmaps and block jobs (stream, commit, mirror, cancel) use the existing QMP client.

LUKS passphrases stay out of the VM file. Windows keeps them in Credential Manager and writes a secret file that only the current user can read, because QEMU needs `-object secret,file=`. Linux stores that file mode `0600`.

## Debug, launch, and firmware

- GDB stub (`-gdb tcp:127.0.0.1:port` / `-S`) and a guest memory dump through QMP `dump-guest-memory`. The copied `target remote` command uses that same localhost address.
- Record/replay with an icount shift. Choosing Off clears `-icount`.
- Command diff of the live argv, a QMP scratchpad, and an additional-arguments snippet list.
- fw_cfg is a name, kind, and file-or-string table. Values with more than one line round-trip.
- Sandbox presets are Off, Recommended, and Paranoid. On Windows the panel says sandbox is Linux-only.
- Direct boot fields for kernel, initrd, append, and dtb. `aarch64=off` is available for 32-bit ARM guests on `qemu-system-aarch64`.
- The firmware library clones a private OVMF VARS file per VM. Windows 11 ARM always gets that private VARS file. Secure Boot selects a secboot firmware image when one is installed beside the CODE file. If `swtpm` is installed, the TPM checkbox starts `swtpm socket` with the VM. A leftover control socket is removed first, so the next start launches swtpm again. The helper stops if start fails. qemu-nbd exports read-only on `127.0.0.1:10809` and stops when its window closes. The isolated socket NIC listens on `127.0.0.1:1234`.

**Help → Support bundle** writes a zip with the version, redacted settings, the QEMU command, and `qemu-boot.log`. Arguments that contain OSK, IPSW, secret, token, or password text are replaced with `[redacted]`.

## Devices, display, and shares

Shared folders stay on the existing `-virtfs` path and add mount tag, security model, and read-only. On Linux, virtiofs is used when `virtiofsd` is installed. A read-only share passes `--readonly` to virtiofsd. QEMU 11's `vhost-user-fs-pci` has no readonly property, so the device line stays unchanged. That path adds a shared-memory backend (`share=on`). If the VM already uses NUMA, those NUMA nodes become `memory-backend-memfd` with `share=on` instead of a second memory topology. Windows hosts stay on 9p and say so.

VirGL on Linux rewrites `virtio-vga` to `virtio-vga-gl` and `virtio-gpu-pci` to `virtio-gpu-gl-pci`. Windows does not offer VirGL. Multi-head is a virtio-gpu / QXL head count. `virtio-ramfb` is offered only when the selected QEMU lists it.

USB redirect rules skip HID class `0x03`. Smartcard appears only when `usb-ccid` is in that QEMU’s device list. Chardev lines, SPICE guest-agent notes, and tablet or keyboard toggles are on the Lab page.

CPU pin and optional `mem-prealloc` are per VM. A guest-agent serial port can show connected or down. `guest-exec` stays off unless you turn it on.

## Acceleration

Per-VM policy is Auto, Prefer hardware, Force TCG, or Prefer WSL KVM. The effective choice is applied at start. Auto clears a TCG or WSL launch that Lab itself turned on. It does not clear Force TCG that a Win9x profile set on its own.

**Help → WHPX health** runs on Windows. A failed probe can offer Force TCG. Other operating systems do not set that failure flag, so a Linux host does not prompt for TCG just because `whpx` is missing from `-accel help`.

virtiofsd and vsock helpers start before QEMU. If a later start check fails, those companions are stopped.

## Wizard profiles and bring-your-own wraps

First-run cards open an existing wizard path: Windows 11 ARM, TrueNAS, Inferno, a retro guest, and Import OVA. Import OVA selects appliance import.

New wizard profiles cover extra architectures, including PowerNV and s390x, plus Windows on ARM and confidential-guest entries. Nitro, SEV-SNP, and TDX are hidden on Windows Store builds. SEV still needs `/dev/sev` on Linux.

| Wrap | What AQEMU does |
|--|--|
| Quickemu | Imports a `.conf` onto the bundled QEMU. No automatic download of proprietary ISOs |
| xemu | Picks a binary you installed and writes a toml. See [`extras/xemu/README.txt`](../extras/xemu/README.txt). No MCPX, BIOS, HDD, DVD, or EEPROM is shipped |
| Android | Detects an installed SDK and AVD. Does not vendor Google’s emulator |
| OpenCore | Sits beside the existing macOS profiles. You supply OVMF, OpenCore, and the OSK |
| Nitro / SEV / TDX | Linux when the host can support them. Hidden on Windows Store builds |
| Darwin research shell | Imports a firmware directory prepared on a Mac. It is not Inferno and it does not boot SpringBoard |

## Left out of 1.5.1

These stay skipped: Unicorn, Qiling, WinAFL, Firecracker, crosvm, Xenia, Dolphin, RPCS3, Anbox, Waydroid, Limbo, Xen, dead XNU trees, and Corellium. `vmapple` is a Mac-host note, not a Windows feature. Inferno and Reims are the existing paths, not a second copy.

The in-app Reims status panel stays blocked on [issue #42](https://github.com/chronic8000/aqemu/issues/42). Store text still does not say AQEMU includes Xbox, Android, or macOS.
