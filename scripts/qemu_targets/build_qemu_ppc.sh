#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: ppc (qemu-system-ppc)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_ppc.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_ppc.sh
#   ./scripts/qemu_targets/build_qemu_ppc.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "ppc" "$@"
