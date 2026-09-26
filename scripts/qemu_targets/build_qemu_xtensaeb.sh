#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: xtensaeb (qemu-system-xtensaeb)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_xtensaeb.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_xtensaeb.sh
#   ./scripts/qemu_targets/build_qemu_xtensaeb.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "xtensaeb" "$@"
