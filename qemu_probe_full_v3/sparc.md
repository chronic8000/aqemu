# QEMU Complete Architecture capability map: `sparc`

- **Architecture Target:** `sparc`
- **Binary Executable:** `C:\Users\chron\CURSOR-PROJECTS\aqemu\build_win\qemu-system-sparc.exe`
- **Probed At:** 2026-07-31T08:33:09.565044
- **Fallback Machine Used for Context:** `SS-5`

> **INSTRUCTIONS FOR CURSOR AI:** This document contains the verified whitelist of supported flags, boards, CPUs, devices, storage drivers, audio backends, and display renderers for `qemu-system-{arch}`. Use this data as the absolute ground truth to construct and validate VM configuration parameters.

## 1. Supported Machines (`-machine help`)

```text
Supported machines are:
LX                   Sun4m platform, SPARCstation LX
SPARCClassic         Sun4m platform, SPARCClassic
SPARCbook            Sun4m platform, SPARCbook
SS-10                Sun4m platform, SPARCstation 10
SS-20                Sun4m platform, SPARCstation 20
SS-4                 Sun4m platform, SPARCstation 4
SS-5                 Sun4m platform, SPARCstation 5 (default)
SS-600MP             Sun4m platform, SPARCserver 600MP
Voyager              Sun4m platform, SPARCstation Voyager
leon3_generic        Leon-3 generic
none                 empty machine
```
Total items listed: 12

## 2. Supported CPUs (`-cpu help`)

```text
Available CPU types:
 Fujitsu-MB86904      (IU 04000000 FPU 00080000 MMU 04000000 NWINS 8)
 Fujitsu-MB86907      (IU 05000000 FPU 00080000 MMU 05000000 NWINS 8)
 TI-MicroSparc-I      (IU 41000000 FPU 00080000 MMU 41000000 NWINS 7) -fsmuld
 TI-MicroSparc-II     (IU 42000000 FPU 00080000 MMU 02000000 NWINS 8)
 TI-MicroSparc-IIep   (IU 42000000 FPU 00080000 MMU 04000000 NWINS 8)
 TI-SuperSparc-40     (IU 41000000 FPU 00000000 MMU 00000800 NWINS 8)
 TI-SuperSparc-50     (IU 40000000 FPU 00000000 MMU 01000800 NWINS 8)
 TI-SuperSparc-51     (IU 40000000 FPU 00000000 MMU 01000000 NWINS 8)
 TI-SuperSparc-60     (IU 40000000 FPU 00000000 MMU 01000800 NWINS 8)
 TI-SuperSparc-61     (IU 44000000 FPU 00000000 MMU 01000000 NWINS 8)
 TI-SuperSparc-II     (IU 40000000 FPU 00000000 MMU 08000000 NWINS 8)
 LEON2                (IU f2000000 FPU 00080000 MMU f2000000 NWINS 8)
 LEON3                (IU f3000000 FPU 00080000 MMU f3000000 NWINS 8)
Default CPU feature flags (use '-' to remove): mul div fsmuld
Available CPU feature flags (use '+' to add): float128
Numerical features (use '=' to set): iu_version fpu_version mmu_version nwindows
```
Total items listed: 17

## 3. Supported Devices & Busses (`-device help`)

```text
Storage devices:
name "floppy", bus floppy-bus, desc "virtual floppy drive"
name "scsi-cd", bus SCSI, desc "virtual SCSI CD-ROM"
name "scsi-hd", bus SCSI, desc "virtual SCSI disk"
Misc devices:
name "loader", desc "Generic Loader"
name "uefi-vars-sysbus", bus System
name "uefi-vars-x64", bus System
```
Total items listed: 8

## 4. Storage Image Formats (`-drive format=help`)

```text
Supported formats: blkdebug blklogwrites blkreplay blkverify bochs cloop compress copy-before-write copy-on-read dmg file ftp ftps host_device http https luks nbd null-aio null-co parallels preallocate qcow qcow2 qed quorum raw replication snapshot-access throttle vdi vhdx vmdk vpc vvfat
Supported formats (read-only): blkdebug blklogwrites blkreplay blkverify bochs cloop compress copy-before-write copy-on-read dmg file ftp ftps host_device http https luks nbd null-aio null-co parallels preallocate qcow qcow2 qed quorum raw replication snapshot-access throttle vdi vhdx vmdk vpc vvfat
```
Total items listed: 2

## 5. Network Backends (`-netdev help`)

```text
Available netdev backend types:
socket
stream
dgram
hubport
tap
passt
user
```
Total items listed: 8

## 6. Character Device Backends (`-chardev help`)

```text
Available chardev backend types:
  file
  spicevmc
  pipe
  console
  msmouse
  testdev
  wctablet
  hub
  null
  qemu-vdagent
  stdio
  vc
  memory
  socket
  serial
  dbus
  spiceport
  ringbuf
  udp
  mux
```
Total items listed: 21

## 7. Display Backends (`-display help`)

```text
Available display backend types:
none
curses
spice-app
dbus
Some display backends support suboptions, which can be set with
   -display backend,option=value,option=value...
For a short list of the suboptions for each display, see the top-level -help output; more detail is in the documentation.
```
Total items listed: 8

## 8. Audio Drivers (`-audiodev help`)

```text
Available audio drivers:
none
dbus
dsound
spice
wav
```
Total items listed: 6

## 9. Accelerators (`-accel help`)

```text
Accelerators supported in QEMU binary:
tcg
```
Total items listed: 2

## 10. User Objects (`-object help`)

```text
List of user creatable objects:
  authz-list
  authz-list-file
  authz-simple
  colo-compare
  cryptodev-backend
  cryptodev-backend-builtin
  dbus-display
  dbus-vmstate
  filter-buffer
  filter-dump
  filter-mirror
  filter-redirector
  filter-replay
  filter-rewriter
  input-barrier
  iothread
  main-loop
  memory-backend-ram
  qtest
  rng-builtin
  rng-egd
  secret
  throttle-group
  tls-cipher-suites
  tls-creds-anon
  tls-creds-psk
  tls-creds-x509
```
Total items listed: 28

## AQEMU Wizard Rules for this Architecture

1. **Machine Whitelist:** Only select machines that match strings in Section 1.
2. **Bus Attachment Matching:** Devices in Section 3 specify their required bus (e.g. `bus PCI`, `bus ISA`, `bus virtio-bus`). Do not place PCI devices on ISA-only machines like `isapc`.
3. **Network Backend Validation:** Use Section 5 backends (e.g. `user`, `tap`, `socket`).
4. **Storage Drivers:** Only attach block drivers matching Section 4 formats.
