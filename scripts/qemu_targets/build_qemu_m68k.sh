#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: m68k (qemu-system-m68k)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_m68k.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_m68k.sh
#   ./scripts/qemu_targets/build_qemu_m68k.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "m68k" "$@"
