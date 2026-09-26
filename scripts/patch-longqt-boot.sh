#!/bin/bash
set -euo pipefail
ISO="${1:-/mnt/c/Users/chron/Downloads/LongQT-OpenCore-v0.7.iso}"
DXE="${2:-/tmp/ocdrv/OpenPartitionDxe.efi}"
OUT="${3:-/mnt/c/Users/chron/AQEMU_VM/macOS_OpenCore_BOOT.img}"
REPO_DXE='/mnt/c/Users/chron/CURSOR-PROJECTS/aqemu/third_party/opencore/OpenPartitionDxe.efi'
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

7z e -y -o"$TMP" "$ISO" BOOT.img >/dev/null
cp "$TMP/BOOT.img" "$OUT"
# FAT images need offset=0 for mtools when whole file is the FS
export MTOOLS_SKIP_CHECK=1
mcopy -o -i "$OUT" "$DXE" ::/EFI/OC/Drivers/OpenPartitionDxe.efi
echo '=== verify ==='
mdir -i "$OUT" ::/EFI/OC/Drivers
mkdir -p "$(dirname "$REPO_DXE")"
cp "$DXE" "$REPO_DXE"
ls -la "$OUT" "$REPO_DXE"
echo OK
