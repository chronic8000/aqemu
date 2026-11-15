# Menus reference

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Every item in AQEMU's menu bar, as AQEMU builds it on start. Items marked *(Windows)* are greyed out on Linux.

---

## File

| Item | What it does |
| --- | --- |
| **Disks → Storage Browser…** | "Browse disk images and ISOs in your VM folder" |
| **Disks → Create HDD Image** | Make a new disk image |
| **Disks → Convert HDD Image** | Convert a disk image |
| **Disks → Convert disk…** | `qemu-img convert` to qcow2, raw, vmdk, vhdx or vpc |
| **Disks → qemu-nbd…** | Share a disk read-only on `127.0.0.1:10809` |
| **Import and Export → Virtual Machine Scanner…** | "Search a folder for .aqemu files and add those virtual machines" |
| **Import and Export → Import Appliance (OVA / OVF)…** | Import an OVA or OVF appliance |
| **Import and Export → Export Appliance (OVA)…** | Export the selected VM as an appliance |
| **Import and Export → Import lab pack… / Export lab pack…** | Lab packs (XML; "Disks stay linked, not embedded") |
| **Import and Export → Quickemu conf…** | Read a Quickemu `.conf` file |
| **Host → Remote Hosts…** | "SSH tunnels to remote QEMU, or open libvirt/Xen/LXC in virt-manager" |
| **Host → Configure WSL…** *(Windows)* | WSL distro and user for KVM-in-WSL |
| **Host → Audio…** | Host audio settings |
| **Preferences…** | AQEMU-wide settings. See [Preferences](settings/preferences.md). |
| **First Start Wizard** | Run the first-start setup again |
| **Exit** | Quit AQEMU |

## VM

| Item | What it does |
| --- | --- |
| **Add VM → Add New VM** | Opens the [New VM Wizard](../guides/features/new-vm-wizard.md) |
| **Add VM → Load VM From File** | Add an existing `.aqemu` file |
| **Delete VM** | Remove the VM from AQEMU |
| **Save As Template** | Save this VM's settings as a template |
| **Copy** | Clone the VM |
| **Start**, **Connect** (Ctrl+Shift+C), **Pause**, **Shutdown**, **Stop**, **Reset**, **Save** | Power controls. **Connect** opens a running VM's screen. |
| **Serial monitor**, **QEMU monitor** | Consoles for the running VM |
| **Apple →** | Experimental Apple SoC tools. See [iOS (Apple SoC)](../guides/ios-apple-soc.md). |
| **Recovery → Storage Recovery…** | Build a read-only recovery VM from damaged disks |
| **Recovery → Recovery Checkpoint…** | Protect a VM's disks with a recovery overlay ([Snapshots](../guides/features/snapshots.md)) |
| **Storage → Manage Snapshots** | qcow2 snapshots |
| **Storage → Attach Remote SAN Storage (iSCSI / NVMe-oF)…** | Connect a network disk |
| **Tools →** | Serial monitor, QEMU monitor, Show Emulator Control, Show QEMU Arguments, Create Shell Script, Show QEMU Error Log Window, Change Icon, Audio… |
| **Lab →** | Power-user tools: Lab…, Command diff…, Block jobs…, Migrate…, Chain Studio…, Firmware library…, Argument snippets…, Linked clones…, Start tagged group…, Stop tagged group… |

## Help

| Item | What it does |
| --- | --- |
| **About AQEMU** | Version and credits |
| **Copy setup for a GitHub issue** | Copies your AQEMU/Qt/host/QEMU details to the clipboard |
| **Open crash dumps folder** | Opens the crash dump folder |
| **QEMU Help Browser…** | QEMU's own documentation |
| **Buy AQEMU** | Microsoft Store build only. Opens the Store page. |
| **Diagnostics →** | Support bundle…, QEMU catalog…, QEMU bundle auditor…, WHPX health… *(Windows)*, First-run cards…, WSL dashboard… *(Windows)*. See [Troubleshooting](../guides/features/troubleshooting.md). |

---

[Documentation Hub](../README.md) · [Settings tab map](settings/README.md) · [Preferences](settings/preferences.md)
