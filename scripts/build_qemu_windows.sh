#!/usr/bin/env bash
# ==============================================================================
# Universal Windows QEMU bundle build launcher (MSYS2)
# ==============================================================================
# Routes to Windows on ARM (CLANGARM64) or Windows x86_64 (UCRT64/MINGW64)
# ==============================================================================
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

if [[ "${MSYSTEM:-}" == "CLANGARM64" || "${PROCESSOR_ARCHITECTURE:-}" == "ARM64" ]]; then
  exec bash "${ROOT}/scripts/build_qemu_win_arm64.sh" "$@"
else
  exec bash "${ROOT}/scripts/build_qemu_win_x86_64.sh" "$@"
fi
