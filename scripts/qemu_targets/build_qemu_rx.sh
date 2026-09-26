#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: rx (qemu-system-rx)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_rx.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_rx.sh
#   ./scripts/qemu_targets/build_qemu_rx.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "rx" "$@"
