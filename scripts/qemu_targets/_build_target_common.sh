#!/usr/bin/env bash
# ==============================================================================
# Common Backend for Building Individual QEMU Targets (AQEMU)
# ==============================================================================
# Usage:
#   _build_target_common.sh <TARGET> [PREFIX]
# ==============================================================================
set -euo pipefail

TARGET="${1:?Target architecture required (e.g. x86_64, aarch64, applesoc, reims)}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PREFIX="${2:-${ROOT}/third_party/qemu-install}"
BUILD_BASE="${ROOT}/third_party"

mkdir -p "${PREFIX}/bin" "${PREFIX}/share"

echo "======================================================================"
echo "  Building QEMU Target: ${TARGET}"
echo "  Install Prefix:       ${PREFIX}"
echo "======================================================================"

# ------------------------------------------------------------------------------
# 1. Host Architecture & Toolchain Detection
# ------------------------------------------------------------------------------
OS="$(uname -s 2>/dev/null || echo unknown)"
ARCH="$(uname -m 2>/dev/null || echo unknown)"

case "${OS}" in
  MINGW*|MSYS*|CYGWIN*|Windows_NT)
    IS_WINDOWS=true
    if [[ "${MSYSTEM:-}" == "CLANGARM64" || "${PROCESSOR_ARCHITECTURE:-}" == "ARM64" ]]; then
      export MSYSTEM=CLANGARM64
      export PATH="/clangarm64/bin:/usr/bin:${PATH}"
      export CC="clang"
      export CXX="clang++"
      export PKG_CONFIG="C:/msys64/clangarm64/bin/pkg-config.exe"
      export PKG_CONFIG_PATH="C:/msys64/clangarm64/lib/pkgconfig"
      export PATH="C:/msys64/clangarm64/bin:C:/msys64/usr/bin:${PATH}"
    elif [[ -d "/ucrt64" ]]; then
      export MSYSTEM=UCRT64
      export PATH="/ucrt64/bin:/usr/bin:${PATH}"
      export CC="gcc"
      export CXX="g++"
      export PKG_CONFIG="C:/msys64/ucrt64/bin/pkg-config.exe"
      export PKG_CONFIG_PATH="C:/msys64/ucrt64/lib/pkgconfig"
      export PATH="C:/msys64/ucrt64/bin:C:/msys64/usr/bin:${PATH}"
    else
      export MSYSTEM=MINGW64
      export PATH="/mingw64/bin:/usr/bin:${PATH}"
      export CC="gcc"
      export CXX="g++"
      export PKG_CONFIG="C:/msys64/mingw64/bin/pkg-config.exe"
      export PKG_CONFIG_PATH="C:/msys64/mingw64/lib/pkgconfig"
      export PATH="C:/msys64/mingw64/bin:C:/msys64/usr/bin:${PATH}"
    fi
    unset PKG_CONFIG_LIBDIR || true
    ;;
  *)
    IS_WINDOWS=false
    export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
    # Raspberry Pi 5 optimization check
    if [[ -f /proc/device-tree/model ]] && grep -qi "Raspberry Pi 5" /proc/device-tree/model 2>/dev/null; then
      echo "Applying Raspberry Pi 5 Cortex-A76 tuning & 64KB ELF segment alignment..."
      export CFLAGS="${CFLAGS:-} -mcpu=cortex-a76 -mtune=cortex-a76 -O3"
      export CXXFLAGS="${CXXFLAGS:-} -mcpu=cortex-a76 -mtune=cortex-a76 -O3"
      export LDFLAGS="${LDFLAGS:-} -Wl,-z,max-page-size=65536"
    else
      export CFLAGS="${CFLAGS:-} -O3"
      export CXXFLAGS="${CXXFLAGS:-} -O3"
    fi
    ;;
esac

# ------------------------------------------------------------------------------
# 2. Source Feature Flags
# ------------------------------------------------------------------------------
# shellcheck source=../qemu_feature_flags.sh
source "${ROOT}/scripts/qemu_feature_flags.sh"
aqemu_qemu_feature_flags

JOBS="$(nproc 2>/dev/null || echo 4)"

# ------------------------------------------------------------------------------
# 3. Handle Special Targets: applesoc (Inferno) and reims (Reims vGPU)
# ------------------------------------------------------------------------------
# shellcheck source=../qemu_special_targets.sh
source "${ROOT}/scripts/qemu_special_targets.sh"

if aqemu_is_special_target "${TARGET}"; then
  aqemu_build_special_target "${TARGET}" "${PREFIX}" "${JOBS}"
  aqemu_qemu_verify_install "${PREFIX}" "${TARGET}"
  echo "Special target '${TARGET}' build completed successfully!"
  exit 0
fi

# ------------------------------------------------------------------------------
# 4. Standard QEMU Target Build
# ------------------------------------------------------------------------------
QEMU_SRC="${ROOT}/third_party/qemu"
if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU source submodule missing. Running: git submodule update --init --depth 1 third_party/qemu"
  git -C "${ROOT}" submodule update --init --depth 1 third_party/qemu
fi

# Handle target naming
TARGET_CLEAN="$(echo "${TARGET}" | sed 's/-softmmu$//')"
if [[ "${TARGET_CLEAN}" == "microblazeel" ]]; then
  CONFIG_TARGET="microblaze-softmmu"
else
  CONFIG_TARGET="${TARGET_CLEAN}-softmmu"
fi

BUILD_DIR="${BUILD_BASE}/qemu-build-${TARGET_CLEAN}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "Configuring target: ${CONFIG_TARGET}"
"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --target-list="${CONFIG_TARGET}" \
  "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"

echo "Building target ${CONFIG_TARGET} with ${JOBS} parallel jobs..."
ninja -C "${BUILD_DIR}" -j"${JOBS}"
ninja -C "${BUILD_DIR}" install

# Alias microblazeel if needed
if [[ "${TARGET_CLEAN}" == "microblazeel" ]]; then
  if [[ -f "${PREFIX}/bin/qemu-system-microblaze" && ! -f "${PREFIX}/bin/qemu-system-microblazeel" ]]; then
    ln -sf "qemu-system-microblaze" "${PREFIX}/bin/qemu-system-microblazeel"
  elif [[ -f "${PREFIX}/bin/qemu-system-microblaze.exe" && ! -f "${PREFIX}/bin/qemu-system-microblazeel.exe" ]]; then
    cp -f "${PREFIX}/bin/qemu-system-microblaze.exe" "${PREFIX}/bin/qemu-system-microblazeel.exe"
  fi
fi

# Verify installation
aqemu_qemu_verify_install "${PREFIX}" "${TARGET_CLEAN}"
echo "======================================================================"
echo "  Target [${TARGET_CLEAN}] built and installed to ${PREFIX}/bin"
echo "======================================================================"
