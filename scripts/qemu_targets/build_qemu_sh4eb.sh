#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: sh4eb (qemu-system-sh4eb)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_sh4eb.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_sh4eb.sh
#   ./scripts/qemu_targets/build_qemu_sh4eb.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "sh4eb" "$@"
