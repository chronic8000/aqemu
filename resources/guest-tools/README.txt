AQEMU guest tools

Linux
  1. Open a terminal in this folder.
  2. Run: sh install-linux.sh
  That installs qemu-guest-agent and starts the clipboard helper.

Windows
  1. Install qemu-guest-agent from the QEMU project (it includes the virtio serial driver).
  2. In this folder run:
     powershell -STA -ExecutionPolicy Bypass -File aqemu-guest.ps1

After that, AQEMU can copy a file to a guest path and copy a file from a guest path.
Copying files in the guest file manager sends them to AQEMU.
DOS and Windows 95/98 keep using Paste files (floppy or USB). They cannot run these tools.
