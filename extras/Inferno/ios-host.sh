#!/bin/bash
# Run iPhone tools on this machine (Linux) or inside WSL (Windows).
# Picks host-tools/aarch64 or host-tools/x86_64 from uname -m.
# Starts the USB bridge and usbmuxd, then runs the given command.
#
#   ios-host.sh ensure
#   ios-host.sh 1 -- ideviceinfo -s
#   ios-host.sh 3 -- bash -c '...'
# Mode 1 is lockdown. Mode 3 is CDC ethernet (guest internet). Mode 0 leaves usbmuxd alone.
set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case "$(uname -m)" in
	aarch64|arm64) ARCH=aarch64 ;;
	x86_64|amd64) ARCH=x86_64 ;;
	*)
		echo "AQEMU: this host is $(uname -m). Host tools are built for aarch64 and x86_64."
		exit 1
		;;
esac

PREFIX="$ROOT/host-tools/$ARCH"
export PATH="$PREFIX/bin:$PREFIX/sbin:$PATH"
export LD_LIBRARY_PATH="$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

if [ "$(id -u)" != 0 ]; then
	if command -v sudo >/dev/null 2>&1 && sudo -n true 2>/dev/null; then
		exec sudo -n --preserve-env=PATH,LD_LIBRARY_PATH,IPSW,TICKET -- bash "$0" "$@"
	fi
	echo "AQEMU: ios-host.sh needs root (vhci-hcd and usbmuxd)."
	echo "On Windows, AQEMU runs it with: wsl -u root"
	exit 1
fi

log() { echo "AQEMU: $*"; }

start_bridge() {
	if [ ! -x "$PREFIX/bin/aqemu-usb-bridge" ]; then
		echo "AQEMU: missing $PREFIX/bin/aqemu-usb-bridge"
		echo "AQEMU: the $ARCH USB bridge has not been built into this app yet."
		exit 127
	fi
	modprobe usbip-core 2>/dev/null || true
	modprobe vhci-hcd 2>/dev/null || true
	if [ ! -e /sys/devices/platform/vhci_hcd.0/attach ]; then
		echo "AQEMU: vhci-hcd did not create /sys/devices/platform/vhci_hcd.0"
		echo "AQEMU: this kernel has no USB/IP virtual host controller."
		exit 1
	fi
	local pid comm
	pid=$(ss -H -ltnp 'sport = :8030' 2>/dev/null | sed -n 's/.*pid=\([0-9][0-9]*\).*/\1/p' | head -1)
	if [ -n "${pid:-}" ]; then
		comm=$(ps -p "$pid" -o comm= 2>/dev/null | tr -d ' ' || true)
		if [ "$comm" = "aqemu-usb-bridge" ]; then
			log "USB bridge already listening on 127.0.0.1:8030"
			return 0
		fi
		log "Port 8030 is held by $comm (pid $pid). Stopping it so WSL can own the iPhone USB link."
		kill -TERM "$pid" 2>/dev/null || true
		sleep 1
		kill -KILL "$pid" 2>/dev/null || true
		sleep 0.5
	fi
	mkdir -p /var/log
	if ss -H -ltn 'sport = :8030' 2>/dev/null | grep -q .; then
		echo "AQEMU: 127.0.0.1:8030 is still busy after stopping the old listener."
		ss -ltnp 'sport = :8030' 2>/dev/null || true
		exit 1
	fi
	nohup "$PREFIX/bin/aqemu-usb-bridge" >>/var/log/aqemu-usb-bridge.log 2>&1 &
	local i
	for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
		if ss -H -ltn 'sport = :8030' 2>/dev/null | grep -q .; then
			log "USB bridge listening on 127.0.0.1:8030 ($ARCH)"
			return 0
		fi
		sleep 0.25
	done
	echo "AQEMU: USB bridge did not start. Last log lines:"
	tail -n 30 /var/log/aqemu-usb-bridge.log 2>/dev/null || true
	exit 1
}

start_mux() {
	local mode="$1"
	if [ "$mode" = "0" ]; then
		return 0
	fi
	local mux=""
	if [ -x "$PREFIX/sbin/usbmuxd" ]; then
		mux="$PREFIX/sbin/usbmuxd"
	elif [ -x "$PREFIX/bin/usbmuxd" ]; then
		mux="$PREFIX/bin/usbmuxd"
	fi
	if [ -z "$mux" ]; then
		echo "AQEMU: usbmuxd is not in $PREFIX"
		exit 127
	fi
	if [ -f /run/aqemu-usbmux.mode ] && [ "$(cat /run/aqemu-usbmux.mode 2>/dev/null)" = "$mode" ] \
		&& pgrep -x usbmuxd >/dev/null 2>&1; then
		log "usbmuxd already running in MODE=$mode"
		return 0
	fi
	killall usbmuxd 2>/dev/null || true
	sleep 0.3
	rm -f /var/run/usbmuxd /run/usbmuxd /var/run/usbmuxd.pid /run/usbmuxd.pid
	# -u is a user name on some builds and a flag on others. -U sets the socket owner.
	USBMUXD_DEFAULT_DEVICE_MODE="$mode" "$mux" -U root >/var/log/aqemu-usbmuxd.log 2>&1 || \
		USBMUXD_DEFAULT_DEVICE_MODE="$mode" "$mux" >/var/log/aqemu-usbmuxd.log 2>&1 || true
	sleep 0.5
	if ! pgrep -x usbmuxd >/dev/null 2>&1; then
		echo "AQEMU: usbmuxd did not stay running. Log:"
		cat /var/log/aqemu-usbmuxd.log 2>/dev/null || true
		exit 1
	fi
	mkdir -p /run
	echo "$mode" >/run/aqemu-usbmux.mode
	chmod 666 /var/run/usbmuxd /run/usbmuxd 2>/dev/null || true
	log "usbmuxd MODE=$mode"
}

if [ "${1:-}" = "ensure" ]; then
	start_bridge
	exit 0
fi

MODE="${1:-1}"
shift || true
if [ "${1:-}" = "--" ]; then
	shift
fi
start_bridge
if [ ! -x "$PREFIX/bin/idevice_id" ]; then
	echo "AQEMU: missing $PREFIX/bin/idevice_id"
	echo "AQEMU: $ARCH libimobiledevice tools are not installed in this app yet."
	exit 127
fi
if ldd "$PREFIX/bin/idevice_id" 2>/dev/null | grep -q 'not found'; then
	log "Some libraries are missing for idevice_id; installing USB/SSL packages."
	if command -v apt-get >/dev/null 2>&1; then
		export DEBIAN_FRONTEND=noninteractive
		apt-get update -qq || true
		apt-get install -y libusb-1.0-0 libssl3 libcurl4 zlib1g libzip4 libreadline8 2>/dev/null \
			|| apt-get install -y libusb-1.0-0 libssl3 libcurl4 zlib1g libzip5 libreadline8t64 \
			|| true
	fi
fi
start_mux "$MODE"
if [ "$#" -eq 0 ]; then
	log "USB host is up ($ARCH). No command given."
	exit 0
fi
exec "$@"
