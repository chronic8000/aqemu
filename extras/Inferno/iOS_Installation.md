# Installing iOS in AQEMU (ChefKiss Inferno)

This is **hard**. Follow **Part A** once you already have firmware files. Use **Part B** for every AQEMU field, how to extract the IPSW, tickets, SEP, internet, IPA, storage, and re-install.

AQEMU does **not** ship IPSWs, kernels, DeviceTrees, tickets, or Apple firmware. You bring those. AQEMU drives Inferno QEMU, restore, filesystem patches, buttons, and guest internet.

This is **research / TCG** (slow), not an App Store “iPhone simulator.” Safari and App Store need **companion reverse-tether**, not the AQEMU **Network** NIC tab.

**Companion disk (now and later):** you do **not** build a companion Linux VM from this guide. Today: pick an existing companion `.qcow2` / disk in **File → Apple SoC Restore** (same idea as a HDD image path) and click **Start companion in WSL**. Later: a **Download** button will fetch a Debian companion image; you still only set a **path** to that file, like any other disk. SSH stays **`127.0.0.1:32222`**. USB bridge stays **`127.0.0.1:8030`**.

Official Inferno docs:

- [File setup](https://chefkiss.dev/guides/inferno/file-setup/)
- [Filesystem patches](https://chefkiss.dev/guides/inferno/fs-patches/)
- [Device buttons](https://chefkiss.dev/guides/inferno/device-buttons/)
- [Companion setup](https://chefkiss.dev/guides/inferno/companion-setup/)
- [Discussion #192 (guest internet)](https://github.com/ChefKissInc/Inferno/discussions/192)

---

# Part A — Easy path (once files exist)

Do this **in order**. If a step fails, jump to the matching heading in Part B. Do not skip filesystem patches.

### A0. You need

| Item | Notes |
| --- | --- |
| Windows 10/11 + WSL Ubuntu | Set in AQEMU first-run / WSL settings |
| Inferno `qemu-system-applesoc` | Used via WSL |
| Companion disk path | Restore dialog (future: Download + path box) |
| Companion SSH user + password | Same as Device Tools |
| IPSW + extracted firmware | **File → iOS Firmware Tool…** (unpack + forge tickets + IM4P). Part B §2–4. Reference: iOS **14.0 beta 5** `18A5351d`, iPhone 11 / **t8030** / **n104ap** |
| **32 GiB** sparse `root` | Grow **before** first restore (Part B §6) |

### A1. Create the VM

**New VM Wizard → Apple → iOS (ARM64)** → Finish.  
Confirm: emulator `qemu-system-applesoc`, machine **`t8030`**, RAM **≥ 4 GiB**, **4 vCPUs**.

### A2. Point MACHINE at firmware

**File → iOS Firmware Tool…** first (unpack, **Forge restore + SEP tickets**, DeviceTree IM4P). Then fill remaining rows in [Part B §5](#b5-every-aqemu-field). Minimum: Kernel, DeviceTree (`.dtb`/`.dec` not `.im4p`), Trustcache, Restore ticket, SEP firmware **repackaged** `.img4`, SEP ROM (Cebu B1), IPSW, Restore ramdisk `.dmg`, USB **IPv4 `127.0.0.1` port `8030`**. Save.

### A3. Grow `root` to 32 GiB

Power Off iOS. Resize the real `root` file (follow symlinks). See [§6](#b6-grow-root-32-gib).

### A4. Companion, then iOS restore

1. **File → Apple SoC Restore** — companion **disk path**, SSH user/password, USB `127.0.0.1:8030`.
2. **Start companion in WSL** — wait **1–3 minutes** until SSH `:32222` is up.
3. **Power On iOS** — Apple logo / restore bar, **not** SpringBoard. Log: `*_inferno\qemu-boot.log` should show `Auto Boot: false` and `-restore rd=md0`.
4. **Restore IPSW via SSH…** — when it finishes, QEMU **exits**. That is normal.

### A5. Filesystem patches (or the screen stays black)

Power Off. **File → Apply iOS filesystem patches…** → guest `root` only (**not** the companion). Success **clears Restore ramdisk**. Then Power On → Setup / SpringBoard.

### A6. Internet and IPA

Toolbar **Net** (or **File → Guest Internet / iOS Device Tools…**) → same SSH password → **Enable guest internet**. If `nmcli` / “strictly unmanaged” fails, [§11](#b11-guest-internet). **Install IPA…** needs a real `.ipa`, not a `.deb`.

### A7. Buttons

Toolbar: Vol− / Vol+ / Home / Side / SOS / Pad / Net. Swipe-home is unreliable — use **Home**.

---

# Part B — In depth (everything)

## B1. What you are running

| Piece | Role |
| --- | --- |
| **AQEMU** | Windows GUI, WSL launch, MACHINE paths, restore, patches, Device Tools |
| **Inferno** `qemu-system-applesoc` | Apple SoC guest (t8030 = iPhone 11 A13) |
| **Companion Linux** | Sees fake USB; runs `idevicerestore`, `usbmuxd`, `ideviceinstaller`, NetworkManager Shared |
| **iOS guest NVMe** | `*_inferno\root` (and nvram, firmware, SEP images). **Not** the companion disk |

Kernel-patch `info:` lines in the log are **normal**. Exit **139** after a short boot is often a QEMU crash, not “forgot patches.” Exit **1** with **write lock** on `root` means a second Inferno QEMU is still running — Power Off iOS; do not confuse that with closing the AQEMU window.

---

## B2. Get the IPSW

Preset that works in AQEMU today: **`t8030`**, **iPhone12,1**, model **`n104ap`**.

ChefKiss example (unsigned — needs a **forged ticket**):

`iPhone11,8,iPhone12,1_14.0_18A5351d_Restore.ipsw`  
(iOS **14.0 beta 5**, build **18A5351d**)

Sources: [The Apple Wiki](https://theapplewiki.com/), ipsw.me, and ChefKiss’s [file setup](https://chefkiss.dev/guides/inferno/file-setup/). Inferno support for **newer iOS** will grow; AQEMU follows Inferno, it does not invent SoC support.

Keep the `.ipsw` somewhere stable. You will set **MACHINE → IPSW (restore)** to this same file.

An `.ipsw` is a **ZIP**. You can rename to `.zip` and extract, or use AQEMU’s firmware tool.

---

## B3. Unpack the IPSW (File → iOS Firmware Tool)

**File → iOS Firmware Tool…** — do **not** run Python in a terminal. This dialog is the unpack + ticket + IM4P step.

1. **IPSW File** — browse to the `.ipsw` / `.zip`.
2. **Extraction Output Directory** — default is next to `aqemu.exe` as `firmware_extracted`. Pick a folder you will keep.
3. **Unpack IPSW** (Step 1). AQEMU fills **BuildManifest.plist**, suggests DeviceTree `.im4p`, restore ramdisk, and (if empty) MACHINE **IPSW**.

Typical files after unpack (names vary by IPSW):

| Look for | Used as |
| --- | --- |
| `BuildManifest.plist` | Filled into Step 2 automatically |
| `DeviceTree.n104ap.im4p` | Step 3 decrypt/extract → **DeviceTree** `.dtb` / `.dec` |
| `kernelcache.release.*` or research kernel | **Kernel** (decrypted / `.research`) |
| Large restore **`.dmg`** | MACHINE **Restore ramdisk (-initrd)** (suggested if empty) |
| `Firmware/all_flash/sep-firmware.n104.RELEASE.im4p` | Later pack into **repackaged** SEP `.img4` — not the MACHINE SEP field raw |
| Trustcache / img4 pieces from the Inferno recipe | **Trustcache** |
| `kernelcache` / firmware folders | Keep the whole extract; do not delete until MACHINE paths work |

**Kernel** is the research/decrypted kernelcache Inferno can `-kernel`, not a random Mach-O from the IPSW payload folder unless ChefKiss says that file is the one.

---

## B4. Tickets, SEP ROM, SEP firmware (same Firmware Tool window)

Stay in **File → iOS Firmware Tool…**. Do **not** copy `python3 create_apticket.py …` from older notes.

### Restore ticket + SEP ticket (Step 2)

Unsigned IPSWs need a **forged AP ticket**. A 2-byte placeholder will fail restore.

1. **Model** — `n104ap` (iPhone 11 / t8030).
2. **BuildManifest.plist** — already filled after Unpack.
3. **ticket.shsh2** — **Browse…** or **ChefKiss extras…** ([ChefKiss extras](https://chefkiss.dev/Extras/Inferno/)). You can drop the file next to the bundled scripts as `extras/Inferno/ticket.shsh2`. AQEMU remembers the path. AQEMU does **not** ship this blob.
4. **Forge restore + SEP tickets** — AQEMU runs the bundled scripts (installs `pyasn1` via pip if needed). You never type the command.

Outputs (next to the extract by default):

- `root_ticket.der` → applied to MACHINE **Restore ticket** (used as `idevicerestore -T`)
- `sep_root_ticket.der` → input for packing SEP firmware (next)

Forge again whenever you **change IPSW build**. Python 3: **File → Configure → iOS firmware tools**, or on PATH (`py -3` / `python`). AQEMU starts it — you do not open a shell.

### DeviceTree / kernel IM4P (Step 4)

IM4P files are still wrapped. Step 4 needs **`pyimg4`** on PATH or `pyimg4.exe` beside `aqemu.exe` (AQEMU does not vendor it).

| Operation | Use |
| --- | --- |
| Extract raw payload | Pull payload out of `.im4p` |
| Decrypt payload | Needs **AES IV** and **AES Key** (hex) from The Apple Wiki for that build/file |
| Show payload info | Inspect before decrypt |

**DeviceTree must not stay `.im4p`.** Start will refuse raw IM4P. Decrypt/extract until you have `.dtb` or `.dec`. Empty MACHINE DeviceTree is filled from a `.dec`/`.dtb` result.

If pyimg4 is missing: **File → Configure → iOS firmware tools → pyimg4**, or copy `pyimg4.exe` next to `aqemu.exe` / on PATH.

### Repackaged SEP firmware (Step 3 in the same window)

Raw `sep-firmware.n104.RELEASE.im4p` is **not** what Inferno `sep-fw=` wants. Without a **repackaged** `sep-firmware.n104.RELEASE.new.img4`, restore often dies after NORData: **`Could not read data (-256)`**.

Still in **File → iOS Firmware Tool…**:

1. **SEP .im4p** — filled after Unpack (`Firmware/all_flash/sep-firmware.n104.RELEASE.im4p`).
2. **IVKEY** — Apple Wiki IV and key **concatenated** (no space) for that file. **Apple Wiki…** opens the keys page.
3. **Decrypt + pack SEP firmware** — AQEMU runs `img4` decrypt then wrap with `-T rsep` and the Step 2 SEP ticket. MACHINE **SEP firmware** is set to the `.new.img4`.

Place **`img4.exe`** (xerub [img4lib](https://github.com/xerub/img4lib)) in **File → Configure → iOS firmware tools**, beside `aqemu.exe`, or on PATH. You do not type the ChefKiss `for /f` command.

### SEP ROM and SecureROM

SEP ROM for t8030 / iPhone 11 is the **Cebu B1** dump from ChefKiss’s ROM collection (filename says Cebu B1). MACHINE → **SEP ROM**.

**SecureROM** is optional. Leave empty unless you have the dump and Inferno expects it.

---

## B5. Every AQEMU field

### Wizard / main

| Control | What to set |
| --- | --- |
| Guest | **iOS (ARM64)** |
| Emulator | `qemu-system-applesoc` (WSL on Windows) |
| Machine | **`t8030`** (s8000 = older A9 path; this guide is t8030) |
| RAM | **≥ 4 GiB** |
| vCPUs | **4** is a sane default |
| Network tab | **Ignore for guest internet.** Reverse-tether only (Part B §11) |

### Boot area (DeviceTree / Kernel)

| Field | What to set |
| --- | --- |
| **DeviceTree (.dtb / .im4p)** | Extracted **`.dtb` or `.dec`**. Not wrapped `.im4p` |
| **Kernel (.research / .elf)** | Decrypted / research kernelcache |
| **Boot arguments** | Leave Inferno/AQEMU defaults unless ChefKiss says otherwise. Do not paste random `-append` into Additional Args |

### MACHINE → Options (Apple SoC)

| Field | What to set |
| --- | --- |
| **Trustcache** | Trustcache file from IPSW / Inferno recipe |
| **Restore ticket** | `root_ticket.der` (real, not placeholder) |
| **SEP firmware** | `sep-firmware.n104.RELEASE.new.img4` (repackaged) |
| **SEP ROM** | Cebu B1 dump |
| **SecureROM (optional)** | Empty unless you have it |
| **IPSW (restore)** | The `.ipsw` / `.zip` Restore dialog uploads |
| **Restore ramdisk (-initrd)** | Restore **`.dmg`**. **Required for restore.** When set, AQEMU also passes `boot-mode=enter_recovery` so old NVRAM `auto-boot=true` cannot skip recovery. **Must be empty** for SpringBoard. FS-patch success clears it |
| **Disable KASLR (kaslr-off)** | **On** (default). Uncheck only if you want KASLR |
| **USB remote** | Type **IPv4 localhost (recommended)**, address **`127.0.0.1`**, port **`8030`**. Restore → **Apply localhost preset** writes the same. UNIX `/tmp/InfernoUSBRemote` is the other Inferno option; ChefKiss prefers IPv4 8030. Companion and iOS **must match** |

Save the VM after filling this. Start validates kernel + DeviceTree; missing/wrong types get a clear error.

### Restore dialog (companion)

| Control | What to set |
| --- | --- |
| Companion **disk path** | Like a HDD image box: existing `.qcow2` / image. **Later: Download Debian companion**, then this path still points at that file. Do not install Ubuntu by hand from this guide |
| SSH user / password | Linux user in that image (example `bob`) |
| SSH port | **32222** (hostfwd) |
| USB remote | Same **127.0.0.1:8030** |
| **Start companion in WSL** | Boots companion + `usb-tcp-remote` |
| **Stop companion** | Before wiping disks / resizing `root` if needed |
| **Diagnose** | USB / SSH / 8030 checks |
| **Restore IPSW via SSH…** | Uploads IPSW, runs patched `idevicerestore` **inside** the companion |

Patched `idevicerestore` rewrites Inferno **`N104DEV` → AP**. Without it: `Unable to discover device type`.

### After restore

| Control | What |
| --- | --- |
| **File → Apply iOS filesystem patches…** | Also MACHINE tab, Restore dialog, post-restore prompt. Target **guest `root`** |
| **File → Guest Internet / iOS Device Tools…** | Internet + IPA |
| Session toolbar | Vol / Home / Side / SOS / Pad / **Net** |

---

## B6. Grow `root` (32 GiB)

ChefKiss: `qemu-img create -f raw root 32G`. AQEMU auto-creates **8 GiB** (~9 GB in Settings). Enough for SpringBoard; **not** enough for large IPAs (UTM SE died around **800 MB free** with `PackageExtractionFailed`).

Images live in:

```text
<VM folder>\<VM_name>_inferno\
  root, firmware, syscfg, ctrl_bits, nvram, effaceable, panic_log, sep_nvram, sep_ssc
```

Example: `C:\Users\<you>\AQEMU_VM\iOS_ARM64__inferno\`

`root` may be a **symlink** (e.g. `D:\aqemu-backups\iOS_ARM64_root.fs-patched`). Resize and patch the **target file**.

**Power Off iOS** (no leftover `qemu-system-applesoc`). PowerShell:

```powershell
$path = 'D:\aqemu-backups\iOS_ARM64_root.fs-patched'   # or ...\iOS_ARM64__inferno\root
fsutil sparse setflag $path
$fs = [System.IO.File]::Open($path, 'Open', 'ReadWrite')
$fs.SetLength(32GB)
$fs.Close()
(Get-Item $path).Length / 1GB
```

If `root` does not exist yet: **Power On once** (creates empties), **Power Off**, then resize.

Growing the file **after** iOS is installed does **not** change Settings until APFS is recreated (**re-restore** onto the larger disk) or grown from a guest shell (stock Inferno usually has **no** NewTerm — NewTerm 2/3 are **.deb**, not IPA).

---

## B7. Restore sequence (order matters)

1. Restore ramdisk **filled**, USB **8030**, VM saved.  
2. **Start companion** — wait for `:32222`.  
3. **Power On iOS** — restore UI, not SpringBoard.  
4. `qemu-boot.log`: `Auto Boot: false`, `-restore rd=md0`, `nand-enable-reformat=1`.  
5. If Auto Boot stays **true**: wipe `nvram` or full NVMe wipe ([§13](#b13-re-install-ios)); keep ramdisk set so `enter_recovery` applies.  
6. **Restore IPSW via SSH…**  
7. QEMU **exits** when restore completes — expected. **Do not** look for SpringBoard yet.

| Symptom | Fix |
| --- | --- |
| `Unable to discover device type` | Companion `idevicerestore` missing DEV→AP patch |
| `Could not read data (-256)` after NORData | Wrong SEP; need **repackaged** `.new.img4`. Partial flash: wipe NVMe, restore again |
| Tiny / placeholder ticket | Forge `root_ticket.der` for **this** IPSW |
| Device never appears | Companion + iOS up, **same** 8030, wait, **Diagnose** |
| Write lock on `root` | Second Inferno QEMU still running |

USB for restore lives **inside the companion**, not Windows Device Manager.

---

## B8. Filesystem patches (required)

Without this, VNC/SPICE stays **black forever**.

Patches the **iOS guest `root`**, **never** the companion.

1. iOS Powered Off.  
2. **File → Apply iOS filesystem patches…** → **Use current VM root**.  
3. Windows runs `extras/Inferno/apply-fs-patches-wsl.sh` (temporary Ubuntu KVM + linux-apfs): InfernoFSPatcher on `dyld_shared_cache_arm64e`, then disables launchd (CommCenter, locationd, voicemail, …).  
4. Success **clears Restore ramdisk**.  
5. Script picks the iOS NVMe by skipping the ~12 GiB patch-VM boot disk — works for **8 GiB and 32 GiB** `root`.

Manual macOS `hdiutil` path: [fs-patches](https://chefkiss.dev/guides/inferno/fs-patches/).  
Second-pass launchd-only: `apply-launchd-only-wsl.sh` with `ROOT_IMG` set (no hardcoded home paths).

---

## B9. First SpringBoard boot

1. MACHINE **Restore ramdisk** empty.  
2. Companion may stay up (USB / internet / IPA).  
3. **Power On** — TCG is slow. Setup (“Welcome to iPhone”) then SpringBoard.  
4. Use **Home**, not swipe-home.

| Toolbar | Inferno |
| --- | --- |
| Vol− | F3 |
| Vol+ | F4 |
| Side (power) | F5 (hold uses QEMU `sendkey` hold_ms) |
| Home | F6 (double = App Switcher) |
| SOS | Hold Vol+, then Side |
| Pad | Floating pad (not an iPhone bezel) |
| Net | Device Tools / internet |

---

## B10. Menu map

| Task | Where |
| --- | --- |
| Unpack IPSW / IM4P | File → **iOS Firmware Tool…** |
| Companion + restore | File → **Apple SoC Restore** |
| SpringBoard patches | File / MACHINE / Restore → **Apply iOS filesystem patches…** |
| Internet + IPA + screenshot | File / VM → **Guest Internet / iOS Device Tools…** or **Net** |
| Hardware buttons | Session toolbar |

---

## B11. Guest internet (Safari / App Store)

Wi‑Fi/cellular are **not** emulated. **Do not** use AQEMU **Network** cards.

1. Companion running, iOS **On**, USB **8030**.  
2. **Net** or **Guest Internet / iOS Device Tools…**  
3. Same SSH user/password as Restore.  
4. **Enable guest internet (reverse-tether)** — `USBMUXD_DEFAULT_DEVICE_MODE=3` + NetworkManager **Shared** on USB/NCM (often `enxdeadbeef2212`), **not** the companion’s main NIC (`enp0s2`).

Companion SSH uses `accept-new` and `~/.aqemu-companion-known_hosts` (loopback only). If you rebuild the companion and SSH fails, delete that known_hosts file.

**If `nmcli missing`:**

```bash
sudo apt-get update
sudo apt-get install -y network-manager
sudo systemctl enable --now NetworkManager
```

Then **Enable** again.

**If activation fails with “strictly unmanaged” / a nonsense `lo` error** (Ubuntu Server):

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

Success: `idevice_id -l` prints a UDID (example `00008030-1122334455667788`) and `enx*` is **connected** to `aqemu-share-…`. Then Safari / App Store.

**Diagnose USB / idevice** and **Copy manual commands** are on the same Internet tab.

---

## B12. Install an IPA

Device Tools → **Device** → **Install IPA…**

```bash
sudo apt-get install -y ideviceinstaller
```

- Must be a **`.ipa`** (`Payload/*.app`). Chariz **NewTerm 2/3 `.deb`** will not install this way.  
- Missing `iTunesMetadata.plist` / `.sinf` warnings are common.  
- **`PackageExtractionFailed` at ~15%**: bad IPA, zip-in-zip, or **not enough guest free space**. Grow APFS via **re-restore onto 32 GiB `root`** before huge apps (UTM SE).  
- AQEMU only runs `ideviceinstaller` over USB; signing/jailbreak policy is yours.

Screenshot: Device Tools **Screenshot** (file on companion `/tmp/aqemu-ios.png`).

---

## B13. Re-install iOS

Use when Settings stays ~9 GB after a host resize, restore half-flashed, or SpringBoard never appears.

**A.** Power Off iOS. Stop leftover Inferno QEMU. Optionally stop companion.

**B.** Wipe guest NVMe (iOS off):

```powershell
powershell -File extras\Inferno\wipe-ios-nvme.ps1 -VmXml "C:\Users\<you>\AQEMU_VM\iOS_ARM64_.aqemu"
```

Type `YES`. Deletes `root`, `nvram`, `firmware`, … under `*_inferno\`. If `root` is a **symlink**, delete/replace the **target** too.

**C.** Power On once (recreates **8 GiB** empties) → Power Off → sparse-resize `root` to **32 GiB** ([§6](#b6-grow-root-32-gib)).

**D.** Put **Restore ramdisk** back. Save. Start companion. Power On iOS. Confirm restore boot args. **Restore IPSW via SSH…**

**E.** FS patches → initrd cleared → Power On → SpringBoard. Enable internet again. Reinstall IPAs.

You do **not** recreate the VM or re-unpack firmware if MACHINE paths are still valid. You **do** need a ticket for this IPSW build.

**Re-restore without full wipe** (same disk size, NAND OK): set ramdisk; wipe **`nvram`** only if Auto Boot stays true; companion → Power On → restore → patches → clear initrd → Power On.

| Host resize `root` file | NVMe bigger; Settings still ~9 GB until APFS recreate/resize |
| --- | --- |
| Re-restore onto larger empty `root` | Settings should show ~32 GB |
| `diskutil apfs resizeContainer` in guest | Needs a shell; usually unavailable |

No Settings switch for “make disk bigger.”

---

## B14. What this guide does not cover

- Baking the companion OS from an Ubuntu ISO (future **Download** + disk **path**, like HDD).  
- Obtaining IPSWs, SHSH, SEP keys, or Apple firmware (you + ChefKiss + Apple Wiki).  
- Building Inferno QEMU from source.  
- Cydia / Sileo / NewTerm **`.deb`** workflows.

---

## B15. Happy-path checklist

1. iOS (ARM64), `t8030`, all MACHINE firmware + USB `127.0.0.1:8030`  
2. Companion **path** set (download later) → Start companion → SSH `:32222`  
3. `root` **32 GiB** before restore  
4. Power On iOS in restore (ramdisk set)  
5. Restore IPSW via SSH → QEMU exits  
6. FS patches on guest `root` → ramdisk cleared  
7. Power On → Setup / SpringBoard  
8. **Net** → Enable guest internet (NM / unmanaged fix if needed)  
9. **`.ipa` only**; watch free space  
