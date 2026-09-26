#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: tricore (qemu-system-tricore)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_tricore.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_tricore.sh
#   ./scripts/qemu_targets/build_qemu_tricore.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "tricore" "$@"
