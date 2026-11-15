# Debugging the Windows kernel with WinDbg

Use **VM → Lab → Debug → Kernel debug (WinDbg)** to attach WinDbg or WinDbgX to a Windows guest.

## Modes

| Mode | Host | QEMU side | WinDbg `-k` |
|--|--|--|--|
| Serial KD (named pipe) | Windows | `-chardev pipe,id=aqkd0,path=…` + `-serial chardev:aqkd0` (or legacy `-serial pipe:…`) | `com:pipe,port=\\.\pipe\<name>,resets=0,reconnect` |
| Serial KD (TCP) | Any | `-chardev socket,id=aqkd0,…,server=on,wait=off` + `-serial chardev:aqkd0` | `tcp:Port=<p>,Server=<host>` |
| KDNET | Any | No extra serial args; guest talks UDP to the host | `net:port=<p>,key=<a.b.c.d>` |
| GDB stub | Any | Existing `-gdb` / `-S` | Not WinDbg; use GDB or an EXDI bridge (experimental) |

On Linux hosts, “named pipe” falls back to a local TCP listen port.

Kernel debug serial is inserted as **COM1**. The AQEMU serial monitor does not take COM1 while pipe/TCP KD is on. Set **Guest COM port** and the copied `bcdedit` lines if you change that.

## Guest setup

Elevated Command Prompt (Vista and later):

```
bcdedit /debug on
bcdedit /dbgsettings serial debugport:1 baudrate:115200
```

For KDNET (Windows 8+):

```
bcdedit /debug on
bcdedit /dbgsettings net hostip:10.0.2.2 port:50000 key:a.b.c.d
```

With user-mode NAT, the host is usually `10.0.2.2`. Bridge/TAP use the real host IP. Reboot the guest after changing settings.

XP / 2000 / NT: add `/debug /debugport=COM1 /baudrate:115200` to the `boot.ini` entry (Lab copies a commented snippet).

## KDNET NIC

Prefer **e1000** or **e1000e**. Lab warns and can switch the first NIC at start. virtio-net needs the virtio-win KDNET module and is not assumed.

The KDNET key is stored in Windows Credential Manager (or a `0600` file on Linux), not in the VM XML.

## Warnings

- **Secure Boot** must be off before `bcdedit` can enable kernel debug.
- **BitLocker**: changing debug settings can force recovery.
- **VBS / HVCI / Hyper-V in the guest**: you may need `bcdedit /set hypervisordebug on`, or disable VBS for the session.
- **ARM64 Windows guests**: prefer KDNET; legacy serial KD is often unreliable.
- **WHPX / KVM**: serial KD and KDNET are guest-side and should work under hardware accel. The GDB stub is most reliable under TCG.
- Listening on **0.0.0.0** for TCP KD exposes the debug port; Lab asks before saving that option.

## Buttons

- **Copy WinDbg connection** — the `-k …` string
- **Copy guest bcdedit commands** — guest setup text
- **Open WinDbg** — launches `windbg` / WinDbgX when Debugging Tools for Windows (or WinDbgX) is on `PATH` or under Windows Kits
