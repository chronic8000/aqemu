# QEMU Complete Architecture capability map: `microblaze`

- **Architecture Target:** `microblaze`
- **Binary Executable:** `C:\Users\chron\CURSOR-PROJECTS\aqemu\build_win\qemu-system-microblaze.exe`
- **Probed At:** 2026-07-31T08:33:00.077878
- **Fallback Machine Used for Context:** `none`

> **INSTRUCTIONS FOR CURSOR AI:** This document contains the verified whitelist of supported flags, boards, CPUs, devices, storage drivers, audio backends, and display renderers for `qemu-system-{arch}`. Use this data as the absolute ground truth to construct and validate VM configuration parameters.

## 1. Supported Machines (`-machine help`)

```text
Supported machines are:
none                 empty machine
petalogix-ml605      PetaLogix linux refdesign for xilinx ml605 (little endian)
petalogix-s3adsp1800 PetaLogix linux refdesign for xilinx Spartan 3ADSP1800 (default)
xlnx-zynqmp-pmu      Xilinx ZynqMP PMU machine (little endian)
```
Total items listed: 5

## 2. Supported CPUs (`-cpu help`)

```text
Available CPUs:
  microblaze-cpu
```
Total items listed: 2

## 3. Supported Devices & Busses (`-device help`)

```text
Storage devices:
name "160s33b", bus SSI, desc "Serial Flash"
name "25csm04", bus SSI, desc "Serial Flash"
name "320s33b", bus SSI, desc "Serial Flash"
name "640s33b", bus SSI, desc "Serial Flash"
name "at25128a-nonjedec", bus SSI, desc "Serial Flash"
name "at25256a-nonjedec", bus SSI, desc "Serial Flash"
name "at25df041a", bus SSI, desc "Serial Flash"
name "at25df321a", bus SSI, desc "Serial Flash"
name "at25df641", bus SSI, desc "Serial Flash"
name "at25fs010", bus SSI, desc "Serial Flash"
name "at25fs040", bus SSI, desc "Serial Flash"
name "at26df081a", bus SSI, desc "Serial Flash"
name "at26df161a", bus SSI, desc "Serial Flash"
name "at26df321", bus SSI, desc "Serial Flash"
name "at26f004", bus SSI, desc "Serial Flash"
name "at45db081d", bus SSI, desc "Serial Flash"
name "en25f32", bus SSI, desc "Serial Flash"
name "en25p32", bus SSI, desc "Serial Flash"
name "en25p64", bus SSI, desc "Serial Flash"
name "en25q32b", bus SSI, desc "Serial Flash"
name "en25q64", bus SSI, desc "Serial Flash"
name "gd25q32", bus SSI, desc "Serial Flash"
name "gd25q64", bus SSI, desc "Serial Flash"
name "is25lp016d", bus SSI, desc "Serial Flash"
name "is25lp032", bus SSI, desc "Serial Flash"
name "is25lp064", bus SSI, desc "Serial Flash"
name "is25lp080d", bus SSI, desc "Serial Flash"
name "is25lp128", bus SSI, desc "Serial Flash"
name "is25lp256", bus SSI, desc "Serial Flash"
name "is25lq040b", bus SSI, desc "Serial Flash"
name "is25wp032", bus SSI, desc "Serial Flash"
name "is25wp064", bus SSI, desc "Serial Flash"
name "is25wp128", bus SSI, desc "Serial Flash"
name "is25wp256", bus SSI, desc "Serial Flash"
name "m25p05", bus SSI, desc "Serial Flash"
name "m25p10", bus SSI, desc "Serial Flash"
name "m25p128", bus SSI, desc "Serial Flash"
name "m25p16", bus SSI, desc "Serial Flash"
name "m25p20", bus SSI, desc "Serial Flash"
name "m25p32", bus SSI, desc "Serial Flash"
name "m25p40", bus SSI, desc "Serial Flash"
name "m25p64", bus SSI, desc "Serial Flash"
name "m25p80", bus SSI, desc "Serial Flash"
name "m25pe16", bus SSI, desc "Serial Flash"
name "m25pe20", bus SSI, desc "Serial Flash"
name "m25pe80", bus SSI, desc "Serial Flash"
name "m25px32", bus SSI, desc "Serial Flash"
name "m25px32-s0", bus SSI, desc "Serial Flash"
name "m25px32-s1", bus SSI, desc "Serial Flash"
name "m25px64", bus SSI, desc "Serial Flash"
name "m45pe10", bus SSI, desc "Serial Flash"
name "m45pe16", bus SSI, desc "Serial Flash"
name "m45pe80", bus SSI, desc "Serial Flash"
name "mt25ql01g", bus SSI, desc "Serial Flash"
name "mt25ql02g", bus SSI, desc "Serial Flash"
name "mt25ql512ab", bus SSI, desc "Serial Flash"
name "mt25qu01g", bus SSI, desc "Serial Flash"
name "mt25qu02g", bus SSI, desc "Serial Flash"
name "mt35xu01g", bus SSI, desc "Serial Flash"
name "mt35xu02gbba", bus SSI, desc "Serial Flash"
name "mx25l12805d", bus SSI, desc "Serial Flash"
name "mx25l12855e", bus SSI, desc "Serial Flash"
name "mx25l1606e", bus SSI, desc "Serial Flash"
name "mx25l2005a", bus SSI, desc "Serial Flash"
name "mx25l25635e", bus SSI, desc "Serial Flash"
name "mx25l25635f", bus SSI, desc "Serial Flash"
name "mx25l25655e", bus SSI, desc "Serial Flash"
name "mx25l3205d", bus SSI, desc "Serial Flash"
name "mx25l4005a", bus SSI, desc "Serial Flash"
name "mx25l6405d", bus SSI, desc "Serial Flash"
name "mx25l8005", bus SSI, desc "Serial Flash"
name "mx66l1g45g", bus SSI, desc "Serial Flash"
name "mx66l51235f", bus SSI, desc "Serial Flash"
name "mx66u1g45g", bus SSI, desc "Serial Flash"
name "mx66u51235f", bus SSI, desc "Serial Flash"
name "n25q00", bus SSI, desc "Serial Flash"
name "n25q00a", bus SSI, desc "Serial Flash"
name "n25q032", bus SSI, desc "Serial Flash"
name "n25q032a11", bus SSI, desc "Serial Flash"
name "n25q032a13", bus SSI, desc "Serial Flash"
name "n25q064", bus SSI, desc "Serial Flash"
name "n25q064a11", bus SSI, desc "Serial Flash"
name "n25q064a13", bus SSI, desc "Serial Flash"
name "n25q128", bus SSI, desc "Serial Flash"
name "n25q128a11", bus SSI, desc "Serial Flash"
name "n25q128a13", bus SSI, desc "Serial Flash"
name "n25q256a", bus SSI, desc "Serial Flash"
name "n25q256a11", bus SSI, desc "Serial Flash"
name "n25q256a13", bus SSI, desc "Serial Flash"
name "n25q512a", bus SSI, desc "Serial Flash"
name "n25q512a11", bus SSI, desc "Serial Flash"
name "n25q512a13", bus SSI, desc "Serial Flash"
name "n25q512ax3", bus SSI, desc "Serial Flash"
name "s25fl016k", bus SSI, desc "Serial Flash"
name "s25fl064k", bus SSI, desc "Serial Flash"
name "s25fl129p0", bus SSI, desc "Serial Flash"
name "s25fl129p1", bus SSI, desc "Serial Flash"
name "s25fl256s0", bus SSI, desc "Serial Flash"
name "s25fl256s1", bus SSI, desc "Serial Flash"
name "s25fl512s", bus SSI, desc "Serial Flash"
name "s25fs512s", bus SSI, desc "Serial Flash"
name "s25sl004a", bus SSI, desc "Serial Flash"
name "s25sl008a", bus SSI, desc "Serial Flash"
name "s25sl016a", bus SSI, desc "Serial Flash"
name "s25sl032a", bus SSI, desc "Serial Flash"
name "s25sl032p", bus SSI, desc "Serial Flash"
name "s25sl064a", bus SSI, desc "Serial Flash"
name "s25sl064p", bus SSI, desc "Serial Flash"
name "s25sl12800", bus SSI, desc "Serial Flash"
name "s25sl12801", bus SSI, desc "Serial Flash"
name "s70fl01gs", bus SSI, desc "Serial Flash"
name "s70fs01gs", bus SSI, desc "Serial Flash"
name "sst25vf016b", bus SSI, desc "Serial Flash"
name "sst25vf032b", bus SSI, desc "Serial Flash"
name "sst25vf040b", bus SSI, desc "Serial Flash"
name "sst25vf080b", bus SSI, desc "Serial Flash"
name "sst25wf010", bus SSI, desc "Serial Flash"
name "sst25wf020", bus SSI, desc "Serial Flash"
name "sst25wf040", bus SSI, desc "Serial Flash"
name "sst25wf080", bus SSI, desc "Serial Flash"
name "sst25wf512", bus SSI, desc "Serial Flash"
name "w25q01jvq", bus SSI, desc "Serial Flash"
name "w25q02jvm", bus SSI, desc "Serial Flash"
name "w25q256", bus SSI, desc "Serial Flash"
name "w25q32", bus SSI, desc "Serial Flash"
name "w25q32dw", bus SSI, desc "Serial Flash"
name "w25q512jv", bus SSI, desc "Serial Flash"
name "w25q64", bus SSI, desc "Serial Flash"
name "w25q80", bus SSI, desc "Serial Flash"
name "w25q80bl", bus SSI, desc "Serial Flash"
name "w25x10", bus SSI, desc "Serial Flash"
name "w25x16", bus SSI, desc "Serial Flash"
name "w25x20", bus SSI, desc "Serial Flash"
name "w25x32", bus SSI, desc "Serial Flash"
name "w25x40", bus SSI, desc "Serial Flash"
name "w25x64", bus SSI, desc "Serial Flash"
name "w25x80", bus SSI, desc "Serial Flash"
Misc devices:
name "guest-loader", desc "Guest Loader"
name "loader", desc "Generic Loader"
name "uefi-vars-sysbus", bus System
name "uefi-vars-x64", bus System
```
Total items listed: 143

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
  console
  null
  msmouse
  dbus
  memory
  spiceport
  file
  spicevmc
  serial
  socket
  ringbuf
  qemu-vdagent
  pipe
  hub
  vc
  udp
  testdev
  mux
  wctablet
  stdio
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
  can-bus
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
Total items listed: 29

## AQEMU Wizard Rules for this Architecture

1. **Machine Whitelist:** Only select machines that match strings in Section 1.
2. **Bus Attachment Matching:** Devices in Section 3 specify their required bus (e.g. `bus PCI`, `bus ISA`, `bus virtio-bus`). Do not place PCI devices on ISA-only machines like `isapc`.
3. **Network Backend Validation:** Use Section 5 backends (e.g. `user`, `tap`, `socket`).
4. **Storage Drivers:** Only attach block drivers matching Section 4 formats.
