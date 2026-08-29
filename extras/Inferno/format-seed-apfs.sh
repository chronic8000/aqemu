#!/usr/bin/env bash
# Format the AQEMU_SEED APFS partition on Inferno guest root with a Data volume.
# Usage: format-seed-apfs.sh /path/to/root
# Run as root (loop devices). First run may clone+build linux-apfs/apfsprogs.
set -euo pipefail

ROOT="${1:?path to *_inferno/root}"
if [[ ! -f "$ROOT" ]]; then
	echo "ERROR: root image not found: $ROOT"
	exit 1
fi

LBA=4096
START_LBA=256
OFFSET=$((START_LBA * LBA))
SIZE=$(stat -c%s "$ROOT")
BACKUP=$((6 * LBA))
PART_BYTES=$((SIZE - OFFSET - BACKUP))
if [[ "$PART_BYTES" -lt $((512 * 1024)) ]]; then
	echo "ERROR: APFS partition too small ($PART_BYTES)"
	exit 1
fi

magic() {
	dd if="$ROOT" bs=1 skip=$((OFFSET + 32)) count=4 status=none 2>/dev/null || true
}

if [[ "$(magic)" == "NXSB" ]]; then
	echo "APFS NXSB already present on $ROOT"
	exit 0
fi

WORK="${HOME}/aqemu-mkapfs"
MKAPFS="$WORK/apfsprogs/mkapfs/mkapfs"
if [[ ! -x "$MKAPFS" ]]; then
	echo "Building mkapfs once in $WORK ..."
	mkdir -p "$WORK"
	if [[ ! -d "$WORK/apfsprogs/.git" ]]; then
		git clone --depth 1 https://github.com/linux-apfs/apfsprogs.git "$WORK/apfsprogs"
	fi
	make -C "$WORK/apfsprogs/mkapfs" -j"$(nproc)"
fi

modprobe loop 2>/dev/null || true
LOOP=$(losetup -f --show -o "$OFFSET" --sizelimit "$PART_BYTES" "$ROOT")
cleanup() { losetup -d "$LOOP" 2>/dev/null || true; }
trap cleanup EXIT

echo "mkapfs Data volume on $LOOP (offset $OFFSET, $PART_BYTES bytes)"
"$MKAPFS" -L Data "$LOOP"
losetup -d "$LOOP"
trap - EXIT
LOOP=""

python3 - "$ROOT" "$OFFSET" <<'PY'
import struct, sys

ROLE_OFF = 0x3C4
APFS_VOL_ROLE_DATA = 0x0040
root, offset = sys.argv[1], int(sys.argv[2])
offset = int(offset)

def fletcher64(data: bytes) -> int:
    sum1 = 0
    sum2 = 0
    for i in range(0, len(data), 4):
        v = struct.unpack_from("<I", data, i)[0]
        sum1 += v
        sum2 += sum1
    c1 = sum1 + sum2
    c1 = 0xFFFFFFFF - (c1 % 0xFFFFFFFF)
    c2 = sum1 + c1
    c2 = 0xFFFFFFFF - (c2 % 0xFFFFFFFF)
    return ((c2 & 0xFFFFFFFF) << 32) | (c1 & 0xFFFFFFFF)

with open(root, "r+b") as f:
    f.seek(offset)
    blob = f.read(64 * 1024 * 1024)
found = None
for i in range(0, len(blob) - 4096, 4096):
    if blob[i + 32 : i + 36] == b"APSB":
        found = i
        break
if found is None:
    sys.exit("APSB not found after mkapfs")
block = bytearray(blob[found : found + 4096])
struct.pack_into("<H", block, ROLE_OFF, APFS_VOL_ROLE_DATA)
block[0:8] = b"\x00" * 8
struct.pack_into("<Q", block, 0, fletcher64(block))
with open(root, "r+b") as f:
    f.seek(offset + found)
    f.write(block)
print(f"APSB role=Data at partition offset {found}")
PY

if [[ "$(magic)" != "NXSB" ]]; then
	echo "ERROR: NXSB not present after mkapfs"
	exit 1
fi
echo "Seed APFS Data volume ready on $ROOT"
