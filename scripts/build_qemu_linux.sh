#!/usr/bin/env bash
# ==============================================================================
# Universal QEMU build launcher for Linux hosts (Pi 5, ARM64, and x86_64)
# ==============================================================================
# Usage:
#   scripts/build_qemu_linux.sh [TARGET] [PREFIX]
# ==============================================================================
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

ARCH="$(uname -m 2>/dev/null || echo unknown)"
if [[ -f /proc/device-tree/model ]] && grep -qi "Raspberry Pi 5" /proc/device-tree/model 2>/dev/null; then
  echo "--> Routing to dedicated Raspberry Pi 5 QEMU build script..."
  exec bash "${ROOT}/scripts/build_qemu_pi5.sh" "$@"
elif [[ "${ARCH}" == "aarch64" || "${ARCH}" == "arm64" ]]; then
  echo "--> Routing to dedicated Linux ARM64 QEMU build script..."
  exec bash "${ROOT}/scripts/build_qemu_linux_arm64.sh" "$@"
else
  echo "--> Routing to dedicated Linux x86_64 QEMU build script..."
  exec bash "${ROOT}/scripts/build_qemu_linux_x86_64.sh" "$@"
fi
