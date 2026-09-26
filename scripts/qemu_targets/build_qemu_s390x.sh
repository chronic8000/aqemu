#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: s390x (qemu-system-s390x)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_s390x.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_s390x.sh
#   ./scripts/qemu_targets/build_qemu_s390x.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "s390x" "$@"
