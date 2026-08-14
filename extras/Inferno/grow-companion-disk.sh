#!/bin/bash
set -e
DISK="/mnt/c/Users/chron/AQEMU_VM/iPhone_IPSW_Restore_Companion/companion.qcow2"
BIN=/usr/local/bin/qemu-system-x86_64-inferno
PIDF=/tmp/aqemu-inferno-companion.pid
export SSHPASS=ipsw

pkill -f qemu-system-x86_64-inferno 2>/dev/null || true
rm -f "$PIDF"

qemu-img info "$DISK" | head -3

setsid "$BIN" -L /usr/share/qemu -bios /usr/share/seabios/bios-256k.bin -nic none \
  -drive "file=$DISK,if=ide,format=qcow2,index=0,media=disk" \
  -netdev user,id=net0,hostfwd=tcp::32222-:22 \
  -device virtio-net-pci,netdev=net0,romfile=,rombar=0 \
  -m 2048M -M q35 -accel tcg -nographic -monitor none \
  </dev/null >/tmp/aqemu-grow.log 2>&1 &
echo $! >"$PIDF"

ok=0
for i in $(seq 1 80); do
  if sshpass -e ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
     -o ConnectTimeout=4 -p 32222 bob@127.0.0.1 "echo SSH_OK" 2>/dev/null; then
    ok=1
    break
  fi
  sleep 3
done

if [ "$ok" != 1 ]; then
  echo "SSH_FAIL"
  tail -20 /tmp/aqemu-grow.log
  exit 1
fi

ssh_do() { sshpass -e ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p 32222 bob@127.0.0.1 "$@"; }

echo "=== before ==="
ssh_do "df -h /; lsblk"

ssh_do 'sudo -n DEBIAN_FRONTEND=noninteractive apt-get install -y cloud-guest-utils 2>/dev/null || true
  sudo -n growpart /dev/sda 3 2>/dev/null || sudo -n growpart /dev/sda 2
  sudo -n pvresize /dev/sda3 2>/dev/null || sudo -n pvresize /dev/sda2
  sudo -n lvextend -l +100%FREE /dev/mapper/ubuntu--vg-ubuntu--lv
  sudo -n resize2fs /dev/mapper/ubuntu--vg-ubuntu--lv
  echo "=== after ==="
  df -h /'

pid=$(cat "$PIDF")
kill -TERM "$pid" 2>/dev/null || true
sleep 2
kill -KILL "$pid" 2>/dev/null || true
rm -f "$PIDF"
echo GROW_DONE
