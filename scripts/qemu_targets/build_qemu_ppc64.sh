#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: ppc64 (qemu-system-ppc64)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_ppc64.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_ppc64.sh
#   ./scripts/qemu_targets/build_qemu_ppc64.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "ppc64" "$@"
