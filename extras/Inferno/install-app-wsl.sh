#!/usr/bin/env bash
# Copy an IPA's .app onto an Inferno iOS root, skipping installd.
# installd rejects ad-hoc and private-CA signatures (0xe8008014 / 0xe8008018).
# Inferno already patches the kernel to bypass code-signature checks, and Power On
# adds amfi_get_out_of_my_way=1 to -append (the Boot Args line).
# cs_enforcement_disable panics this kernel ("can't has cs_enforcement_disable").
#
# Required:
#   ROOT_IMG   Guest *_inferno/root raw image. iOS must be powered off.
#   IPA        Path to the .ipa. Its existing signature and entitlements are kept.
# Optional:
#   WORK       Scratch dir (default: $HOME/aqemu-inferno-app-install)
set -euo pipefail

if [[ -z "${ROOT_IMG:-}" || -z "${IPA:-}" ]]; then
  echo "ERROR: ROOT_IMG and IPA must both be set"
  exit 1
fi

ROOT_REAL="$(readlink -f "$ROOT_IMG" 2>/dev/null || true)"
if [[ -z "$ROOT_REAL" || ! -e "$ROOT_REAL" ]]; then
  ROOT_REAL="$ROOT_IMG"
fi
if [[ ! -f "$ROOT_REAL" ]]; then
  echo "ERROR: root image not found: $ROOT_IMG (resolved: $ROOT_REAL)"
  exit 1
fi
if [[ ! -f "$IPA" ]]; then
  echo "ERROR: IPA not found: $IPA"
  exit 1
fi

for cmd in qemu-system-x86_64 qemu-img cloud-localds mkfs.vfat mcopy python3; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "ERROR: $cmd is not installed in WSL."
    echo "The filesystem-patch helper needs qemu-system-x86, cloud-image-utils, and mtools."
    exit 1
  fi
done

if command -v fuser >/dev/null 2>&1; then
  if fuser "$ROOT_REAL" >/dev/null 2>&1; then
    echo "ERROR: $ROOT_REAL is in use. Power Off iOS and try again."
    exit 1
  fi
fi

WORK="${WORK:-$HOME/aqemu-inferno-app-install}"
CACHE="${HOME}/aqemu-inferno-fs-patch"
CLOUD_IMG="$CACHE/ubuntu-24.04-minimal-cloudimg-amd64.img"
RUNTIME_IMG="$WORK/patch-vm.qcow2"
ROOT_WORK="$WORK/root-to-patch"
SEED_DIR="$WORK/seed"
PAYLOAD_DIR="$WORK/payload"
APP_DIR="$WORK/app-src"

mkdir -p "$WORK" "$SEED_DIR" "$PAYLOAD_DIR" "$CACHE"
cd "$WORK"

echo "=== AQEMU install app onto iOS root ==="
echo "ROOT_IMG=$ROOT_IMG"
echo "ROOT_REAL=$ROOT_REAL"
echo "IPA=$IPA"
echo "WORK=$WORK"

echo "=== Extract IPA (signature and entitlements kept) ==="
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR"
python3 - "$IPA" "$APP_DIR" <<'PY'
import os, stat, sys, zipfile
from pathlib import Path
ipa, dest_root = sys.argv[1], Path(sys.argv[2])
with zipfile.ZipFile(ipa) as z:
    names = [n for n in z.namelist() if n.startswith("Payload/") and ".." not in Path(n).parts]
    apps = [n for n in names if n.rstrip("/").endswith(".app") or ".app/" in n]
    if not apps:
        sys.exit("ERROR: IPA has no Payload/*.app")
    for info in z.infolist():
        name = info.filename
        if not name.startswith("Payload/") or ".." in Path(name).parts:
            continue
        dest = dest_root / name
        mode = (info.external_attr >> 16) & 0xFFFF
        if name.endswith("/"):
            dest.mkdir(parents=True, exist_ok=True)
            continue
        dest.parent.mkdir(parents=True, exist_ok=True)
        if stat.S_ISLNK(mode):
            target = z.read(info)
            try:
                text = target.decode()
            except UnicodeDecodeError:
                text = target.decode("latin1")
            if dest.exists() or dest.is_symlink():
                dest.unlink()
            os.symlink(text, dest)
            continue
        with z.open(info) as src, open(dest, "wb") as out:
            while True:
                chunk = src.read(1024 * 1024)
                if not chunk:
                    break
                out.write(chunk)
        bits = mode & 0o777
        if bits:
            os.chmod(dest, bits)
print("extracted", dest_root)
PY
find "$APP_DIR/Payload" -maxdepth 1 -name '*.app' -type d -print

echo "=== Cloud image ==="
if [[ ! -f "$CLOUD_IMG" ]] || [[ "$(stat -c%s "$CLOUD_IMG")" -lt 100000000 ]]; then
  curl -L --progress-bar -o "$CLOUD_IMG.partial" \
    https://cloud-images.ubuntu.com/minimal/releases/noble/release/ubuntu-24.04-minimal-cloudimg-amd64.img
  mv -f "$CLOUD_IMG.partial" "$CLOUD_IMG"
fi
rm -f "$RUNTIME_IMG"
qemu-img convert -O qcow2 "$CLOUD_IMG" "$RUNTIME_IMG"
qemu-img resize "$RUNTIME_IMG" 12G

echo "=== Root working copy (16 GB class image; this takes several minutes) ==="
cp -f "$ROOT_REAL" "$ROOT_WORK"
ls -lh "$ROOT_WORK"

echo "=== Guest install script ==="
cat > "$PAYLOAD_DIR/do-install.sh" <<'EOS'
#!/usr/bin/env bash
set -euxo pipefail
LOG=/mnt/payload/do-install.log
exec > >(tee -a "$LOG") 2>&1
echo "=== Inferno app install starting $(date) ==="
uname -a
lsblk -o NAME,SIZE,TYPE,FSTYPE,LABEL,MOUNTPOINT || true

mkdir -p /mnt/src
modprobe 9p || true
modprobe 9pnet || true
modprobe 9pnet_virtio || true
if ! mount -t 9p -o trans=virtio,version=9p2000.L,msize=104857600 appsrc /mnt/src; then
  echo "ERROR: could not mount the IPA directory"
  dmesg | tail -40
  exit 6
fi
echo "=== IPA contents ==="
find /mnt/src/Payload -maxdepth 1 -mindepth 1 -print

ROOT_DEV=""
BEST=0
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
mkdir -p "$SYS/Applications"
echo "Applications: $(ls -ld "$SYS/Applications" || true)"

shopt -s nullglob
apps=(/mnt/src/Payload/*.app)
[[ ${#apps[@]} -gt 0 ]] || { echo "ERROR: no .app in Payload"; exit 5; }
for app in "${apps[@]}"; do
  name="$(basename "$app")"
  dest="$SYS/Applications/$name"
  echo "Installing $name -> $dest"
  rm -rf "$dest"
  cp -a "$app" "$dest"
  while IFS= read -r -d '' bin; do
    sig=$(od -An -t x1 -N 4 "$bin" | tr -d ' \n')
    if [[ "$sig" == "cffaedfe" || "$sig" == "cefaedfe" || "$sig" == "feedfacf" || "$sig" == "feedface" ]]; then
      chmod 755 "$bin"
    fi
  done < <(find "$dest" -type f -print0)
  du -sh "$dest" || true
  ls -ld "$dest"
done

sync
umount /mnt/ios || umount -l /mnt/ios
umount /mnt/src || true
sync
echo "=== SUCCESS $(date) ===" | tee /mnt/payload/SUCCESS
EOS
chmod +x "$PAYLOAD_DIR/do-install.sh"
rm -f "$PAYLOAD_DIR/SUCCESS" "$PAYLOAD_DIR/do-install.log"

PAYLOAD_IMG="$WORK/payload.img"
rm -f "$PAYLOAD_IMG"
dd if=/dev/zero of="$PAYLOAD_IMG" bs=1M count=8 status=none
mkfs.vfat -n PAYLOAD "$PAYLOAD_IMG"
MTOOLS_SKIP_CHECK=1 mcopy -i "$PAYLOAD_IMG" "$PAYLOAD_DIR/do-install.sh" ::do-install.sh

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
  - [ bash, -lc, "chmod +x /mnt/payload/do-install.sh; bash /mnt/payload/do-install.sh; sync; poweroff -f" ]
EOF

cat > "$SEED_DIR/meta-data" <<EOF
instance-id: inferno-app-install-$(date +%s)
local-hostname: inferno-app-install
EOF

SEED_ISO="$WORK/seed.iso"
cloud-localds "$SEED_ISO" "$SEED_DIR/user-data" "$SEED_DIR/meta-data"

echo "=== Launching helper VM ==="
set +e
qemu-system-x86_64 \
  -name inferno-app-install \
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
  -fsdev local,id=appfs,path="$APP_DIR",security_model=none \
  -device virtio-9p-pci,fsdev=appfs,mount_tag=appsrc \
  -netdev user,id=net0 \
  -device virtio-net-pci,netdev=net0 \
  -nographic \
  -serial mon:stdio
QEMU_RC=$?
set -e
echo "qemu exit=$QEMU_RC"

MTOOLS_SKIP_CHECK=1 mcopy -n -i "$PAYLOAD_IMG" ::SUCCESS "$PAYLOAD_DIR/SUCCESS" 2>/dev/null || true
MTOOLS_SKIP_CHECK=1 mcopy -n -i "$PAYLOAD_IMG" ::do-install.log "$PAYLOAD_DIR/do-install.log" 2>/dev/null || true

if [[ -f "$PAYLOAD_DIR/SUCCESS" ]]; then
  echo "=== Writing root image back ==="
  cp -f "$ROOT_WORK" "$ROOT_REAL"
  sync
  echo "App installed on $ROOT_REAL"
  echo "Power On iOS. The icon appears after SpringBoard starts."
  echo "Boot Args in the serial log must include amfi_get_out_of_my_way=1."
  echo SUCCESS
  exit 0
fi

echo "INSTALL FAILED"
tail -200 "$PAYLOAD_DIR/do-install.log" 2>/dev/null || echo "(no log)"
exit 1
