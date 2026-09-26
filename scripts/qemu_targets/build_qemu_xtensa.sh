#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: xtensa (qemu-system-xtensa)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_xtensa.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_xtensa.sh
#   ./scripts/qemu_targets/build_qemu_xtensa.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "xtensa" "$@"
