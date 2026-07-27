#!/usr/bin/env bash
# Build bundled QEMU targets for Linux / Raspberry Pi (AQEMU)
# Usage: scripts/build_qemu_linux.sh [PREFIX]
#
# Builds EVERY softmmu target with the full AQEMU feature set.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
PREFIX="${1:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build"

if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Run: git submodule update --init --depth 1 third_party/qemu"
  exit 1
fi

export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"

mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

# shellcheck source=qemu_softmmu_targets.sh
source "${ROOT}/scripts/qemu_softmmu_targets.sh"
TARGETS="$(qemu_softmmu_target_list "${QEMU_SRC}")"
echo "Building ALL softmmu targets: ${TARGETS}"

# shellcheck source=qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --target-list="${TARGETS}" \
  "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"

ninja -C "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"
ninja -C "${BUILD_DIR}" install

echo "Installed QEMU bundle to ${PREFIX}"
ls -la "${PREFIX}/bin"/qemu-system-* "${PREFIX}/bin"/qemu-img 2>/dev/null || true
aqemu_qemu_verify_install "${PREFIX}"
