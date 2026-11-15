# Networking

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

Want your VM on the internet? In most cases you don't have to do anything: the wizard picks **User Mode / NAT** and a network card that suits your guest.

---

## 1. Pick a network type in the wizard

The wizard's **Network** page says: "Choose how this VM reaches the network (virt-manager-style). User Mode works everywhere; Bridge/TAP need host setup."

| Choice | What it means | Use it when |
| --- | --- | --- |
| **User Mode / NAT** (recommended) | QEMU's built-in NAT ("SLIRP / usernet"). The guest can reach the internet; nothing outside can reach the guest unless you forward a port. No admin rights needed. Works on Windows and Linux. | Almost always |
| **Bridge** | Joins a host bridge (Linux: `virbr0` / `br0`). The guest appears on your LAN like a real PC. | You need the guest visible on your network, and you've set up a bridge |
| **TAP** | A host virtual interface (Linux/macOS). Advanced. | You know why you need it |
| **No Network** | No network card at all | Offline or very old guests |

## 2. The Network tab

After the VM exists, everything is on the **Network** tab, which has three sub-tabs.

### Network

Tick **Enable network**, then choose a **Network GUI type**:

- **Basic** — one card per row, with a **Connection mode** such as "Use the user mode network stack", "Open a TUN/TAP interface", "Open a listening TCP socket", "Connect to listening TCP socket" or "Create shared VLAN via UDP multicast socket". Pick the **Network card model** here too.
- **Native** — the full QEMU options. **Connection type** can be `nic`, `user`, `channel`, `bridge`, `tap`, `socket` or `multicast socket` (plus `vde` and `dump` when your QEMU supports them), with every QEMU option (`hostfwd`, `smb`, `tftp`, `macaddr`, …) exposed.

**Generate New MAC** gives a card a fresh MAC address. **File → Preferences… → Network** chooses how new MACs are made: random, QEMU's `52:54:00:XX:XX:XX` range, or a valid address for each card model.

### Redirections (port forwarding)

With User Mode, the outside world can't reach the guest. To run a server in the guest (SSH, a web server, a game server…), forward a port:

1. Tick **Enable redirections**.
2. Click **Add Item**.
3. Set **Protocol** (**TCP** or **UDP**), **Host port**, **To guest IP** and **Guest port**.

Example: host port `2222` → guest port `22` lets you run `ssh -p 2222 user@localhost` on the host. (With QEMU's user mode network, the guest's own address is normally `10.0.2.15`.)

### TFTP/SAMBA

- **TFTP folder:** serves a host folder over TFTP (handy for network booting).
- **Allow SMB access to:** shares a host folder with the guest over Samba. A Samba server (`smbd`) must be installed on the host, networking must be on and set to user mode, and the guest finds the folder at `\\10.0.2.4\qemu`. This is a Linux-host feature; on Windows, use [Paste files](file-transfer.md).

## 3. Which network card?

The wizard picks a card model for your guest. The per-OS tables in the [guest OS guides](../guest-os/README.md) show the card AQEMU starts with. Across AQEMU's own OS profiles:

| Guests | Card AQEMU usually picks |
| --- | --- |
| Linux distributions | `virtio-net-pci` (46 of 47; Linux on IBM Z gets `virtio-net-ccw`) |
| Most Windows versions | `e1000e` |
| Older Windows, DOS and OS/2 | `ne2k_pci` (NE2000) or `rtl8139` |
| ReactOS | `e1000` |
| Intel macOS | `virtio-net-pci` (forced) |

**VirtIO** is the fastest, but the guest needs VirtIO drivers. Modern Linux has them built in; Windows needs a VirtIO driver disc that you supply. If a guest can't see its network card, switch the card to one it has a driver for.

The **Apply VirtIO defaults** button on the Machine tab is made for the Windows 11 ARM / kiosk profile. It sets `machine=virt`, VirtIO GPU, VirtIO disk and network, USB tablet/keyboard/RNG and USB audio in one go, so don't use it on a PC-type (x86) VM.

## 4. Apple SoC VMs

On experimental Apple SoC (iOS) VMs, the **Net** button on the session toolbar is something different: it opens the companion's guest-internet tool, "Not the AQEMU Network NIC tab". See [iOS (Apple SoC)](../ios-apple-soc.md).

---

[Documentation Hub](../../README.md) · [File transfer](file-transfer.md) · [Troubleshooting](troubleshooting.md)
