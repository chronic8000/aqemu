#!/usr/bin/env bash
# ==============================================================================
# Build QEMU Target: microblaze (qemu-system-microblaze)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_qemu_microblaze.sh [PREFIX]
#
# Examples:
#   ./scripts/qemu_targets/build_qemu_microblaze.sh
#   ./scripts/qemu_targets/build_qemu_microblaze.sh /opt/qemu
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
exec bash "${SCRIPT_DIR}/_build_target_common.sh" "microblaze" "$@"
