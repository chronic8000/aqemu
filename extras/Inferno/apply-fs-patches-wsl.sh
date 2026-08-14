#!/usr/bin/env bash
# Apply ChefKiss Inferno filesystem patches to an iOS guest root NVMe image.
# Used by AQEMU: File → Apply iOS filesystem patches… / post-restore dialog.
#
# Required:
#   ROOT_IMG   Absolute path to the guest *_inferno/root raw image (or symlink).
# Optional:
#   WORK       Scratch dir (default: $HOME/aqemu-inferno-fs-patch)
#   EXTRAS_DIR AQEMU extras/Inferno (for a prebuilt inferno_fs_patcher if present)
#
# NOT the IPSW restore companion disk — only the Apple SoC guest root.
set -euo pipefail

if [[ -z "${ROOT_IMG:-}" ]]; then
  echo "ERROR: ROOT_IMG is not set (path to iOS guest *_inferno/root)"
  exit 1
fi

# Resolve symlink (Windows AQEMU may point root -> D:\aqemu-backups\...)
ROOT_REAL="$(readlink -f "$ROOT_IMG" 2>/dev/null || true)"
if [[ -z "$ROOT_REAL" || ! -e "$ROOT_REAL" ]]; then
  ROOT_REAL="$ROOT_IMG"
fi
if [[ ! -f "$ROOT_REAL" ]]; then
  echo "ERROR: root image not found: $ROOT_IMG (resolved: $ROOT_REAL)"
  exit 1
fi

WORK="${WORK:-$HOME/aqemu-inferno-fs-patch}"
EXTRAS_DIR="${EXTRAS_DIR:-}"
CLOUD_IMG="$WORK/ubuntu-24.04-minimal-cloudimg-amd64.img"
RUNTIME_IMG="$WORK/patch-vm.qcow2"
ROOT_WORK="$WORK/root-to-patch"
SEED_DIR="$WORK/seed"
PAYLOAD_DIR="$WORK/payload"
PATCHER_SRC="$WORK/InfernoFSPatcher"
PATCHER_BIN="$PATCHER_SRC/build/inferno_fs_patcher"

mkdir -p "$WORK" "$SEED_DIR" "$PAYLOAD_DIR"
cd "$WORK"

echo "=== AQEMU Inferno FS patches ==="
echo "ROOT_IMG=$ROOT_IMG"
echo "ROOT_REAL=$ROOT_REAL"
echo "WORK=$WORK"

echo "=== InfernoFSPatcher ==="
if [[ -x "$EXTRAS_DIR/inferno_fs_patcher" ]]; then
  PATCHER_BIN="$EXTRAS_DIR/inferno_fs_patcher"
  echo "Using bundled patcher: $PATCHER_BIN"
elif [[ ! -x "$PATCHER_BIN" ]]; then
  if [[ ! -d "$PATCHER_SRC/.git" ]]; then
    git clone --depth 1 https://git.chefkiss.dev/AppleHax/InfernoFSPatcher "$PATCHER_SRC" \
      || git clone --depth 1 https://github.com/ChefKissInc/InfernoFSPatcher "$PATCHER_SRC"
  fi
  cmake -S "$PATCHER_SRC" -B "$PATCHER_SRC/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$PATCHER_SRC/build" -j"$(nproc)"
fi
cp -f "$PATCHER_BIN" "$PAYLOAD_DIR/inferno_fs_patcher"
chmod +x "$PAYLOAD_DIR/inferno_fs_patcher"

echo "=== Cloud image ==="
if [[ ! -f "$CLOUD_IMG" ]] || [[ "$(stat -c%s "$CLOUD_IMG")" -lt 100000000 ]]; then
  curl -L --progress-bar -o "$CLOUD_IMG.partial" \
    https://cloud-images.ubuntu.com/minimal/releases/noble/release/ubuntu-24.04-minimal-cloudimg-amd64.img
  mv -f "$CLOUD_IMG.partial" "$CLOUD_IMG"
fi
rm -f "$RUNTIME_IMG"
qemu-img convert -O qcow2 "$CLOUD_IMG" "$RUNTIME_IMG"
qemu-img resize "$RUNTIME_IMG" 12G

echo "=== Root working copy ==="
# Always refresh from source so we patch the disk AQEMU is using
cp -f "$ROOT_REAL" "$ROOT_WORK"
ls -lh "$ROOT_WORK"

echo "=== Guest patch script ==="
cat > "$PAYLOAD_DIR/do-patch.sh" <<'EOS'
#!/usr/bin/env bash
set -euxo pipefail
LOG=/mnt/payload/do-patch.log
exec > >(tee -a "$LOG") 2>&1
echo "=== Inferno FS patch starting $(date) ==="
uname -a
lsblk -o NAME,SIZE,TYPE,FSTYPE,MOUNTPOINT || true

ROOT_DEV=""
BEST=0
# Boot OS is vda/sda (~12GiB cloud image). iOS root is the next virtio-blk
# (8GiB default or 32GiB ChefKiss). Seed/payload images are much smaller.
for d in /dev/vdb /dev/vdc /dev/vdd /dev/vde /dev/sdb /dev/sdc /dev/sdd /dev/sde; do
  [[ -b "$d" ]] || continue
  SZ=$(blockdev --getsize64 "$d" || echo 0)
  echo "disk $d size=$SZ"
  if [[ "$SZ" -ge 4000000000 && "$SZ" -gt "$BEST" ]]; then
    BEST=$SZ
    ROOT_DEV="$d"
  fi
done
[[ -n "$ROOT_DEV" ]] || { echo "ERROR: root device not found"; exit 1; }
echo "ROOT_DEV=$ROOT_DEV"

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq build-essential git "linux-headers-$(uname -r)" python3 gdisk fdisk || \
  apt-get install -y -qq build-essential git linux-headers-generic python3 gdisk fdisk

cd /tmp
rm -rf linux-apfs-rw
git clone --depth 1 https://github.com/linux-apfs/linux-apfs-rw.git
cd linux-apfs-rw
make -j"$(nproc)"
modprobe libcrc32c || true
insmod ./apfs.ko || { dmesg | tail -40; exit 4; }

mkdir -p /mnt/ios
MOUNTED=0
try_mount() {
  local t="$1"
  [[ -e "$t" ]] || return 1
  echo "Trying mount $t (readwrite)"
  if mount -t apfs -o readwrite "$t" /mnt/ios 2>/tmp/apfs.err; then
    echo "Mounted $t"
    touch /mnt/ios/.aqemu_write_test 2>/tmp/apfs-w.err && rm -f /mnt/ios/.aqemu_write_test && echo "write-test OK" && return 0
    echo "mount succeeded but not writable:"; cat /tmp/apfs-w.err || true
    umount /mnt/ios || true
  fi
  cat /tmp/apfs.err || true
  return 1
}

while read -r part; do
  [[ -n "$part" ]] || continue
  if try_mount "/dev/$part"; then MOUNTED=1; break; fi
done < <(lsblk -nr -o NAME,FSTYPE | awk '$2=="apfs"{print $1}')

if [[ "$MOUNTED" -ne 1 ]]; then
  for t in "${ROOT_DEV}1" "${ROOT_DEV}2" "${ROOT_DEV}3" "${ROOT_DEV}4" "$ROOT_DEV"; do
    if try_mount "$t"; then MOUNTED=1; break; fi
  done
fi

if [[ "$MOUNTED" -ne 1 ]]; then
  gdisk -l "$ROOT_DEV" || fdisk -l "$ROOT_DEV" || true
  LOOP=$(losetup -f --show -b 4096 -P "$ROOT_DEV")
  echo "LOOP=$LOOP"
  sleep 1
  for t in "$LOOP" "${LOOP}p1" "${LOOP}p2" "${LOOP}p3" "${LOOP}p4" "${LOOP}p5" "${LOOP}p6"; do
    if try_mount "$t"; then MOUNTED=1; break; fi
  done
fi
[[ "$MOUNTED" -eq 1 ]] || { echo "ERROR: APFS mount failed"; dmesg | tail -50; exit 2; }

echo "=== Top of mount ==="
ls -la /mnt/ios | head -50
DSC=$(find /mnt/ios -name 'dyld_shared_cache_arm64e' 2>/dev/null | head -1 || true)
[[ -n "$DSC" ]] || { echo "ERROR: dyld cache not found"; find /mnt/ios -maxdepth 5 -type d | head -100; exit 3; }
SYS="${DSC%/System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64e}"
echo "DSC=$DSC"
echo "SYS=$SYS"
LAUNCHD="$SYS/System/Library/xpc/launchd.plist"
ls -la "$DSC"
ls -la "$LAUNCHD"

echo "=== InfernoFSPatcher ==="
/mnt/payload/inferno_fs_patcher "$DSC"

echo "=== launchd.plist ==="
python3 - <<'PY'
import plistlib, shutil, pathlib
cands = list(pathlib.Path("/mnt/ios").rglob("System/Library/xpc/launchd.plist"))
assert cands, "launchd.plist not found"
p = cands[0]
print("editing", p)
shutil.copy2(p, str(p) + ".bak-pre-inferno")
with open(p, "rb") as f:
    data = plistlib.load(f)
services = [
    "com.apple.voicemail.vmd",
    "com.apple.CommCenter",
    "com.apple.CommCenterMobileHelper",
    "com.apple.CommCenterRootHelper",
    "com.apple.locationd",
]
changed = []

def find_entry(obj, name):
    if isinstance(obj, dict):
        if name in obj and isinstance(obj[name], dict):
            return obj[name]
        if obj.get("Label") == name:
            return obj
        for v in obj.values():
            r = find_entry(v, name)
            if r is not None:
                return r
    elif isinstance(obj, list):
        for i in obj:
            r = find_entry(i, name)
            if r is not None:
                return r
    return None

ld = data.get("LaunchDaemons") if isinstance(data, dict) else None
for name in services:
    entry = find_entry(data, name)
    if entry is None:
        print("WARN missing", name)
        continue
    entry["Disabled"] = True
    changed.append(name)
    print("Disabled", name)
if isinstance(ld, dict):
    for k, v in ld.items():
        if not isinstance(v, dict):
            continue
        kl = k.lower()
        if any(s in kl for s in ("commcenter", "voicemail", "locationd")):
            v["Disabled"] = True
            if k not in changed:
                changed.append(k)
                print("Disabled via ld-key", k)
print("CHANGED:", changed)
with open(p, "wb") as f:
    plistlib.dump(data, f, fmt=plistlib.FMT_BINARY)
for name in services:
    e = find_entry(plistlib.load(open(p, "rb")), name)
    print("verify", name, "Disabled=" + str(e.get("Disabled") if e else None))
print("done")
PY

sync
umount /mnt/ios || umount -l /mnt/ios
sync
echo "=== SUCCESS $(date) ===" | tee /mnt/payload/SUCCESS
EOS
chmod +x "$PAYLOAD_DIR/do-patch.sh"
rm -f "$PAYLOAD_DIR/SUCCESS" "$PAYLOAD_DIR/do-patch.log"

PAYLOAD_IMG="$WORK/payload.img"
rm -f "$PAYLOAD_IMG"
dd if=/dev/zero of="$PAYLOAD_IMG" bs=1M count=64 status=none
mkfs.vfat -n PAYLOAD "$PAYLOAD_IMG"
MTOOLS_SKIP_CHECK=1 mcopy -i "$PAYLOAD_IMG" "$PAYLOAD_DIR/do-patch.sh" ::do-patch.sh
MTOOLS_SKIP_CHECK=1 mcopy -i "$PAYLOAD_IMG" "$PAYLOAD_DIR/inferno_fs_patcher" ::inferno_fs_patcher

cat > "$SEED_DIR/user-data" <<'EOF'
#cloud-config
password: inferno
chpasswd: { expire: False }
ssh_pwauth: True
bootcmd:
  - mkdir -p /mnt/payload
  - |
    for i in 1 2 3 4 5 6 7 8 9 10; do
      if mount -L PAYLOAD /mnt/payload 2>/dev/null; then break; fi
      for d in /dev/vdd /dev/vdc /dev/vdb /dev/sdd /dev/sdc; do
        mount "$d" /mnt/payload 2>/dev/null && break 2
      done
      sleep 2
    done
runcmd:
  - [ bash, -lc, "chmod +x /mnt/payload/do-patch.sh /mnt/payload/inferno_fs_patcher; bash /mnt/payload/do-patch.sh; sync; poweroff -f" ]
EOF

cat > "$SEED_DIR/meta-data" <<EOF
instance-id: inferno-fs-patch-$(date +%s)
local-hostname: inferno-fs-patch
EOF

SEED_ISO="$WORK/seed.iso"
cloud-localds "$SEED_ISO" "$SEED_DIR/user-data" "$SEED_DIR/meta-data"
ls -lh "$SEED_ISO" "$PAYLOAD_IMG"

echo "=== Launching patch VM ==="
set +e
qemu-system-x86_64 \
  -name inferno-fs-patch \
  -machine q35,accel=kvm:tcg \
  -cpu max \
  -m 4096 \
  -smp 4 \
  -drive file="$RUNTIME_IMG",if=none,id=os,format=qcow2,cache=writeback,discard=unmap \
  -device virtio-blk-pci,drive=os,bootindex=1 \
  -drive file="$ROOT_WORK",if=none,id=iosroot,format=raw,cache=unsafe \
  -device virtio-blk-pci,drive=iosroot,physical_block_size=4096,logical_block_size=4096 \
  -drive file="$SEED_ISO",if=none,id=cidata,format=raw,readonly=on \
  -device virtio-blk-pci,drive=cidata \
  -drive file="$PAYLOAD_IMG",if=none,id=payload,format=raw,cache=unsafe \
  -device virtio-blk-pci,drive=payload \
  -netdev user,id=net0 \
  -device virtio-net-pci,netdev=net0 \
  -nographic \
  -serial mon:stdio
QEMU_RC=$?
set -e
echo "qemu exit=$QEMU_RC"

MTOOLS_SKIP_CHECK=1 mcopy -n -i "$PAYLOAD_IMG" ::SUCCESS "$PAYLOAD_DIR/SUCCESS" 2>/dev/null || true
MTOOLS_SKIP_CHECK=1 mcopy -n -i "$PAYLOAD_IMG" ::do-patch.log "$PAYLOAD_DIR/do-patch.log" 2>/dev/null || true

if [[ -f "$PAYLOAD_DIR/SUCCESS" ]]; then
  echo "=== Installing patched root back ==="
  cp -f "$ROOT_WORK" "$ROOT_REAL"
  sync
  echo "Patched root written to $ROOT_REAL"
  echo SUCCESS
  exit 0
fi

echo "PATCH FAILED"
tail -200 "$PAYLOAD_DIR/do-patch.log" 2>/dev/null || echo "(no log)"
exit 1
