#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: hppa (qemu-system-hppa)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_hppa.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_hppa.sh
#   ./scripts/qemu_targets/build_qemu_hppa.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "hppa" "$@"
