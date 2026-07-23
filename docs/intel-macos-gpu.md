# Intel macOS graphics in AQEMU

## Defaults (everyone)

Intel macOS VMs default to **VMware SVGA** (`vmware-svga` with 128 MB VRAM). This works on Windows, WSL/KVM, and Linux without a passthrough GPU. It is software-accelerated guest graphics — not Metal.

## Metal / AMD GPU passthrough

Apple’s Metal stack in a QEMU guest needs a **real AMD PCIe dGPU** assigned with Linux **VFIO** (`-device vfio-pci,host=BB:DD.F`).

| Host | What you get |
|------|----------------|
| **Bare-metal Linux + AMD dGPU** | AMD GPU picker enabled (off by default). Bind the card to `vfio-pci`, then enable passthrough after install. |
| **Windows / WSL2 (any GPU)** | If an AMD GPU is detected, the picker is **shown greyed-out** with an explanation. WSLg/GPU-PV accelerates Linux apps; it does **not** PCIe-assign the GPU into QEMU. |
| **No AMD GPU** | Passthrough UI is **hidden**. Use VMware SVGA. |

Modern **NVIDIA** GPUs do not provide macOS Metal drivers. Do not expect Metal from an RTX card.

### Linux host prep (short)

1. Enable IOMMU in firmware (`intel_iommu=on` / `amd_iommu=on`).
2. Bind the AMD VGA (+ HDMI audio) to `vfio-pci` (see OSX-KVM `scripts/vfio-group.sh` and `notes.md`).
3. Create/install the VM with **passthrough off**.
4. In AQEMU → Intel macOS settings, select the AMD GPU and enable passthrough.
5. Attach a display to the passed-through GPU (or use Looking Glass / similar); emulated VGA is disabled while passthrough is on.

AQEMU does **not** automatically rebind drivers or edit kernel cmdline in this version.
