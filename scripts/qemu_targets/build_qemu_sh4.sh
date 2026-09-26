#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: sh4 (qemu-system-sh4)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_sh4.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_sh4.sh
#   ./scripts/qemu_targets/build_qemu_sh4.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "sh4" "$@"
