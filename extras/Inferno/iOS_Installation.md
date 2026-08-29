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

Toolbar: Vol− / Vol+ / Home / Side / SOS / Pad / Net. Swipe-home is ignored on Inferno (Face ID MT-SPI). After SpringBoard, use **Home**. On **Welcome to iPhone / Swipe up to get started**, Home often does nothing — use **SOS** (ChefKiss workaround).

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
| `Firmware/all_flash/DeviceTree.n104ap.im4p` | Step 4 decrypt/extract → MACHINE **DeviceTree** `.dtb` / `.dec` (not the SEP `.im4p`) |
| `kernelcache.release.*` or research kernel | **Kernel** (decrypted / `.research`) |
| Restore ramdisk **`.dmg`** (`RestoreRamDisk` in BuildManifest, e.g. `038-44135-124.dmg`) | MACHINE **Restore ramdisk (-initrd)** (suggested if empty). Not the largest IPSW `.dmg` |
| `Firmware/all_flash/sep-firmware.n104.RELEASE.im4p` | Later pack into **repackaged** SEP `.img4` — not the MACHINE SEP field raw |
| Trustcache / img4 pieces from the Inferno recipe | **Trustcache** |
| `kernelcache` / firmware folders | Keep the whole extract; do not delete until MACHINE paths work |

**Kernel** is the research/decrypted kernelcache Inferno can `-kernel`, not a random Mach-O from the IPSW payload folder unless ChefKiss says that file is the one.

### The three `.dmg` files in `firmware_extracted`

After unpack you will see **three `.dmg` files** at the root of `firmware_extracted` (names vary by IPSW build; the `18A5351d` example names are below):

| File | Role | Do you use it? |
| --- | --- | --- |
| `038-44087-125.dmg` | **UpdateRamdisk** — OTA delta update environment. Not used for a fresh restore | **No** — ignore it |
| `038-44135-124.dmg` | **RestoreRamdisk** — the recovery-mode environment that `idevicerestore` boots the device into. This is what goes in **MACHINE → Restore ramdisk (-initrd)** | **Yes — this is your `-initrd`** |
| `038-44337-083.dmg` | **RootFilesystem** — the full iOS APFS/HFS image. `idevicerestore` flashes this to the `root` NVMe image. You never set this field manually; the restore companion handles it automatically from the IPSW | **No** — handled automatically |

`BuildManifest.plist` (`RestoreRamDisk` key) identifies which `.dmg` is which. AQEMU's Firmware Tool reads that key and **pre-fills** the Restore ramdisk field for you, so you should not need to guess.

**How `-initrd` puts the device into recovery:**

When **Restore ramdisk (-initrd)** is set in MACHINE, AQEMU passes `-initrd <path>` to Inferno QEMU. QEMU maps that file as `md0` and appends `rd=md0 nand-enable-reformat=1 -restore` to the kernel boot args, which tells iOS to boot into restore mode instead of SpringBoard. The device is now in a state where `idevicerestore` (running in the companion) can connect over USB/TCP and flash the IPSW.

**After restore succeeds:** the FS-patch step **clears** the Restore ramdisk field automatically. Do not set it again unless you are doing a re-restore.

---

## B4. Tickets, SEP ROM, SEP firmware (same Firmware Tool window)

Stay in **File → iOS Firmware Tool…**. Do **not** copy `python3 create_apticket.py …` from older notes.

### Restore ticket + SEP ticket (Step 2)

Unsigned IPSWs need a **forged AP ticket**. A 2-byte placeholder will fail restore.

1. **Model** — `n104ap` (iPhone 11 / t8030).
2. **BuildManifest.plist** — already filled after Unpack.
3. **ticket.shsh2** — **not in the IPSW.** It is a **SHSH blob** ChefKiss hosts for Inferno’s unsigned-IPSW ticket scripts (contains an Apple **Img4** `ApImg4Ticket`). AQEMU does **not** ship it, download it, or put it in the installer — that would redistribute Apple signing material. You get it from ChefKiss:

   Open [Inferno file setup](https://chefkiss.dev/guides/inferno/file-setup/) → **Creating the AP Ticket** → the sentence *“a ticket shsh is provided here”* (that **here** is their `ticket.shsh2`). Save the file, then **Browse…** in the Firmware Tool. You may keep a local copy as `extras/Inferno/ticket.shsh2` (gitignored). Firmware Tool **How to get this…** opens that same guide page.
4. **Restore ticket / SEP ticket (.der)** — leave the suggested paths (next to the extract) or **Browse…** to choose where Forge **writes**. You do not hunt for these in the IPSW.
5. Click **Forge restore + SEP tickets** — that **creates** the `.der` files (installs `pyasn1` via pip if needed). You never type the command.

Outputs (next to the extract by default):

- `root_ticket.der` → applied to MACHINE **Restore ticket** (used as `idevicerestore -T`)
- `sep_root_ticket.der` → input for packing SEP firmware (next)

Forge again whenever you **change IPSW build**. Python 3: **File → Configure → iOS firmware tools**, or on PATH (`py -3` / `python`). AQEMU starts it — you do not open a shell.

### DeviceTree / kernel IM4P (Step 4)

Do **not** reuse `sep-firmware.n104.RELEASE.im4p` here — that file is Step 3 only. Step 4 is **DeviceTree** (and optionally kernelcache). Browse to:

`firmware_extracted/Firmware/all_flash/DeviceTree.n104ap.im4p`

IM4P files are still wrapped. Step 4 needs **`pyimg4`** on PATH or `pyimg4.exe` beside `aqemu.exe` (AQEMU does not vendor it).

| Operation | AES IV / AES Key | Use |
| --- | --- | --- |
| Show payload info | Leave blank | Check whether the IM4P is encrypted |
| Extract raw payload | Leave blank | Only if info shows no encryption |
| Decrypt payload | **Required** — DeviceTree (or kernel) IV and Key from The Apple Wiki for **this file and this build**. Not the Step 3 SEP IVKEY |

**DeviceTree must not stay `.im4p`.** Start will refuse raw IM4P. Decrypt until you have `.dtb` or `.dec`. Empty MACHINE DeviceTree is filled from that result. Kernel is usually ChefKiss’s **research** kernel, not a second pass on SEP.

If pyimg4 is missing: **File → Configure → iOS firmware tools → pyimg4**, or copy `pyimg4.exe` next to `aqemu.exe` / on PATH.

### Repackaged SEP firmware (Step 3 in the same window)

Raw `sep-firmware.n104.RELEASE.im4p` is **not** what Inferno `sep-fw=` wants. Without a **repackaged** `sep-firmware.n104.RELEASE.new.img4`, restore often dies after NORData: **`Could not read data (-256)`**.

Still in **File → iOS Firmware Tool…**:

1. **SEP .im4p** — filled after Unpack (`Firmware/all_flash/sep-firmware.n104.RELEASE.im4p`).
2. **IVKEY** — Apple Wiki keys for **this IPSW build** and the file **`sep-firmware.n104.RELEASE.im4p`** only. **Not iBoot, iBEC, iBSS, or LLB.**

   Open [The Apple Wiki firmware keys](https://theapplewiki.com/wiki/Firmware_Keys) (Firmware Tool **Apple Wiki…**). Open the page for **your** build + **iPhone 11 / iPhone12,1** (example GM: [Keys:Azul 18A373 (iPhone12,1)](https://theapplewiki.com/wiki/Keys:Azul_18A373_(iPhone12,1)); ChefKiss sample IPSW is **18A5351d** beta 5: [Keys:AzulSeed 18A5351d (iPhone12,1)](https://theapplewiki.com/wiki/Keys:AzulSeed_18A5351d_(iPhone12,1))).

   Find the heading **SEP-Firmware** / `sep-firmware.n104.RELEASE.im4p`. Copy **IV** then **Key**, lowercase hex, **no spaces, no `0x`, no newlines**:

   `IVKEY` = `IV` + `Key` (IV first, 32 hex chars, then the 64-char key → **96 hex characters**).

   Example from **18A5351d** SEP-Firmware (ChefKiss sample IPSW / iPhone12,1). Use this only if that is your IPSW:

   ```text
   IV:  017a328b048aab2edcc4cfe043c2d844
   Key: a55e67143d57938e37ec6b83ba9e181c0d24bd0a6a14f9f39752b967a9c45cfc
   ```

   Paste as one line:

   ```text
   017a328b048aab2edcc4cfe043c2d844a55e67143d57938e37ec6b83ba9e181c0d24bd0a6a14f9f39752b967a9c45cfc
   ```

   Wrong: iBoot keys. Wrong: **18A373 GM keys** on a **18A5351d** IPSW (or the reverse). Wrong: 14.7.1 `18G82` keys on 14.0. The Firmware Tool strips spaces; do not insert commas or `IV:` / `Key:` labels.
3. **Decrypt + pack SEP firmware** — AQEMU runs `img4` decrypt then wrap with `-T rsep` and the Step 2 SEP ticket. MACHINE **SEP firmware** is set to the `.new.img4`.

Place **`img4.exe`** (xerub [img4lib](https://github.com/xerub/img4lib), not `pyimg4.exe`) in **File → Configure → iOS firmware tools**, beside `aqemu.exe`, or on PATH. You do not type the ChefKiss `for /f` command.

On Windows the upstream tree is Unix-style; AQEMU’s port is:

```text
.\scripts\build_img4_windows.ps1
```

That produces `build_win\img4.exe` (MinGW + OpenSSL + in-tree lzfse). Needs MSYS2 UCRT64/MINGW64 `gcc` and OpenSSL.

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
| **DeviceTree (.dec / .dtb / .im4p)** | Firmware Tool `.dec` (or `.dtb`). Not wrapped `.im4p`. Browse filter includes `.dec` |
| **Kernel (.research / .elf)** | Decrypted / research kernelcache |
| **Boot arguments** | Leave Inferno/AQEMU defaults unless ChefKiss says otherwise. Do not paste random `-append` into Additional Args |

### MACHINE → Options (Apple SoC)

| Field | What to set |
| --- | --- |
| **Trustcache** | Trustcache file from IPSW / Inferno recipe |
| **Restore ticket** | `root_ticket.der` (real, not placeholder) |
| **SEP firmware** | **MACHINE tab → Options → SEP firmware** (same row — there is no second “Machine SEP” box). Packed `sep-firmware.n104.RELEASE.new.img4` from Firmware Tool Step 3. Not raw `.im4p` |
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
| **Wipe Inferno disks…** | After `-256` / partial flash. **File → Wipe Inferno disks…**, MACHINE **Wipe Inferno disks…**, or this Restore button. Shows the selected VM’s `.aqemu` path and `*_inferno` folder (not hardcoded). Power Off iOS first. Does not wipe companion.qcow2 |
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

ChefKiss: `qemu-img create -f raw root 32G`. Pick **NAND (root) size** on MACHINE, Restore, or the New VM disk page: **16 / 32 / 64 / 128 / 256 GiB** or **Custom** (16–2048 GiB). That size is used when `root` is **created**. Growing the file after iOS is installed does **not** change Settings until you **Wipe Inferno disks**, Power On (new `root` at the chosen size), then restore.

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
| `Could not read data (-256)` after NORData | **SEP firmware** on MACHINE is still raw `.im4p`, or Restore IPSW build ≠ ticket/SEP pack. Power Off → **Wipe Inferno disks…** → pack `.new.img4` → Restore the **same** IPSW |
| `invalid GPT header` / status 78 | Empty zeros, or **Storage with GPT format** (valid empty GPT). Update-path ramrod needs an APFS **Data** volume. Power Off, Power On (AQEMU formats seed APFS via WSL mkapfs). |
| `Missing data volume` / status 75 | APFS partition exists but has no Data role volume. Same Power On format step. |
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
4. **Do not swipe** from the bottom. Inferno’s touch (MT-SPI) ignores Home / App Switcher swipes ([ChefKiss #53](https://github.com/ChefKissInc/Inferno/issues/53)).  
5. After you have a Home screen, **Home** (F6) works. On **Welcome to iPhone / Swipe up to get started**, Home is ignored (that screen wants a Face ID swipe, and iPhone12,1 has no Home button). After the display times out to the lock screen, Home may wake/unlock — that is expected.  
6. To leave Welcome: click toolbar **SOS** (must **hold Vol+ and Side together**). If SOS only shows the volume HUD, the buttons did not overlap — click **Monitor** and run `sendkey f4-f5 2500` (do not Power Off). When slide-to-power-off appears, either tap the **X**, then **double-click Home** (App Switcher) and close Setup, **or** slide to power off, wait for a blank-ish display, then **Power On** / Reset (do not Wipe, do not Restore).

| Toolbar | Inferno |
| --- | --- |
| Vol− | F3 |
| Vol+ | F4 |
| Side (power) | F5 (hold uses QEMU `sendkey` hold_ms) |
| Home | F6 (double = App Switcher). Does nothing on Welcome swipe-up. |
| SOS | Hold Vol+, then Side — use this on Welcome |
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

**A.** Power Off iOS. Optionally **Stop companion**.

**B.** **File → Wipe Inferno disks…** (or Restore dialog **Wipe Inferno disks…**). Confirm. AQEMU uses the **selected** iOS VM’s `.aqemu` file — the dialog shows that path and the `*_inferno` folder next to it. No PowerShell.

**C.** Power On once (recreates empty images) → Power Off → grow `root` in the GUI if you use 32 GiB ([§6](#b6-grow-root-32-gib)).

**D.** Put **Restore ramdisk (-initrd)** back. Save. **Start companion**. Power On iOS. Confirm restore boot args. **Restore IPSW via SSH…** (IPSW must match MACHINE **IPSW (restore)** / tickets / SEP pack).

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
