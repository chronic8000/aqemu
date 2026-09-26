#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: microblazeel (qemu-system-microblazeel)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_microblazeel.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_microblazeel.sh
#   ./scripts/qemu_targets/build_qemu_microblazeel.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "microblazeel" "$@"
