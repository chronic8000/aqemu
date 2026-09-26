#!/usr/bin/env bash
# ==============================================================================
# Build QEMU for Windows x86_64 (MSYS2 UCRT64 / MinGW64)
# ==============================================================================
# Usage:
#   ./scripts/build_qemu_win_x86_64.sh [TARGET] [PREFIX]
#
# Examples:
#   ./scripts/build_qemu_win_x86_64.sh                 # Builds ALL softmmu targets
#   ./scripts/build_qemu_win_x86_64.sh x86_64          # Builds qemu-system-x86_64 only
#   ./scripts/build_qemu_win_x86_64.sh aarch64         # Builds qemu-system-aarch64 only
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
TARGET_ARG="${1:-all}"
PREFIX="${2:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build-win-x86_64"

# Probe MSYS environment (prefer UCRT64, fallback to MINGW64)
if [[ -d "/ucrt64" ]]; then
  export MSYSTEM=UCRT64
  export PATH="/ucrt64/bin:/usr/bin:${PATH}"
  export PKG_CONFIG="C:/msys64/ucrt64/bin/pkg-config.exe"
  export PKG_CONFIG_PATH="C:/msys64/ucrt64/lib/pkgconfig"
  WIN_BIN="C:/msys64/ucrt64/bin"
else
  export MSYSTEM=MINGW64
  export PATH="/mingw64/bin:/usr/bin:${PATH}"
  export PKG_CONFIG="C:/msys64/mingw64/bin/pkg-config.exe"
  export PKG_CONFIG_PATH="C:/msys64/mingw64/lib/pkgconfig"
  WIN_BIN="C:/msys64/mingw64/bin"
fi
unset PKG_CONFIG_LIBDIR || true

echo "=== Building QEMU for Windows x86_64 (${MSYSTEM} / Target: ${TARGET_ARG}) ==="
echo "Using compiler: $(which gcc 2>/dev/null || echo gcc)"
echo "Using PKG_CONFIG: ${PKG_CONFIG}"

if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Running: git submodule update --init --depth 1 third_party/qemu"
  git -C "${ROOT}" submodule update --init --depth 1 third_party/qemu
fi

# Ensure diff is on PATH for Meson
export PATH="${WIN_BIN}:C:/msys64/usr/bin:${PATH}"
if [[ -x /usr/bin/diff.exe && ! -e "${WIN_BIN}/diff.exe" ]]; then
  cp -f /usr/bin/diff.exe "${WIN_BIN}/diff.exe" 2>/dev/null || true
fi

mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

# Determine target list
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

# Feature flags
# shellcheck source=qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --cc=gcc \
  --cxx=g++ \
  --target-list="${TARGETS}" \
  "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"

JOBS="$(nproc 2>/dev/null || echo 4)"
echo "Building with ${JOBS} parallel jobs..."
ninja -C "${BUILD_DIR}" -j"${JOBS}"
ninja -C "${BUILD_DIR}" install

echo "Installed QEMU bundle to ${PREFIX}"
ls -la "${PREFIX}"/bin/qemu-system-* "${PREFIX}"/qemu-system-* "${PREFIX}"/bin/qemu-img* 2>/dev/null || true

aqemu_qemu_verify_install "${PREFIX}" "${VERIFY_TARGET}"
echo "Windows x86_64 QEMU build completed successfully!"
