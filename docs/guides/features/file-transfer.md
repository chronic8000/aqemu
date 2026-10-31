# File transfer and shared folders

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

How do I get files into my VM, and back out again? AQEMU has three ways. Pick the first one that fits.

| Way | Works on | Guest software needed | Best for |
| --- | --- | --- | --- |
| **Paste files** (session toolbar) | Windows and Linux hosts | None | Any guest, including DOS and Windows 9x |
| **To guest** / **From guest** (session toolbar) | Windows and Linux hosts | `qemu-guest-agent` | One file to or from an exact path |
| **Shared folders** (Media → Folder Sharing) | **Linux hosts only** | 9p support (Linux/BSD) or Samba client (Windows) | A folder that stays shared |

---

## 1. Paste files (easiest)

1. Start the VM.
2. On your host, copy the files in your file manager (**Ctrl+C**).
3. Click **Paste files** on the session toolbar (or just drag the files onto the guest window).
4. AQEMU lists the files and asks you to confirm.
5. In the guest, open **floppy drive A** (small copies, if the VM has floppy A) or the USB drive named **AQEMU**.

Each paste makes a new floppy or USB disk. Text on the clipboard is not used. Larger copies (up to 512 MB) become a USB drive.

## 2. To guest / From guest (one file by path)

1. Install `qemu-guest-agent` in the guest once (on Ubuntu/Debian: `sudo apt install qemu-guest-agent`). The **Guest tools** button inserts a floppy with an installer for Linux guests and a helper for Windows guests.
2. Click **To guest**, pick the host file, and type the full path inside the guest, including the file name (for example `C:\Users\Public\notes.txt` or `/home/user/notes.txt`).
3. Or click **From guest**, type the guest path, and choose where to save it on the host.

If the agent isn't running, AQEMU tells you instead of pretending the copy worked. Some boards (Raspberry Pi, s390x, NeXT and Apple SoC) don't get the guest-agent channel at all.

## 3. Shared folders (Linux hosts)

On a Linux host, the **Media** tab has a **Folder Sharing** sub-tab:

- **Add Folder** shares a host folder with a Linux or BSD guest over VirtIO 9p. AQEMU shows the `mount -t 9p …` command to run in the guest. The guest needs the 9p module.
- **Add Samba Folder** ("Share folder using samba (for Windows guests)") needs `smbd` installed on the host and **User Mode** networking. In the guest, the folder is at `\\10.0.2.4\qemu`.

The **Network → TFTP/SAMBA** sub-tab also has **Allow SMB access to:** and **TFTP folder:** fields for the same user-mode network.

**On a Windows host**, shared folders aren't offered. The tooltip says: "Shared folders need a Linux host. On Windows, start the virtual machine and use Paste files on the session toolbar."

---

The full step-by-step guide, including the guest helper that sends files copied inside the guest back to the host, is [Copy and paste](../copy-and-paste.md).

[Documentation Hub](../../README.md) · [Session toolbar](session-toolbar.md) · [Networking](networking.md)
