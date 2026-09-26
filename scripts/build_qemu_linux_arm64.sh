#!/usr/bin/env bash
# ==============================================================================
# Build QEMU for Linux ARM64 (AArch64)
# ==============================================================================
# Usage:
#   ./scripts/build_qemu_linux_arm64.sh [TARGET] [PREFIX]
#
# Examples:
#   ./scripts/build_qemu_linux_arm64.sh                 # Builds ALL softmmu targets
#   ./scripts/build_qemu_linux_arm64.sh aarch64         # Builds qemu-system-aarch64 only
#   ./scripts/build_qemu_linux_arm64.sh x86_64          # Builds qemu-system-x86_64 only
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
TARGET_ARG="${1:-all}"
PREFIX="${2:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build-linux-arm64"

echo "=== Building QEMU for Linux ARM64 (Target: ${TARGET_ARG}) ==="

if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Running: git submodule update --init --depth 1 third_party/qemu"
  git -C "${ROOT}" submodule update --init --depth 1 third_party/qemu
fi

export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
export CFLAGS="${CFLAGS:-} -O3"
export CXXFLAGS="${CXXFLAGS:-} -O3"

mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

# Feature flags
# shellcheck source=qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags

# shellcheck source=qemu_special_targets.sh
source "${ROOT}/scripts/qemu_special_targets.sh"

JOBS="$(nproc 2>/dev/null || echo 4)"

# Check if an individual special target was requested (applesoc / reims)
if aqemu_is_special_target "${TARGET_ARG}"; then
  echo "Special target requested: ${TARGET_ARG}"
  aqemu_build_special_target "${TARGET_ARG}" "${PREFIX}" "${JOBS}"
  echo "Special target '${TARGET_ARG}' built successfully!"
  exit 0
fi

# Determine target list for upstream QEMU
# shellcheck source=qemu_softmmu_targets.sh
source "${ROOT}/scripts/qemu_softmmu_targets.sh"

if [[ "${TARGET_ARG}" == "all" || "${TARGET_ARG}" == "ALL" ]]; then
  TARGETS="$(qemu_softmmu_target_list "${QEMU_SRC}")"
  VERIFY_TARGET=""
else
  TARGET_CLEAN="$(echo "${TARGET_ARG}" | sed 's/-softmmu$//')"
  TARGETS="${TARGET_CLEAN}-softmmu"
  VERIFY_TARGET="${TARGET_CLEAN}"
fi

echo "Configuring targets: ${TARGETS}"

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --target-list="${TARGETS}" \
  "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"

echo "Building with ${JOBS} parallel jobs..."
ninja -C "${BUILD_DIR}" -j"${JOBS}"
ninja -C "${BUILD_DIR}" install

# Build special targets when building 'all'
if [[ "${TARGET_ARG}" == "all" || "${TARGET_ARG}" == "ALL" ]]; then
  echo "=== Building special target: applesoc (ChefKiss Inferno) ==="
  aqemu_build_applesoc "${PREFIX}" "${JOBS}"

  echo "=== Building special target: reims (steelbrain Reims vGPU) ==="
  aqemu_build_reims "${PREFIX}" "${JOBS}"
fi

echo "Installed QEMU to ${PREFIX}"
ls -la "${PREFIX}/bin"/qemu-system-* "${PREFIX}/bin"/qemu-img 2>/dev/null || true

aqemu_qemu_verify_install "${PREFIX}" "${VERIFY_TARGET}"
echo "Linux ARM64 QEMU build completed successfully!"
