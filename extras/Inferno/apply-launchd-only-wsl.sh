#!/usr/bin/env bash
# Second-pass: disable ChefKiss launchd services on already dyld-patched root.
set -euo pipefail

WORK="${WORK:-$HOME/inferno-fs-patch}"
ROOT_WORK="$WORK/root-to-patch"
ROOT_IMG="${ROOT_IMG:-/mnt/c/Users/chron/AQEMU_VM/iOS_ARM64__inferno/root}"
CLOUD_IMG="$WORK/ubuntu-24.04-minimal-cloudimg-amd64.img"
RUNTIME_IMG="$WORK/patch-vm-launchd.qcow2"
PAYLOAD_DIR="$WORK/payload-launchd"
SEED_DIR="$WORK/seed-launchd"

mkdir -p "$PAYLOAD_DIR" "$SEED_DIR"
cd "$WORK"

[[ -f "$ROOT_WORK" ]] || { echo "missing $ROOT_WORK"; exit 1; }

cat > "$PAYLOAD_DIR/do-launchd.sh" <<'EOS'
#!/usr/bin/env bash
set -euxo pipefail
LOG=/mnt/payload/do-launchd.log
exec > >(tee -a "$LOG") 2>&1
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq build-essential git "linux-headers-$(uname -r)" python3 || \
  apt-get install -y -qq build-essential git linux-headers-generic python3

cd /tmp
rm -rf linux-apfs-rw
git clone --depth 1 https://github.com/linux-apfs/linux-apfs-rw.git
cd linux-apfs-rw
make -j"$(nproc)"
modprobe libcrc32c || true
insmod ./apfs.ko

mkdir -p /mnt/ios
ROOT_DEV=/dev/vdb
mount -t apfs -o readwrite "${ROOT_DEV}1" /mnt/ios

python3 - <<'PY'
import plistlib, shutil, pathlib, pprint
p = pathlib.Path("/mnt/ios/System/Library/xpc/launchd.plist")
assert p.exists(), p
shutil.copy2(p, str(p) + ".bak-pre-launchd-disable")
with open(p, "rb") as f:
    data = plistlib.load(f)
print("TOP KEYS:", list(data.keys()) if isinstance(data, dict) else type(data))
ld = data.get("LaunchDaemons") if isinstance(data, dict) else None
if isinstance(ld, dict):
    keys = list(ld.keys())
    print("LaunchDaemons count", len(keys))
    for needle in ("CommCenter", "voicemail", "locationd", "Location"):
        hits = [k for k in keys if needle.lower() in k.lower()]
        print(f"hits[{needle}]:", hits[:20])
    print("sample keys:", keys[:30])
else:
    print("LaunchDaemons type", type(ld))

services = [
    "com.apple.voicemail.vmd",
    "com.apple.CommCenter",
    "com.apple.CommCenterMobileHelper",
    "com.apple.CommCenterRootHelper",
    "com.apple.locationd",
]

def find_entry(obj, name):
    if isinstance(obj, dict):
        if name in obj and isinstance(obj[name], dict):
            return obj[name], ("key", name)
        if obj.get("Label") == name and "Disabled" in obj or obj.get("Label") == name:
            return obj, ("label", name)
        for k, v in obj.items():
            r = find_entry(v, name)
            if r[0] is not None:
                return r
    elif isinstance(obj, list):
        for i in obj:
            r = find_entry(i, name)
            if r[0] is not None:
                return r
    return None, None

changed = []
for name in services:
    entry, how = find_entry(data, name)
    if entry is None:
        # fuzzy: any dict with Label containing name
        def fuzzy(obj):
            if isinstance(obj, dict):
                lab = obj.get("Label")
                if isinstance(lab, str) and (lab == name or name in lab or lab in name):
                    return obj
                for v in obj.values():
                    r = fuzzy(v)
                    if r is not None:
                        return r
            elif isinstance(obj, list):
                for i in obj:
                    r = fuzzy(i)
                    if r is not None:
                        return r
            return None
        entry = fuzzy(data)
        how = ("fuzzy", name)
    if entry is None:
        print("STILL MISSING", name)
        continue
    entry["Disabled"] = True
    changed.append((name, how))
    print("Disabled", name, "via", how)

# Also scan LaunchDaemons keys fuzzy
if isinstance(ld, dict):
    for k, v in ld.items():
        if not isinstance(v, dict):
            continue
        kl = k.lower()
        if any(s in kl for s in ("commcenter", "voicemail", "locationd")):
            v["Disabled"] = True
            if k not in [c[0] for c in changed]:
                changed.append((k, ("ld-key", k)))
                print("Disabled via ld-key", k)

print("CHANGED:", changed)
with open(p, "wb") as f:
    plistlib.dump(data, f, fmt=plistlib.FMT_BINARY)
# verify reload
with open(p, "rb") as f:
    data2 = plistlib.load(f)
for name in services:
    e, _ = find_entry(data2, name)
    print("verify", name, "Disabled=" + str(e.get("Disabled") if e else None))
print("LAUNCHD_OK")
PY

sync
umount /mnt/ios
sync
echo SUCCESS > /mnt/payload/SUCCESS
EOS
chmod +x "$PAYLOAD_DIR/do-launchd.sh"

PAYLOAD_IMG="$WORK/payload-launchd.img"
rm -f "$PAYLOAD_IMG"
dd if=/dev/zero of="$PAYLOAD_IMG" bs=1M count=32 status=none
mkfs.vfat -n PAYLOAD "$PAYLOAD_IMG"
MTOOLS_SKIP_CHECK=1 mcopy -i "$PAYLOAD_IMG" "$PAYLOAD_DIR/do-launchd.sh" ::do-launchd.sh

cat > "$SEED_DIR/user-data" <<'EOF'
#cloud-config
password: inferno
chpasswd: { expire: False }
ssh_pwauth: True
bootcmd:
  - mkdir -p /mnt/payload
  - mount -L PAYLOAD /mnt/payload || mount /dev/vdd /mnt/payload
runcmd:
  - [ bash, -lc, "chmod +x /mnt/payload/do-launchd.sh; bash /mnt/payload/do-launchd.sh; sync; poweroff -f" ]
EOF
cat > "$SEED_DIR/meta-data" <<EOF
instance-id: launchd-fix-$(date +%s)
local-hostname: launchd-fix
EOF
SEED_ISO="$WORK/seed-launchd.iso"
cloud-localds "$SEED_ISO" "$SEED_DIR/user-data" "$SEED_DIR/meta-data"

rm -f "$RUNTIME_IMG"
qemu-img convert -O qcow2 "$CLOUD_IMG" "$RUNTIME_IMG"
qemu-img resize "$RUNTIME_IMG" 12G

echo "=== Launchd-only patch VM ==="
qemu-system-x86_64 \
  -machine q35,accel=kvm:tcg -cpu max -m 4096 -smp 4 \
  -drive file="$RUNTIME_IMG",if=none,id=os,format=qcow2,cache=writeback \
  -device virtio-blk-pci,drive=os,bootindex=1 \
  -drive file="$ROOT_WORK",if=none,id=iosroot,format=raw,cache=unsafe \
  -device virtio-blk-pci,drive=iosroot,physical_block_size=4096,logical_block_size=4096 \
  -drive file="$SEED_ISO",if=none,id=cidata,format=raw,readonly=on \
  -device virtio-blk-pci,drive=cidata \
  -drive file="$PAYLOAD_IMG",if=none,id=payload,format=raw,cache=unsafe \
  -device virtio-blk-pci,drive=payload \
  -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
  -nographic -serial mon:stdio

MTOOLS_SKIP_CHECK=1 mcopy -n -i "$PAYLOAD_IMG" ::SUCCESS "$PAYLOAD_DIR/SUCCESS" 2>/dev/null || true
MTOOLS_SKIP_CHECK=1 mcopy -n -i "$PAYLOAD_IMG" ::do-launchd.log "$PAYLOAD_DIR/do-launchd.log" 2>/dev/null || true

if [[ ! -f "$PAYLOAD_DIR/SUCCESS" ]]; then
  echo "LAUNCHD PATCH FAILED"
  tail -100 "$PAYLOAD_DIR/do-launchd.log" 2>/dev/null || true
  exit 1
fi

echo "=== Installing patched root to AQEMU path ==="
rm -f "$ROOT_IMG"
cp -f "$ROOT_WORK" "$ROOT_IMG"
sync
ls -lh "$ROOT_IMG"
mkdir -p /mnt/d/aqemu-backups
cp -f "$ROOT_WORK" /mnt/d/aqemu-backups/iOS_ARM64_root.fs-patched || true
echo ALL_DONE
