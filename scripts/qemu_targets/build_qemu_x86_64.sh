#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: x86_64 (qemu-system-x86_64)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_x86_64.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_x86_64.sh
#   ./scripts/qemu_targets/build_qemu_x86_64.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "x86_64" "$@"
