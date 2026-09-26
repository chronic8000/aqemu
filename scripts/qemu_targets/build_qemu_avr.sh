#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: avr (qemu-system-avr)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_avr.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_avr.sh
#   ./scripts/qemu_targets/build_qemu_avr.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "avr" "$@"
