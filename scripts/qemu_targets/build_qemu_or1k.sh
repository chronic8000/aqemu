#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: or1k (qemu-system-or1k)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_or1k.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_or1k.sh
#   ./scripts/qemu_targets/build_qemu_or1k.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "or1k" "$@"
