#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: mipsel (qemu-system-mipsel)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_mipsel.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_mipsel.sh
#   ./scripts/qemu_targets/build_qemu_mipsel.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "mipsel" "$@"
