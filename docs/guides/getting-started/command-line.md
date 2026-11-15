# Using AQEMU from the Command Line

<!-- site:skip-start -->_Docs v1 · checked against the AQEMU source on `master`, 31 October 2026._<!-- site:skip-end -->

AQEMU has a small command-line interface for scripting: start, stop and query VMs without clicking. This page is the full list of commands from the source (`src/main.cpp`).

> **Linux only (in practice).** The commands talk to the running AQEMU over the **D-Bus session bus**. Linux desktops have one; Windows normally does not, so on Windows use the GUI.

---

## Commands

```
aqemu start     ( <AQEMU_FILE> | <VM_Name> )
aqemu stop      ( <AQEMU_FILE> | <VM_Name> )
aqemu shutdown  ( <AQEMU_FILE> | <VM_Name> )
aqemu pause     ( <AQEMU_FILE> | <VM_Name> )
aqemu reset     ( <AQEMU_FILE> | <VM_Name> )
aqemu save      ( <AQEMU_FILE> | <VM_Name> ) [ <Tag> ]
aqemu load      ( <AQEMU_FILE> | <VM_Name> ) [ <Tag> ]
aqemu monitor   ( <AQEMU_FILE> | <VM_Name> )
aqemu error     ( <AQEMU_FILE> | <VM_Name> )
aqemu control   ( <AQEMU_FILE> | <VM_Name> )
aqemu status    [ <AQEMU_FILE> | <VM_Name> ]
aqemu list
```

You can name a VM by its **name** (as shown in the VM list) or by the **path to its `.aqemu` file**.

| Command | What it does |
| --- | --- |
| `start` | Starts the VM |
| `stop` | Powers the VM off (like pulling the plug) |
| `shutdown` | Asks the guest to shut down (ACPI power button) |
| `pause` | Pauses the VM |
| `reset` | Resets the VM |
| `save` | Saves the VM state, optionally with a tag |
| `monitor` | Opens the QEMU monitor window |
| `error` | Opens the QEMU error log window |
| `control` | Opens the emulator control window |
| `status` | Shows the VM's state |
| `list` | Lists your VMs |

All commands except `start` and `load` expect the VM to be running already.

> `load` is listed in the help text, but in the current source it isn't connected to an action. Don't rely on it yet.

## Options

| Option | Meaning |
| --- | --- |
| `-h`, `--help` | Show the help |
| `--version` | Show the version |
| `--test-crash` | Write a local crash report and exit (for testing crash reporting) |
| `--style THEME` | Set the Qt theme/style |

Running `aqemu` with no command opens the normal window.

## Examples

```sh
aqemu list
aqemu start "Obscure OS"
aqemu stop "$HOME/.aqemu/Obscure OS.aqemu"
```

## See the QEMU command instead

If you want to run QEMU yourself without AQEMU, select the VM and use **VM → Tools → Create Shell Script**. **VM → Tools → Show QEMU Arguments** shows the command line without saving it.

---

[Documentation Hub](../../README.md) · [Menus reference](../../reference/menus.md)
