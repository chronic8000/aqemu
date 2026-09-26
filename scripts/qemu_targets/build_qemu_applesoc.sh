#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: applesoc (qemu-system-applesoc)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_applesoc.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_applesoc.sh
#   ./scripts/qemu_targets/build_qemu_applesoc.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "applesoc" "$@"
