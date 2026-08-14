AQEMU / ChefKiss Inferno extras
================================

iOS on Windows (install, extract firmware, MACHINE fields, restore,
FS patches, internet, IPA, re-install):

  extras/Inferno/iOS_Installation.md

That is the iOS README. Linked from the app tree and the project README.

This folder also contains helper files used by that guide:

  create_apticket.py / create_septicket.py
                            (run from File → iOS Firmware Tool, Step 2)
  idevicerestore.patch          (N104DEV → AP)
  companion-bin/                (Linux idevice* for the companion image)
  apply-fs-patches-wsl.sh
  apply-launchd-only-wsl.sh     (set ROOT_IMG; no personal paths)
  wipe-ios-nvme.ps1

ChefKiss:
  https://chefkiss.dev/guides/inferno/file-setup/
  https://chefkiss.dev/guides/inferno/fs-patches/
