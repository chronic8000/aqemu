#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: mips64 (qemu-system-mips64)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_mips64.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_mips64.sh
#   ./scripts/qemu_targets/build_qemu_mips64.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "mips64" "$@"
