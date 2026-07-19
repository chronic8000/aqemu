#!/usr/bin/env bash
# Windows QEMU bundle build for AQEMU.
# Default: pure MSYS2 (same gcc + glib/spice). Requires fix_msys2_gcc_admin.ps1 once.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
exec bash "${ROOT}/scripts/build_qemu_windows_msys.sh" "$@"
