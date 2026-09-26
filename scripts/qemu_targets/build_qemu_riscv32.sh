#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: riscv32 (qemu-system-riscv32)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_riscv32.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_riscv32.sh
#   ./scripts/qemu_targets/build_qemu_riscv32.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "riscv32" "$@"
