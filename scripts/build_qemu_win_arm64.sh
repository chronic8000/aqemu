#!/usr/bin/env bash
# ==============================================================================
# Build QEMU for Windows on ARM (WoA / Qualcomm Snapdragon Laptops)
# ==============================================================================
# Run from MSYS2 CLANGARM64 shell on Snapdragon X Elite / 8cx laptops.
#
# Usage:
#   ./scripts/build_qemu_win_arm64.sh [TARGET] [PREFIX]
#
# Examples:
#   ./scripts/build_qemu_win_arm64.sh                  # Builds ALL softmmu targets
#   ./scripts/build_qemu_win_arm64.sh aarch64          # Builds qemu-system-aarch64 only
#   ./scripts/build_qemu_win_arm64.sh x86_64           # Builds qemu-system-x86_64 only
# ==============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
TARGET_ARG="${1:-all}"
PREFIX="${2:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build-win-arm64"

export MSYSTEM=CLANGARM64
export PATH="/clangarm64/bin:/usr/bin:${PATH}"

# Meson runs under Win32 Python — tool paths must be Windows-style (C:/...)
export CC="clang"
export CXX="clang++"
export PKG_CONFIG="C:/msys64/clangarm64/bin/pkg-config.exe"
export PKG_CONFIG_PATH="C:/msys64/clangarm64/lib/pkgconfig"
unset PKG_CONFIG_LIBDIR || true

echo "=== Building QEMU for Windows on ARM (CLANGARM64 / Target: ${TARGET_ARG}) ==="
echo "Using compiler: $(which clang 2>/dev/null || echo clang)"
echo "Using PKG_CONFIG: ${PKG_CONFIG}"

# Ensure required QEMU build dependencies are installed
QEMU_BUILD_DEPS=(
  diffutils
  mingw-w64-clang-aarch64-glib2
  mingw-w64-clang-aarch64-pixman
  mingw-w64-clang-aarch64-libslirp
  mingw-w64-clang-aarch64-ninja
  mingw-w64-clang-aarch64-zlib
)
for dep in "${QEMU_BUILD_DEPS[@]}"; do
  if ! pacman -Q "$dep" >/dev/null 2>&1; then
    echo "Installing required dependency: $dep"
    pacman -S --needed --noconfirm "$dep"
  fi
done

if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Running: git submodule update --init --depth 1 third_party/qemu"
  git -C "${ROOT}" submodule update --init --depth 1 third_party/qemu
fi

# Ensure diff is on PATH for Meson
export PATH="C:/msys64/clangarm64/bin:C:/msys64/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
if [[ -x /usr/bin/diff.exe && ! -e /clangarm64/bin/diff.exe ]]; then
  cp -f /usr/bin/diff.exe /clangarm64/bin/diff.exe 2>/dev/null || true
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
  --cc=clang \
  --cxx=clang++ \
  --target-list="${TARGETS}" \
  "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"

JOBS="$(nproc 2>/dev/null || echo 4)"
echo "Building with ${JOBS} parallel jobs..."
ninja -C "${BUILD_DIR}" -j"${JOBS}"
ninja -C "${BUILD_DIR}" install

echo "Installed QEMU bundle to ${PREFIX}"
ls -la "${PREFIX}"/bin/qemu-system-* "${PREFIX}"/qemu-system-* "${PREFIX}"/bin/qemu-img* 2>/dev/null || true

aqemu_qemu_verify_install "${PREFIX}" "${VERIFY_TARGET}"
echo "Windows on ARM QEMU build completed successfully!"
