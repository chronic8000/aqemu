#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: mips64el (qemu-system-mips64el)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_mips64el.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_mips64el.sh
#   ./scripts/qemu_targets/build_qemu_mips64el.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "mips64el" "$@"
