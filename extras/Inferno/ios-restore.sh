#!/bin/bash
# idevicerestore on this host (Linux or WSL). ios-host.sh has already
# started the USB bridge and usbmuxd MODE=1 and put the matching arch's
# tools on PATH. IPSW and TICKET are host paths (a /mnt/c path on WSL).
set -u

IPSW=${IPSW:-}
TICKET=${TICKET:-}

if [ -z "$IPSW" ] || [ ! -f "$IPSW" ]; then
	echo "AQEMU: IPSW not found: ${IPSW:-<empty>}"
	exit 1
fi

if ! command -v idevicerestore >/dev/null 2>&1; then
	echo "AQEMU: idevicerestore is not on PATH."
	exit 127
fi

if ! strings "$(command -v idevicerestore)" 2>/dev/null | grep -Fq "INFO: model is"; then
	echo "AQEMU: this idevicerestore build is missing the N104DEV to AP patch."
	exit 4
fi

echo "AQEMU: using $(command -v idevicerestore)"
idevicerestore --version 2>&1 || true
echo "AQEMU: IPSW $IPSW"
echo "AQEMU: Power On iOS with USB remote ipv4 127.0.0.1:8030."
echo "AQEMU: iOS only waits about 120 seconds once the Apple logo appears."

DEV=
for i in $(seq 1 70); do
	LS=$(lsusb 2>/dev/null || true)
	if echo "$LS" | grep -qiE 'Apple|05ac:'; then
		echo "AQEMU: Apple USB device seen (try $i)"
		echo "$LS" | grep -iE 'Apple|05ac:' || true
		sleep 3
		DEV=$(idevice_id -l 2>/dev/null | head -1 || true)
		if [ -z "$DEV" ]; then
			echo "AQEMU: idevice_id is still empty; starting restore anyway."
			DEV=00008030-1122334455667788
		fi
		break
	fi
	echo "AQEMU: ($i/70) waiting for the iPhone on USB"
	sleep 2
done

if [ -z "$DEV" ]; then
	echo "AQEMU: no Apple USB device appeared."
	echo "AQEMU: Power On iOS, then run Restore again within the Apple-logo window."
	lsusb 2>/dev/null || true
	idevice_id -l 2>&1 || true
	exit 2
fi

echo "AQEMU: device $DEV"
ECID_HEX=${DEV##*-}
ECID_ARG=
if [ -n "$ECID_HEX" ] && [ "$ECID_HEX" != "$DEV" ]; then
	ECID_ARG="-i 0x$ECID_HEX"
else
	ECID_ARG="-i 0x1122334455667788"
fi
TICKET_ARG=
if [ -n "$TICKET" ] && [ -s "$TICKET" ]; then
	TICKET_ARG="-T $TICKET"
	echo "AQEMU: ticket $TICKET"
else
	echo "AQEMU: no restore ticket file. idevicerestore will not get -T."
fi

echo "AQEMU: starting idevicerestore"
set +e
idevicerestore --debug -e --erase --restore-mode -y $ECID_ARG $TICKET_ARG "$IPSW"
ec=$?
set -e
if [ "$ec" != 0 ]; then
	echo "AQEMU: idevicerestore exited $ec"
	exit "$ec"
fi
echo "AQEMU: idevicerestore finished."
