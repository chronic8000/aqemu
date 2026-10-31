# Tobimensch issue triage

Upstream: https://github.com/tobimensch/aqemu/issues  
Tracking: https://github.com/chronic8000/aqemu/issues/12  

We cannot close issues on tobimensch/aqemu. Fixes land here. File new bugs on **chronic8000/aqemu**.

| Upstream | Title | Bucket | Notes |
|----------|-------|--------|-------|
| [#6](https://github.com/tobimensch/aqemu/issues/6) | improve "add new VM" wizard | `done` | Wizard rewritten + wizard_trees.json |
| [#10](https://github.com/tobimensch/aqemu/issues/10) | New logo | `done` | Logo present in resources/icons |
| [#15](https://github.com/tobimensch/aqemu/issues/15) | Youtube/Video Tutorial/Review / Screenshots | `done` | Screenshots in README/screenshots/ |
| [#17](https://github.com/tobimensch/aqemu/issues/17) | network: bridge support | `done` | `-net bridge` / helper path in native + classic |
| [#18](https://github.com/tobimensch/aqemu/issues/18) | crashes on VM / screenshot | `tracked` | Session/screenshot path hardened; reopen if repro on 1.0+ |
| [#19](https://github.com/tobimensch/aqemu/issues/19) | TFTP/SAMBA tab overrides network tab | `done` | Merged into existing `-net user` (no duplicate) |
| [#23](https://github.com/tobimensch/aqemu/issues/23) | full screen | `done` | Session toolbar fullscreen (SPICE/VNC) |
| [#26](https://github.com/tobimensch/aqemu/issues/26) | Three patches (GIT) | `tracked` | Review individually if still relevant |
| [#30](https://github.com/tobimensch/aqemu/issues/30) | CPU-count Bug | `tracked` | SMP UI; reopen with repro |
| [#35](https://github.com/tobimensch/aqemu/issues/35) | bios rom | `done` | Firmware/OVMF flows |
| [#36](https://github.com/tobimensch/aqemu/issues/36) | Mouse Grab | `tracked` | SPICE/VNC grab; host-dependent |
| [#40](https://github.com/tobimensch/aqemu/issues/40) | FreeBSD power/snapshots | `tracked` | Guest ACPI; reopen if still broken |
| [#41](https://github.com/tobimensch/aqemu/issues/41) | -append with spaces | `done` | Quoted custom-args parsing |
| [#42](https://github.com/tobimensch/aqemu/issues/42) | Memory leak | `tracked` | No single known leak; profile if needed |
| [#44](https://github.com/tobimensch/aqemu/issues/44) | Deprecated vlan | `done` | No `vlan=` on modern QEMU |
| [#45](https://github.com/tobimensch/aqemu/issues/45) | project idle | `wontfix` | Meta — this fork is active |
| [#46](https://github.com/tobimensch/aqemu/issues/46) | Translates not enabled | `done` | `QTranslator` now member of `AQEMU_Main` |
| [#48](https://github.com/tobimensch/aqemu/issues/48) | donations | `wontfix` | Out of scope |
| [#54](https://github.com/tobimensch/aqemu/issues/54) | hostfwd `...` placeholder | `done` | Empty guest IP omitted |
| [#55](https://github.com/tobimensch/aqemu/issues/55) | Qt4 cmake confusion | `done` | Qt5 CMake |
| [#57](https://github.com/tobimensch/aqemu/issues/57) | QEMU 3.0 era | `done` | Superseded by modern QEMU support |
| [#58](https://github.com/tobimensch/aqemu/issues/58) | vlan=0 breaks start | `done` | vlan not emitted on modern QEMU |
| [#59](https://github.com/tobimensch/aqemu/issues/59) | multiple `-net user` | `done` | hostfwd/TFTP/SMB merge into one user net |
| [#63](https://github.com/tobimensch/aqemu/issues/63) | UEFI | `done` | OVMF / firmware |
| [#67](https://github.com/tobimensch/aqemu/issues/67) | Is it alive? | `wontfix` | Meta — yes, this fork |
| [#70](https://github.com/tobimensch/aqemu/issues/70) | snap | `wontfix` | Not pursuing |
| [#71](https://github.com/tobimensch/aqemu/issues/71) | image format | `done` | qemu-img probe + filters |
| [#72](https://github.com/tobimensch/aqemu/issues/72) | doesn't start | `tracked` | Vague; need repro on chronic8000 |
| [#73](https://github.com/tobimensch/aqemu/issues/73) | GPLv3 | `wontfix` | Stay GPLv2 |
| [#74](https://github.com/tobimensch/aqemu/issues/74) | GCC 10 compile | `tracked` | Build on current toolchain; reopen if needed |
| [#75](https://github.com/tobimensch/aqemu/issues/75) | QEMU 5.0 recognition | `done` | `--version` parse + real label |
| [#76](https://github.com/tobimensch/aqemu/issues/76) | USB passthrough / bus | `done` | `usb-bus.0` + VID:PID fallback |
| [#79](https://github.com/tobimensch/aqemu/issues/79) | VirtualBox FAQ | `wontfix` | FAQ / out of scope |
| [#81](https://github.com/tobimensch/aqemu/issues/81) | Conservancy | `wontfix` | Org/policy |
| [#82](https://github.com/tobimensch/aqemu/issues/82) | German language | `done` | Translations load fix (#46); `.qm` shipping TBD |
| [#83](https://github.com/tobimensch/aqemu/issues/83) | help links broken | `done` | `resources/docs/links.html` updated |
| [#84](https://github.com/tobimensch/aqemu/issues/84) | Gtk-WARNING | `wontfix` | Host theme noise |
| [#85](https://github.com/tobimensch/aqemu/issues/85) | Fullscreen broken | `done` | Session fullscreen |
| [#86](https://github.com/tobimensch/aqemu/issues/86) | CPU count changes | `tracked` | Reopen with repro |
| [#87](https://github.com/tobimensch/aqemu/issues/87) | folder sharing | `done` | virtfs + docs; Windows→SMB tip |
| [#88](https://github.com/tobimensch/aqemu/issues/88) | Sync interfaces | `wontfix` | Vague |
| [#89](https://github.com/tobimensch/aqemu/issues/89) | Segfault / deps | `tracked` | Distro packaging; reopen with backtrace |
| [#90](https://github.com/tobimensch/aqemu/issues/90) | Meta rant | `wontfix` | Meta |
| [#92](https://github.com/tobimensch/aqemu/issues/92) | AUR segfault | `tracked` | Distro packaging |
| [#102](https://github.com/tobimensch/aqemu/issues/102) | VMDK | `done` | Filter / open path |
| [#105](https://github.com/tobimensch/aqemu/issues/105) | VM comment field | `tracked` | Enhancement backlog |
| [#106](https://github.com/tobimensch/aqemu/issues/106) | Disable/enable media | `done` | Session insert/eject CD & floppy |
| [#107](https://github.com/tobimensch/aqemu/issues/107) | aarch64 binary | `done` | Full qemu-system-* discovery |
| [#108](https://github.com/tobimensch/aqemu/issues/108) | TCP_KEEPIDLE | `done` | `#ifdef` guarded + Win keepalive |
| [#109](https://github.com/tobimensch/aqemu/issues/109) | deprecated options | `done` | e.g. `disable-ticketing=on`, no vlan |
| [#110](https://github.com/tobimensch/aqemu/issues/110) | CD-ROM becomes Disk | `done` | Persist media + force Use_Media for CD |
| [#111](https://github.com/tobimensch/aqemu/issues/111) | USB add breaks start | `done` | usb-bus.0 + skip empty bus/port |
| [#112](https://github.com/tobimensch/aqemu/issues/112) | Windows Releases | `done` | Portable zip + Store MSIX |
| [#113](https://github.com/tobimensch/aqemu/issues/113) | Arch segfault GUI | `tracked` | Distro; reopen with backtrace |
| [#114](https://github.com/tobimensch/aqemu/issues/114) | AUR segfault | `tracked` | Distro packaging |
| [#115](https://github.com/tobimensch/aqemu/issues/115) | Debian running VMs | `tracked` | Env-specific; reopen here |
| [#118](https://github.com/tobimensch/aqemu/issues/118) | Raspberry Pi OS icon | `done` | Template/icon |
| [#119](https://github.com/tobimensch/aqemu/issues/119) | operator precedence | `done` | Parentheses on restrict/vnet_hdr/vhost |
| [#120](https://github.com/tobimensch/aqemu/issues/120) | Shared folder DOS | `done` | Document SMB for Windows/DOS guests |
| [#121](https://github.com/tobimensch/aqemu/issues/121) | QEMU not detected | `done` | First Start / path discovery |
| [#122](https://github.com/tobimensch/aqemu/issues/122) | Changing floppies | `done` | Session QMP/HMP media change |
| [#123](https://github.com/tobimensch/aqemu/issues/123) | Basic net UI hide | `done` | Refresh widgets after add card |
| [#124](https://github.com/tobimensch/aqemu/issues/124) | USB stick not visible | `done` | Same USB bus/VID path as #76/#111 |
| [#125](https://github.com/tobimensch/aqemu/issues/125) | shared folder not visible | `done` | virtfs mount tip + SMB for Windows |
| [#126](https://github.com/tobimensch/aqemu/issues/126) | Architecture not available | `done` | Full arch discovery |
| [#130](https://github.com/tobimensch/aqemu/issues/130) | CPU Type list PowerPC | `done` | PowerPC CPU regex |
| [#131](https://github.com/tobimensch/aqemu/issues/131) | Detected as QEMU 2.0 | `done` | `--version` + UI label |
| [#132](https://github.com/tobimensch/aqemu/issues/132) | Sound disabled (bad version) | `done` | Follows #131 |
| [#133](https://github.com/tobimensch/aqemu/issues/133) | compatmonitor0 | `tracked` | Monitor/serial mapping |
| [#134](https://github.com/tobimensch/aqemu/issues/134) | hub 0 not connected | `done` | Pair nic+user without vlan |
| [#136](https://github.com/tobimensch/aqemu/issues/136) | disable-ticketing | `done` | `disable-ticketing=on` |
| [#137](https://github.com/tobimensch/aqemu/issues/137) | Pre-built versions | `done` | Releases + Store |

## Summary counts
- **done**: ~48
- **tracked**: ~14 (need repro on chronic8000 or enhancement backlog)
- **wontfix**: 10
- **total open upstream**: 72

## Waves completed (this fork)
1. **Launch/discovery** — version label, arch discovery, SPICE ticketing, deprecated CLI  
2. **Network** — no vlan on modern QEMU, merged hostfwd/TFTP/SMB, basic UI refresh  
3. **USB/media** — usb-bus.0, CD-ROM persist, floppy change, shared-folder docs  
4. **Polish** — translator lifetime, TCP_KEEPIDLE, CPU PowerPC parse, precedence, help links  
