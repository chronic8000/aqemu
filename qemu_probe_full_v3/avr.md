# QEMU Complete Architecture capability map: `avr`

- **Architecture Target:** `avr`
- **Binary Executable:** `C:\Users\chron\CURSOR-PROJECTS\aqemu\build_win\qemu-system-avr.exe`
- **Probed At:** 2026-07-31T08:32:58.270269
- **Fallback Machine Used for Context:** `none`

> **INSTRUCTIONS FOR CURSOR AI:** This document contains the verified whitelist of supported flags, boards, CPUs, devices, storage drivers, audio backends, and display renderers for `qemu-system-{arch}`. Use this data as the absolute ground truth to construct and validate VM configuration parameters.

## 1. Supported Machines (`-machine help`)

```text
Supported machines are:
2009                 Arduino Duemilanove (ATmega168) (alias of arduino-duemilanove)
arduino-duemilanove  Arduino Duemilanove (ATmega168)
mega2560             Arduino Mega 2560 (ATmega2560) (alias of arduino-mega-2560-v3)
arduino-mega-2560-v3 Arduino Mega 2560 (ATmega2560)
mega                 Arduino Mega (ATmega1280) (alias of arduino-mega)
arduino-mega         Arduino Mega (ATmega1280)
uno                  Arduino UNO (ATmega328P) (alias of arduino-uno)
arduino-uno          Arduino UNO (ATmega328P)
none                 empty machine
```
Total items listed: 10

## 2. Supported CPUs (`-cpu help`)

```text
Available CPUs:
  avr5
  avr51
  avr6
```
Total items listed: 4

## 3. Supported Devices & Busses (`-device help`)

```text
Misc devices:
name "loader", desc "Generic Loader"
name "uefi-vars-sysbus", bus System
name "uefi-vars-x64", bus System
```
Total items listed: 4

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
  qemu-vdagent
  null
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
