#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: arm (qemu-system-arm)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_arm.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_arm.sh
#   ./scripts/qemu_targets/build_qemu_arm.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "arm" "$@"
