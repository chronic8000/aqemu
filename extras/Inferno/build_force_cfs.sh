#!/bin/bash
# Install aqemu-idr-wrap.sh. Do NOT LD_PRELOAD aqemu_force_cfs.so:
# Inferno #219 ramrod prints the same three option keys we see, but CFS=true.
# The preload rewired libimobiledevice plist_dict_set_item and never logged
# StartRestore ENTER — it can flip CFS without any diagnostic line.
set -eu
OUT=${2:-/usr/local/lib/aqemu_force_cfs.so}
WRAP=/tmp/aqemu-idr-wrap.sh
# Ubuntu fs.protected_regular: root cannot overwrite another user's file in /tmp.
rm -f "$WRAP"
cat > "$WRAP" << 'WEOF'
#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
unset LD_PRELOAD
echo "AQEMU wrap: vanilla idevicerestore (no LD_PRELOAD)"
echo "AQEMU wrap argv: $*"
exec /usr/local/bin/idevicerestore "$@"
WEOF
chmod 755 "$WRAP"
echo "AQEMU: wrap will NOT LD_PRELOAD aqemu_force_cfs.so"
if [ -s "$OUT" ]; then
  echo "AQEMU: leaving $OUT in place (not preloaded)"
fi
echo "AQEMU: wrap installed (preload disabled)"
