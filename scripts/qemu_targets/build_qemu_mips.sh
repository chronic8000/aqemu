#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: mips (qemu-system-mips)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_mips.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_mips.sh
#   ./scripts/qemu_targets/build_qemu_mips.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "mips" "$@"
