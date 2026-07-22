# Privacy Policy — AQEMU (Chronic Engineering)

**Last updated:** 2026-07-22  
**Product:** AQEMU (QEMU frontend)  
**Publisher:** Chronic Engineering  
**Source:** https://github.com/chronic8000/aqemu

## Summary

AQEMU is a desktop application that configures and launches [QEMU](https://www.qemu.org/) virtual machines on your computer. **It does not require an online account** and **does not intentionally collect, transmit, or sell personal data** to Chronic Engineering.

## Data stored on your device

AQEMU may store configuration and VM definitions **locally** on your machine, for example:

- Application settings (paths to QEMU, UI preferences)
- Virtual machine configuration files and related paths you choose
- Optional log files written to your user profile / app data directory

You control those files. Uninstalling or deleting them removes that local data (subject to how your OS handles app data folders).

## Network activity

When you start a guest, QEMU (and optionally WSL) may use the network as configured **by you** (user-mode networking, bridges, SPICE/QMP on localhost, etc.). That traffic is between your host, QEMU, and destinations you configure — **not** telemetry to Chronic Engineering.

AQEMU may open links you click (for example documentation or the project GitHub page) in your default browser.

## Third parties

- **Microsoft Store** (if you install from the Store): Microsoft’s privacy policy applies to Store account, purchase, and update delivery.
- **QEMU / WSL / guest operating systems**: separate software with their own behavior and policies. AQEMU does not ship Microsoft Windows installation media, Apple operating system images, OpenCore builds, or an Apple SMC OSK; any such files you supply remain under your responsibility.

## Children

AQEMU is a general-purpose development / virtualization tool and is not directed at children.

## Changes

We may update this policy in the repository. The “Last updated” date will change when we do.

## Contact

Questions about this policy: open an issue at https://github.com/chronic8000/aqemu/issues or use the contact method listed on the Microsoft Store listing (when published).
