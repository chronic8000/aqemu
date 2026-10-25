#!/usr/bin/env bash
set -euo pipefail
export HOME=/home/chron
export AQEMU_ROOT=/mnt/c/Users/chron/CURSOR-PROJECTS/AQEMU/aqemu
python3 - <<'PY'
from pathlib import Path
src = Path("/mnt/c/Users/chron/CURSOR-PROJECTS/AQEMU/aqemu/scripts/build_wsl_qemu.sh")
text = src.read_bytes().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
Path("/tmp/build_wsl_qemu.sh").write_bytes(text)
PY
export AQEMU_WSL_REIMS_RECONFIG=1
export AQEMU_WSL_ONLY=reims
exec bash /tmp/build_wsl_qemu.sh
