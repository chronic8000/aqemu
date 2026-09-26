#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: sparc64 (qemu-system-sparc64)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_sparc64.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_sparc64.sh
#   ./scripts/qemu_targets/build_qemu_sparc64.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "sparc64" "$@"
