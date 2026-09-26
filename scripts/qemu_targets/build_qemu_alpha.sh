#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: alpha (qemu-system-alpha)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_alpha.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_alpha.sh
#   ./scripts/qemu_targets/build_qemu_alpha.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "alpha" "$@"
