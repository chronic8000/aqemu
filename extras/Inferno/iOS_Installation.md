# Installing iOS in AQEMU (ChefKiss Inferno)

This is the starter guide for **iOS (ARM64)** on Windows using AQEMU + Inferno `qemu-system-applesoc`. Follow it in order. When it is proven, this file is meant to be linked from the app.

AQEMU does **not** ship IPSWs, kernels, DeviceTrees, tickets, or Apple firmware. You supply those. AQEMU drives QEMU, the companion USB restore, filesystem patches, and device tools.

This is **research / TCG** (slow). It is not a consumer “iPhone simulator.” App Store and Safari need **companion reverse-tether**, not the AQEMU Network NIC tab.

Official Inferno docs (keep these open):

- [File setup](https://chefkiss.dev/guides/inferno/file-setup/)
- [Filesystem patches](https://chefkiss.dev/guides/inferno/fs-patches/)
- [Device buttons](https://chefkiss.dev/guides/inferno/device-buttons/)
- [Companion setup](https://chefkiss.dev/guides/inferno/companion-setup/)
- [Discussion #192 (guest internet)](https://github.com/ChefKissInc/Inferno/discussions/192)

---

## What you need

### Host

- Windows 10/11 with **WSL** (Ubuntu) already configured in AQEMU’s first-run / WSL settings.
- AQEMU built with Inferno support (`qemu-system-applesoc` available to WSL).
- Enough free disk for a **32 GiB** sparse `root` image (ChefKiss default). AQEMU currently creates an **8 GiB** `root` if you do nothing — grow it **before** restore if you want App Store / large IPAs (see [Storage](#storage-8-gib-vs-32-gib)).

### Companion (already available)

Do **not** build a companion VM from this guide. The companion will be an in-app Debian download later. For now you already have a Linux companion disk that:

- Boots under **File → Apple SoC Restore → Start companion in WSL**
- Listens for SSH on **`127.0.0.1:32222`**
- Has patched **`idevicerestore`** (Inferno reports `N104DEV`; the patch rewrites DEV→AP)
- Can run `usbmuxd`, `idevice_id`, and later `nmcli` / `ideviceinstaller`

You need the companion **SSH user and password** (same ones AQEMU Restore and Device Tools use).

### Firmware (iPhone 11 / t8030)

The working AQEMU preset is **`t8030`** (A13, **iPhone12,1**, hardware model **`n104ap`**).

Use a **supported IPSW**. The ChefKiss example is **iOS 14.0 beta 5** `18A5351d` for iPhone 11:

`iPhone11,8,iPhone12,1_14.0_18A5351d_Restore.ipsw`

From that IPSW (and ChefKiss extras) you need:

| Role | Typical file | Where it goes in AQEMU |
| --- | --- | --- |
| Kernel | extracted kernelcache (`.research` / decrypted) | MACHINE → **Kernel** |
| DeviceTree | extracted **`.dtb` / `.dec`**, not raw `.im4p` | MACHINE → **DeviceTree** |
| Trustcache | from IPSW / Inferno recipe | MACHINE → **Trustcache** |
| Restore ticket | forged **`root_ticket.der`** (not a 2-byte placeholder) | MACHINE → **Restore ticket** |
| Restore ramdisk | restore **`.dmg`** from the IPSW | MACHINE → **Restore ramdisk (-initrd)** |
| SEP firmware | **repackaged** `sep-firmware.n104.RELEASE.new.img4` | MACHINE → **SEP firmware** |
| SEP ROM | **Cebu B1** dump for t8030 | MACHINE → **SEP ROM** |
| IPSW | the `.ipsw` (or `.zip`) | MACHINE → **IPSW (restore)** and Restore dialog |
| USB remote | IPv4 **`127.0.0.1` port `8030`** | MACHINE → **USB remote** (must match companion) |

Optional: SecureROM. Leave KASLR off unless you know you want it.

Ticket / SEP packaging is documented by ChefKiss ([file setup](https://chefkiss.dev/guides/inferno/file-setup/)). Short version:

```text
python3 create_apticket.py n104ap BuildManifest.plist ticket.shsh2 root_ticket.der
python3 create_septicket.py n104ap BuildManifest.plist ticket.shsh2 sep_root_ticket.der
```

Then `img4` with the Apple Wiki IV+key to produce `sep-firmware.n104.RELEASE.new.img4`.  
Unsigned builds **must** use a real forged ticket for `idevicerestore -T`. Rebuild the ticket when you change IPSW build.

`extras/Inferno/` in this tree has helper scripts (`create_apticket.py`, `create_septicket.py`) and notes in `README.txt`.

---

## 1. Create the iOS VM

1. **New VM Wizard → Apple → iOS (ARM64)**.
2. Finish and open the VM.
3. Confirm architecture / emulator is Inferno **`qemu-system-applesoc`**, machine **`t8030`**, RAM **≥ 4 GiB**, **4 vCPUs** is a reasonable default.

AQEMU creates NVMe images under:

```text
<VM folder>\<VM_name>_inferno\
  root, firmware, syscfg, ctrl_bits, nvram, effaceable, panic_log, sep_nvram, sep_ssc
```

Example: `C:\Users\<you>\AQEMU_VM\iOS_ARM64__inferno\`

`root` may be a **symlink** to a backup (for example `D:\aqemu-backups\iOS_ARM64_root.fs-patched`). Always patch and resize the **real file**.

---

## 2. Fill MACHINE (firmware + USB)

On the iOS VM page, **MACHINE** (and DeviceTree / Kernel on the main boot area):

1. Set **Kernel** and **DeviceTree** (DeviceTree must be extracted `.dtb`/`.dec`).
2. **File → iOS Firmware Tool…** unpacks the IPSW; IM4P decrypt needs **`pyimg4`** on PATH (or next to `aqemu.exe`). AQEMU does not vendor `pyimg4`.
3. Set Trustcache, Restore ticket, SEP firmware, SEP ROM, IPSW path, Restore ramdisk.
4. **USB remote:** type **IPv4**, address **`127.0.0.1`**, port **`8030`**.  
   Restore dialog → **Apply localhost preset (127.0.0.1:8030)** writes the same values.
5. Save the VM.

Boot validation fails if kernelcache or DeviceTree is missing/wrong type.

---

## 3. Grow `root` before the first restore (recommended)

ChefKiss uses **`root 32G`**. AQEMU’s auto-created `root` is **8 GiB** (~9 GB in Settings). That is enough to restore and reach SpringBoard, but **not** enough for large IPAs (UTM SE extract fails around 800 MB free).

**Power Off iOS.** Then in PowerShell (adjust the path if `root` is a symlink — resize the target):

```powershell
$path = 'D:\aqemu-backups\iOS_ARM64_root.fs-patched'   # or ...\iOS_ARM64__inferno\root
fsutil sparse setflag $path
$fs = [System.IO.File]::Open($path, 'Open', 'ReadWrite')
$fs.SetLength(32GB)
$fs.Close()
(Get-Item $path).Length / 1GB
```

If the file does not exist yet: **Power On once** (creates empty images), **Power Off**, then resize.

Growing the file **after** iOS is already installed does **not** change Settings storage until APFS is grown or you **re-restore** onto the larger disk (see [Re-install iOS](#re-install-ios)).

Nothing else should have the image open. If Power On fails with `Failed to get "write" lock` on `root`, leftover `qemu-system-applesoc` is still running — Power Off in AQEMU, or stop only those Inferno QEMU processes (do not confuse that with closing the AQEMU window).

---

## 4. Start companion, then Power On iOS (restore mode)

Order matters:

1. **File → Apple SoC Restore**
2. USB remote **127.0.0.1:8030**, companion SSH user/password, companion disk selected.
3. **Start companion in WSL** — wait until SSH on **32222** is up (first TCG boot can take **1–3 minutes**).
4. **Power On** the **iOS** VM (not the companion as the “phone”).

You should see an Apple logo / restore-style screen, **not** SpringBoard yet.

Check:

```text
<VM folder>\<VM_name>_inferno\qemu-boot.log
```

For restore you want something like:

- `Auto Boot: false`
- Boot args containing `-restore rd=md0` and `nand-enable-reformat=1`

If Auto Boot stays **true**, NVRAM from an old restore is skipping recovery. Wipe `nvram` (or full NVMe wipe — [Re-install](#re-install-ios)) and keep **Restore ramdisk** filled so AQEMU passes `boot-mode=enter_recovery`.

**Diagnose** on the Restore dialog is useful if USB does not appear.

---

## 5. Restore the IPSW

Still in **Apple SoC Restore**:

1. Confirm the IPSW path.
2. **Restore IPSW via SSH…**

AQEMU uploads the IPSW into the companion and runs **`idevicerestore`** there (USB is inside the companion, not on Windows).

Typical failures:

| Symptom | What to do |
| --- | --- |
| `Unable to discover device type` | Companion `idevicerestore` missing the DEV→AP patch |
| `Could not read data (-256)` right after NORData | Wrong/raw SEP firmware; use **repackaged** `*.new.img4`. Partial flash: wipe NVMe and restore again |
| Tiny / placeholder ticket | Forge a real `root_ticket.der` for this IPSW |
| Device never appears | Companion + iOS both up, **same** `127.0.0.1:8030`, wait, Diagnose |

When restore **finishes**, Inferno/QEMU often **exits**. That is expected. Do **not** chase SpringBoard yet.

---

## 6. Filesystem patches (required)

Without this step the display stays **black forever**. Patches apply to the **iOS guest `root`**, **never** the companion disk.

1. **Power Off** iOS (already exited is fine).
2. **File → Apply iOS filesystem patches…**  
   (also MACHINE tab, Restore dialog, and the post-restore prompt)
3. **Use current VM root** (or browse; follow symlinks to the real file).
4. **Apply filesystem patches…**

Windows runs `extras/Inferno/apply-fs-patches-wsl.sh` (temporary Ubuntu KVM + linux-apfs): InfernoFSPatcher on `dyld_shared_cache_arm64e`, then disables launchd keys such as CommCenter / locationd / voicemail.

On success AQEMU **clears Restore ramdisk (-initrd)** so the next boot is normal OS, not recovery.

Manual ChefKiss steps (macOS `hdiutil`): [fs-patches](https://chefkiss.dev/guides/inferno/fs-patches/).

---

## 7. First SpringBoard boot

1. Confirm MACHINE **Restore ramdisk** is **empty**.
2. Companion may stay running (needed later for USB/internet/IPA).
3. **Power On** iOS.
4. Wait — TCG is slow. Setup / SpringBoard should appear.
5. Use the **session toolbar** (not a fake iPhone bezel):

| Control | Inferno key |
| --- | --- |
| Vol− | F3 |
| Vol+ | F4 |
| Side (power) | F5 |
| Home | F6 (double Home = App Switcher) |
| SOS | Vol+ then Side |
| Pad | floating button pad |
| Net | Guest Internet / Device Tools |

Swipe-home is unreliable on t8030 — use **Home**.

Kernel-patch `info:` lines in the log are **normal**. Exit **139** after a short boot is often a QEMU crash, not “missing patches.” Exit **1** with **write lock** means a second Inferno QEMU still has `root` open.

---

## 8. Guest internet (Safari / App Store)

Guest Wi‑Fi/cellular is **not** emulated. Do **not** use the AQEMU **Network** card tab for this.

1. Companion running, iOS **Powered On**, USB **127.0.0.1:8030**.
2. **File / VM → Guest Internet / iOS Device Tools…** or session toolbar **Net**.
3. Same SSH user/password as Restore.
4. **Enable guest internet (reverse-tether)**.

That sets `USBMUXD_DEFAULT_DEVICE_MODE=3` and NetworkManager **Shared** on the USB/NCM iface (often `enxdeadbeef2212`), **not** the companion’s main NIC (`enp0s2`).

If **`nmcli missing`**: on the companion

```bash
sudo apt-get update
sudo apt-get install -y network-manager
sudo systemctl enable --now NetworkManager
```

Then click **Enable** again.

If the connection is added but activation fails with **strictly unmanaged** / a nonsense `lo` error (Ubuntu Server default), on the companion:

```bash
sudo tee /etc/NetworkManager/conf.d/10-globally-managed-devices.conf >/dev/null <<'EOF'
[keyfile]
unmanaged-devices=*,except:type:wifi,except:type:wwan,except:type:ethernet
EOF

sudo tee /etc/NetworkManager/conf.d/99-aqemu-usb-ethernet.conf >/dev/null <<'EOF'
[device]
match-device=interface-name:enx*
managed=true
EOF

sudo systemctl restart NetworkManager
IFACE=$(ip -o link | awk -F': ' '{print $2}' | grep '^enx' | head -1)
echo USB_IFACE=$IFACE
sudo nmcli device set "$IFACE" managed yes
sudo nmcli connection delete aqemu-share-$IFACE 2>/dev/null || true
sudo nmcli connection add type ethernet ifname "$IFACE" con-name "aqemu-share-$IFACE" \
  connection.interface-name "$IFACE" ipv4.method shared ipv6.method ignore autoconnect yes
sudo nmcli connection up "aqemu-share-$IFACE" ifname "$IFACE"
nmcli device
idevice_id -l
```

You want `idevice_id -l` to print a UDID (example `00008030-1122334455667788`) and the `enx*` iface **connected** with the `aqemu-share-…` profile. Then try Safari / App Store on the guest.

Device Tools → **Diagnose USB / idevice** and **Copy manual commands** help if it fails.

---

## 9. Install an IPA

**Device** tab in Guest Internet / iOS Device Tools → **Install IPA…**

Needs `ideviceinstaller` on the companion:

```bash
sudo apt-get install -y ideviceinstaller
```

Notes:

- File must be a real **`.ipa`** (zip with `Payload/*.app`). Jailbreak **`.deb`** packages (NewTerm 2/3 from Chariz) are **not** IPAs and will not install this way.
- Warnings about missing `iTunesMetadata.plist` / `.sinf` are common for sideloaded IPAs.
- **`PackageExtractionFailed` at ~15%** is usually a **corrupt IPA**, a **zip-in-zip**, or **not enough free space on the guest** (staging + extract). UTM SE needs well over ~800 MB free — grow APFS / re-restore onto 32 GiB `root` first.
- Unsigned apps may still need whatever Inferno/jailbreak path you use; AQEMU only runs `ideviceinstaller` over USB.

---

## Storage (8 GiB vs 32 GiB)

| What | Effect |
| --- | --- |
| Resize the `root` **file** on Windows | NVMe is bigger; **iOS Settings still shows ~9 GB** until APFS is recreated or resized |
| **Re-restore** IPSW onto the larger empty/wiped `root` | Restore reformats NAND; Settings should show ~32 GB |
| `diskutil apfs resizeContainer … 0` **inside** iOS | Grows APFS without wipe — needs a **guest shell**. Official NewTerm is a **.deb**, not an IPA, so this is usually **not** available on a stock Inferno guest |

There is no Settings switch for “make disk bigger.”

---

## Re-install iOS

Use this when storage is stuck at ~9 GB after a host resize, restore is half-flashed, SpringBoard never appears after patches, or you want a clean guest.

### A. Power Off

Power Off **iOS**. Leave companion stopped or stop it from Restore → stop companion. Do not leave a second Inferno QEMU holding `root`.

### B. Wipe guest NVMe (clean NAND)

From the AQEMU source/extras tree (iOS **must** be off):

```powershell
powershell -File extras\Inferno\wipe-ios-nvme.ps1 -VmXml "C:\Users\<you>\AQEMU_VM\iOS_ARM64_.aqemu"
```

Type `YES`. This deletes `root`, `nvram`, `firmware`, etc. under `*_inferno\`.

If `root` is a **symlink** to another drive, delete or replace **that target file** too, or the wipe only removes the link.

### C. Recreate images and size `root` to 32 GiB

1. **Power On** iOS once so AQEMU recreates empty images (still 8 GiB `root`).
2. **Power Off** immediately (no restore yet).
3. Sparse-resize `root` (or the symlink target) to **32 GiB** as in [section 3](#3-grow-root-before-the-first-restore-recommended).

### D. Restore mode again

1. Put **Restore ramdisk (-initrd)** back on MACHINE (the `.dmg` used before). Save.
2. **Start companion**.
3. **Power On** iOS — confirm `qemu-boot.log` has **Auto Boot: false** and restore boot args.
4. **Restore IPSW via SSH…** and wait until it completes (QEMU will likely exit).

### E. Patch and boot

1. **Apply iOS filesystem patches…** on the **guest** `root` (clears `-initrd`).
2. **Power On** → SpringBoard / Setup.
3. Re-do **Enable guest internet** if you need Safari / App Store.
4. Reinstall IPAs.

You do **not** need to recreate the iOS VM or re-unpack firmware if MACHINE paths are still valid. You **do** need a ticket that matches this IPSW build.

### Re-restore without wiping (same size disk)

If NAND is healthy and you only need recovery again:

1. Set Restore ramdisk; Power Off; wipe **`nvram`** only if Auto Boot stays true.
2. Start companion → Power On iOS → Restore IPSW → FS patches → clear initrd → Power On.

---

## Checklist (happy path)

1. iOS (ARM64) VM, `t8030`, firmware paths + USB `127.0.0.1:8030`
2. `root` grown to 32 GiB **before** restore
3. Start companion → SSH `:32222` up
4. Power On iOS in restore (ramdisk set)
5. Restore IPSW via SSH
6. Power Off → filesystem patches on guest `root` → initrd cleared
7. Power On → SpringBoard
8. Net → Enable guest internet (install NetworkManager on companion if needed)
9. Install **.ipa** files only; watch guest free space

---

## Menu map

| Task | Where |
| --- | --- |
| Unpack IPSW / IM4P | File → **iOS Firmware Tool…** |
| Companion + idevicerestore | File → **Apple SoC Restore** |
| SpringBoard patches | File / MACHINE / Restore → **Apply iOS filesystem patches…** |
| Internet + IPA + screenshot | File / VM → **Guest Internet / iOS Device Tools…** or toolbar **Net** |
| Hardware buttons | Session toolbar Vol / Home / Side / SOS / Pad |

---

## What this guide does not cover

- Creating or installing the companion Linux disk (in-app Debian download later).
- Obtaining IPSWs, SHSH, SEP keys, or Apple firmware (you bring those; ChefKiss + Apple Wiki).
- Building Inferno QEMU from source.
- NewTerm / Cydia `.deb` workflows.
