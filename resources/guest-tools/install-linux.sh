#!/bin/sh
# Install qemu-guest-agent and start the AQEMU clipboard helper.
set -e
cd "$(dirname "$0")"
if command -v apt-get >/dev/null 2>&1; then
	sudo apt-get update
	sudo apt-get install -y qemu-guest-agent
elif command -v dnf >/dev/null 2>&1; then
	sudo dnf install -y qemu-guest-agent
elif command -v pacman >/dev/null 2>&1; then
	sudo pacman -Sy --noconfirm qemu-guest-agent
else
	echo "Install qemu-guest-agent with your package manager, then run this script again."
	exit 1
fi
if command -v systemctl >/dev/null 2>&1; then
	sudo systemctl enable --now qemu-guest-agent || true
fi
if command -v python3 >/dev/null 2>&1; then
	python3 ./aqemu-guest.py >/tmp/aqemu-guest.log 2>&1 &
	echo "Clipboard helper started. Log: /tmp/aqemu-guest.log"
else
	echo "python3 is not installed, so clipboard copy back to the host is not running."
fi
echo "qemu-guest-agent is installed. Use Copy to guest path and Copy from guest in AQEMU."
