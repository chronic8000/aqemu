#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: sparc (qemu-system-sparc)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_sparc.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_sparc.sh
#   ./scripts/qemu_targets/build_qemu_sparc.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "sparc" "$@"
