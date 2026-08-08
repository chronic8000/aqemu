# AQEMU Windows installers

Two packages, two channels:

| Package | Script | Use for |
|--|--|--|
| **MSI** | `build-msi.ps1` | Website / Stripe (you charge customers yourself) |
| **MSIX** | `build-msix.ps1` | Microsoft Store **paid** listings (Store checkout + price tiers) |

InstallShield is not used (commercial Flexera product).

---

## MSI (website / Stripe)

WiX Toolset → classic installer under `Program Files\AQEMU`.

```powershell
powershell -ExecutionPolicy Bypass -File installer\build-msi.ps1
```

Output: `installer\out\AQEMU-1.3.0-win64.msi`

Silent install: `msiexec /i AQEMU-1.3.0-win64.msi /qn`

Requires: [WiX CLI](https://wixtoolset.org/) (`winget install WiXToolset.WiXCLI`), then `wix eula accept wix7` once (or let the script accept it).

---

## MSIX (Microsoft Store, paid)

Desktop Bridge package (`runFullTrust`) with the same built-in QEMU payload. This is what Partner Center needs for **real price tiers** and Store commerce.

```powershell
powershell -ExecutionPolicy Bypass -File installer\build-msix.ps1
```

Output: `installer\out\AQEMU-1.3.0-win64.msix`

Requires: Windows 10/11 SDK (`makeappx.exe`, `signtool.exe`).

### Before Store upload — match Product identity

In Partner Center, create an **MSIX or PWA** app (not EXE/MSI), open **Product identity**, then rebuild with those values:

```powershell
powershell -ExecutionPolicy Bypass -File installer\build-msix.ps1 `
  -Publisher "CN=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" `
  -IdentityName "YourPublisher.AQEMU" `
  -Version "1.3.0.0"
```

- **Publisher** must match the Store `CN=...` exactly (signing cert subject must match too).
- For local sideload testing, the script creates `installer\certs\aqemu-msix-test.pfx` (password `aqemu-msix-dev`). Install the `.cer` into **Trusted People** (and enable Developer Mode / sideloading).
- Partner Center often re-signs for Store distribution after you upload; still ship a package whose Identity matches your reserved name.

### Store checklist

1. New product type: **MSIX or PWA app**
2. Upload `.msix`
3. Availability → **Paid** → pick a **price tier**
4. Reuse the same listing text / screenshots from the MSI submission where possible

---

## Payload

Both scripts stage from `build_win\` (`aqemu.exe`, Qt/SPICE DLLs, bundled `qemu-*.exe`, plugins). Build AQEMU there first.
