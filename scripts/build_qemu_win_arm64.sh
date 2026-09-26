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

deploy_qemu_dlls() {
  local prefix="$1"
  local bin_dir="${prefix}/bin"
  [[ -d "${bin_dir}" ]] || bin_dir="${prefix}"
  echo "=== Deploying QEMU runtime DLLs into ${bin_dir} ==="

  for pass in 1 2 3 4; do
    local new_copied=0
    for bin in "${bin_dir}"/*.exe "${bin_dir}"/*.dll; do
      [[ -f "$bin" ]] || continue
      while read -r dep; do
        if [[ -f "$dep" && ! -f "${bin_dir}/$(basename "$dep")" ]]; then
          cp -f "$dep" "${bin_dir}/"
          new_copied=$((new_copied + 1))
        fi
      done < <(ldd "$bin" 2>/dev/null | grep -i '/clangarm64/bin/' | awk '{print $3}' | sort -u)
    done
    if [[ $new_copied -eq 0 ]]; then
      break
    fi
    echo "Pass $pass: deployed $new_copied runtime DLLs to ${bin_dir}"
  done

  # If AQEMU build_woa directory exists, synchronize QEMU executables and runtime DLLs into it
  if [[ -d "${ROOT}/build_woa" ]]; then
    echo "Synchronizing QEMU executables and runtime DLLs to ${ROOT}/build_woa/..."
    cp -f "${bin_dir}"/*.exe "${ROOT}/build_woa/" 2>/dev/null || true
    cp -f "${bin_dir}"/*.dll "${ROOT}/build_woa/" 2>/dev/null || true
  fi
}

if [[ "${TARGET_ARG}" == "--dlls-only" || "${TARGET_ARG}" == "--deploy-only" ]]; then
  deploy_qemu_dlls "${PREFIX}"
  echo "QEMU runtime DLL deployment completed successfully!"
  exit 0
fi

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
  mingw-w64-clang-aarch64-meson
  mingw-w64-clang-aarch64-python
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
  deploy_qemu_dlls "${PREFIX}"
  echo "Special target '${TARGET_ARG}' built and deployed successfully!"
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
  --cc=clang \
  --cxx=clang++ \
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

echo "Installed QEMU bundle to ${PREFIX}"
ls -la "${PREFIX}"/bin/qemu-system-* "${PREFIX}"/qemu-system-* "${PREFIX}"/bin/qemu-img* 2>/dev/null || true

aqemu_qemu_verify_install "${PREFIX}" "${VERIFY_TARGET}"
deploy_qemu_dlls "${PREFIX}"
echo "Windows on ARM QEMU build and runtime DLL deployment completed successfully!"
