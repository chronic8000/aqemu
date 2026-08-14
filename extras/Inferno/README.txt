AQEMU / ChefKiss Inferno companion extras
========================================

Starter install + re-install walkthrough (for in-app linking later):
  extras/Inferno/iOS_Installation.md


root_ticket.der
---------------
Not inside the IPSW as a ready file. Unsigned iOS builds need a *forged* AP ticket:

  1. Extract BuildManifest.plist from the IPSW (zip).
  2. Download ChefKiss ticket.shsh2 + create_apticket.py
     https://chefkiss.dev/Extras/Inferno/
  3. python3 create_apticket.py n104ap BuildManifest.plist ticket.shsh2 root_ticket.der

  Model n104ap = iPhone 11 / iPhone12,1 (Inferno t8030).
  Rebuild the ticket whenever you change IPSW build.

  Point the iOS VM "ticket" path at this file. AQEMU Start companion stages it as
  aqemu-restore-ticket.der on the IPSW 9p share for idevicerestore -T.

idevicerestore.patch
--------------------
Required. Inferno reports HardwareModel N104DEV; patch rewrites DEV→AP.
Without it: "Unable to discover device type" after serial INFERNO_*.

companion-bin/
-------------
Linux x86_64 binaries pulled/built for the Ubuntu companion (same ABI works on
Debian for the eventual downloadable companion image). Install on guest:

  sudo install -m 755 companion-bin/idevicerestore /usr/local/bin/
  # other idevice* tools as needed; keep matching /usr/local/lib from source build

Guide: https://chefkiss.dev/guides/inferno/file-setup/

sep-firmware.n104.RELEASE.new.img4
----------------------------------
ChefKiss requires a *repackaged* SEP firmware for Inferno sep-fw=, not the raw
.im4p from the IPSW. Without it, restore often dies right after NORData with
"Could not read data (-256)".

Built into build_win/ with:
  create_septicket.py + img4 -k IVKEY (Apple Wiki keys for 18A5351d) -V none
Point the iOS VM "SEP firmware" field at:
  build_win/sep-firmware.n104.RELEASE.new.img4

wipe-ios-nvme.ps1
-----------------
After a failed restore (especially "Could not read data (-256)" right after
"Done sending NORData"), partial flash leaves the iOS guest NVMe in a bad state.
Power Off the iOS VM, then:

  powershell -File extras/Inferno/wipe-ios-nvme.ps1 -VmXml "C:\path\iOS_ARM64_.aqemu"

AQEMU recreates empty images on next boot. Then: Stop companion -> Start companion
-> Power On iOS -> Restore again.

Re-restore after a successful restore
-------------------------------------
Guest NVRAM keeps auto-boot=true. Inferno only adds "-restore rd=md0 …" when
auto-boot is false. Passing -initrd alone is not enough.

AQEMU sets boot-mode=enter_recovery on -machine whenever "Restore ramdisk
(-initrd)" is filled in. You should see in the Error Log:
  Auto Boot: false
  Boot Args: [-restore rd=md0 nand-enable-reformat=1 …]

If Auto Boot stays true, wipe nvram (or full wipe-ios-nvme.ps1) and rebuild
AQEMU so boot-mode=enter_recovery is applied.

Filesystem patches (REQUIRED for SpringBoard)
---------------------------------------------
After idevicerestore finishes, Inferno exits so you can patch the guest
root disk. Without this step the phone VNC stays black forever.

This is the iOS guest *_inferno/root image — NOT the Ubuntu companion disk.

In AQEMU (Windows + WSL):
  File → Apply iOS filesystem patches…
  MACHINE tab → Apply filesystem patches…
  Apple SoC Restore → Apply filesystem patches…
  Or click that button on the post-restore dialog.

The GUI runs extras/Inferno/apply-fs-patches-wsl.sh (temporary Ubuntu KVM +
linux-apfs). Power Off the iOS guest first. On success, Restore ramdisk
(-initrd) is cleared automatically; then Power On for setup.

Official guide (macOS hdiutil attach of the raw "root" image):
  https://chefkiss.dev/guides/inferno/fs-patches/

Summary:
  1. Attach root (4096-byte blocks) read-write
  2. Run InfernoFSPatcher on dyld_shared_cache_arm64e
  3. Disable in System/Library/xpc/launchd.plist LaunchDaemons
     (keys look like /System/Library/LaunchDaemons/com.apple.CommCenter.plist):
       com.apple.voicemail.vmd
       com.apple.CommCenter
       com.apple.CommCenterMobileHelper
       com.apple.CommCenterRootHelper
       com.apple.locationd
  4. Eject, clear Restore ramdisk in AQEMU, Power On

Manual WSL helpers (same scripts the GUI uses):
  extras/Inferno/apply-fs-patches-wsl.sh   (ROOT_IMG=/path/to/.../root)
  extras/Inferno/apply-launchd-only-wsl.sh

AQEMU root image for this VM is typically:
  <VM_dir>/<VM_name>_inferno/root

QEMU boot log (Apple SoC)
-------------------------
Every iOS/Inferno Power On writes live QEMU stderr/stdout to:

  <VM_dir>/<VM_name>_inferno/qemu-boot.log

(Truncated each start.) Open that file while the guest is still black to see
Auto Boot / Boot Args / SEP Panic without waiting for an exit dialog.

Hardware buttons (Home / Power / Volume)
----------------------------------------
Inferno maps device buttons to function keys:
  https://chefkiss.dev/guides/inferno/device-buttons/

  F3 Vol−   F4 Vol+   F5 Side/Power   F6 Home (double = App Switcher)

In AQEMU session toolbar (Apple SoC VMs): Vol− / Vol+ / Home / Side / SOS / More…
Optional floating "Pad" (neutral controls — not an iPhone bezel).
Swipe-home is unreliable on T8030; use Home.

Guest internet (App Store / Safari) — NOT the AQEMU Network NIC tab
-------------------------------------------------------------------
Guest Wi‑Fi/cellular is not emulated. Internet is reverse-tether through
the Ubuntu companion USB bridge (usbmuxd MODE=3 + NetworkManager Shared).

How to enable in AQEMU:
  1. Companion running; iOS Powered On with USB remote 127.0.0.1:8030
  2. File / VM → Guest Internet / iOS Device Tools…
     OR session toolbar → Net
  3. Enter companion SSH user/password (same as Restore)
  4. Click Enable guest internet (reverse-tether)

Also: Diagnose USB / idevice, Copy manual commands, Device tab for IPA.
See ChefKiss Inferno discussion #192 / companion-setup guide.


