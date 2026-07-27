#!/usr/bin/env bash
set -euo pipefail
export MSYSTEM=MINGW64
export PATH="/mingw64/bin:/usr/bin:$PATH"
cd /c/Users/chron/CURSOR-PROJECTS/aqemu
echo "START $(date)" > third_party/qemu-build-all.log
bash scripts/build_qemu_windows_msys.sh >> third_party/qemu-build-all.log 2>&1
echo "END exit=$? $(date)" >> third_party/qemu-build-all.log
