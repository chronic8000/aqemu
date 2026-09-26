#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: reims (qemu-system-reims)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_reims.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_reims.sh
#   ./scripts/qemu_targets/build_qemu_reims.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "reims" "$@"
