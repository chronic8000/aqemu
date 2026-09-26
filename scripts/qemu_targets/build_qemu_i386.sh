#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: i386 (qemu-system-i386)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_i386.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_i386.sh
#   ./scripts/qemu_targets/build_qemu_i386.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "i386" "$@"
