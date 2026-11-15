# Display, mouse and keyboard

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

This guide covers where the guest's screen appears, how the mouse behaves, screen size, and remote viewing over VNC or SPICE.

---

## 1. Where the guest appears

Open the **Display** tab and find **How to show this VM**. This setting **applies to every virtual machine** and takes effect the next time you **Start** a guest.

| Option | What you get |
| --- | --- |
| **Auto — separate window for legacy guests, embedded for others** | DOS, Windows 9x and the XP family open in a separate QEMU window. Everything else opens inside AQEMU. |
| **Embedded in AQEMU — SPICE / VNC session viewer** | The guest's screen opens inside the AQEMU window, with the [session toolbar](session-toolbar.md) on top. |
| **Separate QEMU window — SDL / GTK** | The guest opens in QEMU's own window. Works for any VM. |
| **No graphics (-nographic) — serial console** | No screen at all, just the serial console (same as `qemu-system-ARCH -nographic`). Good for servers and boards with a serial port. |

For a separate window, **QEMU -display backend** chooses the frontend: Default (AQEMU picks SDL or GTK), `sdl`, `gtk`, `none`, `curses` or `egl-headless`.

On Windows, the embedded viewer uses VNC. (AQEMU moves older Windows installs off SPICE, because the SPICE client could crash on channel errors.)

## 2. Mouse: Seamless or Relative

On the **Machine** tab, **Pointer mode** is the setting that matters most:

- **Seamless** — an absolute tablet. Host and guest cursors stay lined up and you never have to "grab" the mouse. Best for modern guests.
- **Relative** — a PS/2-style mouse. Click the guest screen to capture the mouse; press **Esc** (or **Ctrl+Alt**, or middle-click) to release it. Use this for Windows 9x and other guests with no tablet driver.

**Mouse device**, **USB controller** and **USB version** are set for you by Pointer mode. Change them only if you need to. For example, the tooltip says USB 1.1 (UHCI) often works better with Windows 98.

## 3. Graphics card and screen size

On the Machine tab:

- **Graphics** picks the virtual graphics card. The wizard chooses one for your guest (for example Cirrus for Windows 98, standard VGA for ReactOS, VirtIO GPU for Windows 11 ARM).
- **Resolution** sets the guest screen size. With a VirtIO GPU it is passed to the guest; on Intel macOS VMs AQEMU patches the OpenCore resolution on start (**Native** = your host's physical pixels, up to 4096×2160).
- **Keyboard** sets the keyboard layout QEMU uses.

## 4. Remote viewing (VNC Remote and SPICE Remote)

These are optional, for connecting from another program or another computer. You don't need them for the normal embedded display.

**VNC Remote:** tick **Enable VNC**, then choose a **VNC display number** (the port is 5900 + the number) or a **UNIX domain socket**. Under **Security** you can require a password (set when the VM starts) and TLS with x509 certificate folders.

**SPICE Remote:** tick **Enable SPICE**, set **Host**, **Port** and optional **Secure port**, and a password (or **Enable client connection with no password**). You can also set image compression, the renderer, video stream detection and CELT playback compression.

> **Safety:** a VNC or SPICE port with no password lets anyone who can reach that port see and control the VM. Set a password, and don't expose the port to the internet.

## 5. QEMU Window options

The **QEMU Window** sub-tab only matters when the guest uses a separate QEMU window. Options include **Show QEMU window without frame**, **Use Ctrl-Alt-Shift to grab the mouse** (instead of Ctrl-Alt), **Disable QEMU window closing**, **Curses**, **Show cursor** and an **Initial graphical mode** (width, height, depth).

## 6. Full screen

- In the embedded viewer, click **Fullscreen** on the session toolbar. Hold the mouse at the top centre of the screen to bring the toolbar back.
- To always start full screen, tick **Fullscreen mode** under **Options** on the Machine tab.

---

[Documentation Hub](../../README.md) · [Session toolbar](session-toolbar.md) · [Troubleshooting](troubleshooting.md)
