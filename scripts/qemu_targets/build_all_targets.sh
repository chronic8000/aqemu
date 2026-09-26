#!/usr/bin/env bash
# ==============================================================================
# Build ALL 31 QEMU Targets Individually (AQEMU)
# ==============================================================================
# Usage:
#   ./scripts/qemu_targets/build_all_targets.sh [PREFIX]
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PREFIX="${1:-$(cd "${SCRIPT_DIR}/../.." && pwd)/third_party/qemu-install}"

echo "======================================================================"
echo "  Starting Build of All 31 Individual QEMU Targets"
echo "  Install Prefix: ${PREFIX}"
echo "======================================================================"

TARGETS=(
  x86_64
  i386
  aarch64
  arm
  ppc64
  ppc
  riscv64
  riscv32
  mips64el
  mips64
  mipsel
  mips
  s390x
  sparc64
  sparc
  m68k
  loongarch64
  alpha
  hppa
  sh4
  sh4eb
  microblaze
  microblazeel
  or1k
  rx
  avr
  tricore
  xtensa
  xtensaeb
  applesoc
  reims
)

SUCCESS=0
FAILED=0
FAILED_LIST=()

for target in "${TARGETS[@]}"; do
  echo ""
  echo ">>> [Target: ${target}] Building..."
  if bash "${SCRIPT_DIR}/build_qemu_${target}.sh" "${PREFIX}"; then
    echo ">>> [Target: ${target}] SUCCESS"
    SUCCESS=$((SUCCESS + 1))
  else
    echo ">>> [Target: ${target}] FAILED"
    FAILED=$((FAILED + 1))
    FAILED_LIST+=("${target}")
  fi
done

echo ""
echo "======================================================================"
echo "  Build Summary: ${SUCCESS} succeeded, ${FAILED} failed"
if [[ ${FAILED} -gt 0 ]]; then
  echo "  Failed targets: ${FAILED_LIST[*]}"
fi
echo "======================================================================"
