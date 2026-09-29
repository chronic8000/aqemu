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
  local_msys_bin="/ucrt64/bin"
  if command -v cygpath >/dev/null 2>&1; then
    WIN_BIN="$(cygpath -m /ucrt64/bin)"
  else
    WIN_BIN="C:/msys64/ucrt64/bin"
  fi
  export PKG_CONFIG="${WIN_BIN}/pkg-config.exe"
  export PKG_CONFIG_PATH="${WIN_BIN}/../lib/pkgconfig"
else
  export MSYSTEM=MINGW64
  export PATH="/mingw64/bin:/usr/bin:${PATH}"
  local_msys_bin="/mingw64/bin"
  if command -v cygpath >/dev/null 2>&1; then
    WIN_BIN="$(cygpath -m /mingw64/bin)"
  else
    WIN_BIN="C:/msys64/mingw64/bin"
  fi
  export PKG_CONFIG="${WIN_BIN}/pkg-config.exe"
  export PKG_CONFIG_PATH="${WIN_BIN}/../lib/pkgconfig"
fi
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
      done < <(ldd "$bin" 2>/dev/null | grep -iE '/(ucrt64|mingw64)/bin/' | awk '{print $3}' | sort -u)
    done
    if [[ $new_copied -eq 0 ]]; then
      break
    fi
    echo "Pass $pass: deployed $new_copied runtime DLLs to ${bin_dir}"
  done

  # If AQEMU build_win directory exists, synchronize QEMU executables and runtime DLLs into it
  if [[ -d "${ROOT}/build_win" ]]; then
    echo "Synchronizing QEMU executables and runtime DLLs to ${ROOT}/build_win/..."
    cp -f "${bin_dir}"/*.exe "${ROOT}/build_win/" 2>/dev/null || true
    cp -f "${bin_dir}"/*.dll "${ROOT}/build_win/" 2>/dev/null || true
  fi
}

if [[ "${TARGET_ARG}" == "--dlls-only" || "${TARGET_ARG}" == "--deploy-only" ]]; then
  deploy_qemu_dlls "${PREFIX}"
  echo "QEMU runtime DLL deployment completed successfully!"
  exit 0
fi

echo "=== Building QEMU for Windows x86_64 (${MSYSTEM} / Target: ${TARGET_ARG}) ==="
echo "Using compiler: $(which gcc 2>/dev/null || echo gcc)"
echo "Using PKG_CONFIG: ${PKG_CONFIG}"

if ! which diff >/dev/null 2>&1; then
  echo "Installing missing dependency: diffutils"
  pacman -S --needed --noconfirm diffutils
fi

if [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
  for dep in mingw-w64-ucrt-x86_64-meson mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python; do
    if ! pacman -Q "$dep" >/dev/null 2>&1; then
      pacman -S --needed --noconfirm "$dep" || true
    fi
  done
else
  for dep in mingw-w64-x86_64-meson mingw-w64-x86_64-ninja mingw-w64-x86_64-python; do
    if ! pacman -Q "$dep" >/dev/null 2>&1; then
      pacman -S --needed --noconfirm "$dep" || true
    fi
  done
fi

if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Running: git submodule update --init --depth 1 third_party/qemu"
  git -C "${ROOT}" submodule update --init --depth 1 third_party/qemu
fi

# Ensure diff is on PATH for Meson
export PATH="${local_msys_bin}:/usr/bin:${PATH}"
if [[ -x /usr/bin/diff.exe && ! -e "${local_msys_bin}/diff.exe" ]]; then
  cp -f /usr/bin/diff.exe "${local_msys_bin}/diff.exe" 2>/dev/null || true
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
  --cc=gcc \
  --cxx=g++ \
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
echo "Windows x86_64 QEMU build and runtime DLL deployment completed successfully!"
