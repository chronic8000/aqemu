# Copy and Paste

AQEMU copies text and files between the host and a running guest on Linux and Windows, on x86_64 and ARM. Start the virtual machine and use the session toolbar. Quit AQEMU and start it again after an update so the menu launches the new build.

Text and files are different paths.

| What you copy | What AQEMU does | Guest software |
|--|--|--|
| Text | The embedded VNC display already shares the clipboard | None |
| Files, no installer | **Paste files**, or drop them on the guest window | None. The guest opens floppy A or an AQEMU USB drive |
| One file to a guest path | **To guest** | `qemu-guest-agent` |
| One file from a guest path | **From guest** | `qemu-guest-agent` |
| Files copied inside the guest file manager | The guest helper sends them. AQEMU asks where to save them | `qemu-guest-agent` plus the AQEMU helper |

## Paste files into the guest

Copy the files in the host file manager, then click **Paste files**. AQEMU lists the paths and copies them only after you confirm. Text on the clipboard is ignored, including a path sent by the guest. You can also drop files on the guest window.

AQEMU writes a FAT disk and gives it to the running guest:

- Small copies, up to about 1.4 MB, become a 1.44 MB floppy when the virtual machine has floppy drive A. DOS and Windows 9x can open that drive.
- Every other copy, up to 512 MB, becomes a partitioned FAT16 disk and is hot-plugged as a USB drive. In the guest, open the drive named **AQEMU**. Long file names are kept. Windows needs the partition, so a machine with no floppy still receives a USB disk.

This does not need Samba, 9p, virtiofs, or a program inside the guest. Paste again to send another batch. Each batch is a new floppy or USB disk.

On a Windows host, shared folders are not offered as 9p. The Samba helper on the shared-folder page is hidden there because a Windows host has no `smbd`. Use **Paste files** instead.

## Copy one file by path

**To guest** asks for a file on the host, then a full path inside the guest, including the file name.

- Linux guest: `/home/user/notes.txt`
- Windows guest: `C:\Users\Public\notes.txt`

**From guest** asks for the path inside the guest, then where to save it on the host.

Both buttons talk to `qemu-guest-agent` over a virtio serial port named `org.qemu.guest_agent.0`. AQEMU opens that port on PC, `q35`, and `virt` machines. The host side is a socket that only the AQEMU user can open, or a named pipe on Windows. It is not a TCP port. Raspberry Pi, s390x, NeXT, and Apple SoC machines do not get the port, because those boards have no PCI bus for it. If the agent is not running, AQEMU says so instead of pretending the file arrived.

The agent can read and write files that its user can read and write. On many Linux guests that user is root. Type the path you mean.

## Install the guest tools once

Click **Guest tools**. AQEMU inserts a floppy with four files:

| File | Role |
|--|--|
| `README.txt` | Short reminder inside the guest |
| `install-linux.sh` | Installs `qemu-guest-agent` and starts the clipboard helper |
| `aqemu-guest.py` | Linux helper. Watches the clipboard for copied files |
| `aqemu-guest.ps1` | Windows helper. Watches Explorer for copied files |

### Linux guest

Open the floppy and run:

```sh
sh install-linux.sh
```

The script uses `apt-get`, `dnf`, or `pacman` when one of those is present. It enables `qemu-guest-agent` and starts `aqemu-guest.py` when `python3` is installed. The helper also needs `wl-paste` or `xclip` to see files copied on the Linux clipboard.

### Windows guest

Install `qemu-guest-agent` from the QEMU project. That package includes the virtio serial driver the helper needs. Then, in the guest tools folder:

```powershell
powershell -STA -ExecutionPolicy Bypass -File aqemu-guest.ps1
```

Leave that window running. It watches files copied in Explorer.

### After the helper is running

Copy files in the guest file manager. AQEMU asks for a folder on the host and saves them there. The helper sends at most 64 files, and 64 MB each.

DOS and Windows 95/98 cannot run these tools. Keep using **Paste files** for those guests.

## What is not copied

- A file copied in the guest does not appear on the host until the helper is running, or until you use **From guest** with a path.
- **Paste files** does not drop files into an arbitrary guest folder. It adds a floppy or USB drive.
- NeXT and Apple SoC virtual machines do not get the guest-agent ports.
- Copying a folder tree through **To guest** is one file at a time. Dropping a folder with **Paste files** copies the files inside it onto the FAT disk.
