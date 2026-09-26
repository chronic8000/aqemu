#!/usr/bin/env bash
# Pure MSYS2 MinGW64 build of bundled QEMU (same toolchain for compiler + glib/spice).
# Prerequisite: scripts/fix_msys2_gcc_admin.ps1 run as Administrator once.
#
# Builds EVERY softmmu target with the full AQEMU feature set (slirp/user net,
# spice, vnc, usb, curl, …). Partial feature builds are rejected at verify time.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
TARGET_ARG="all"
PREFIX="${ROOT}/third_party/qemu-install"
if [[ $# -ge 2 ]]; then
  TARGET_ARG="$1"
  PREFIX="$2"
elif [[ $# -eq 1 ]]; then
  if [[ "$1" == /* || "$1" == ./* || "$1" == ../* || "$1" =~ ^[a-zA-Z]: ]]; then
    PREFIX="$1"
  else
    TARGET_ARG="$1"
  fi
fi
BUILD_DIR="${ROOT}/third_party/qemu-build-win"

export MSYSTEM=MINGW64
# /usr/bin first so Meson can find msys `diff` (via C:\msys64\usr\bin)
export PATH="/mingw64/bin:/usr/bin:${PATH}"
# Meson runs under Win32 Python — must use C:/ paths (not /mingw64/...)
export PKG_CONFIG="C:/msys64/mingw64/bin/pkg-config.exe"
export PKG_CONFIG_PATH="C:/msys64/mingw64/lib/pkgconfig"
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
      done < <(ldd "$bin" 2>/dev/null | grep -iE '/(mingw64|ucrt64)/bin/' | awk '{print $3}' | sort -u)
    done
    if [[ $new_copied -eq 0 ]]; then
      break
    fi
    echo "Pass $pass: deployed $new_copied runtime DLLs to ${bin_dir}"
  done

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

echo "Using $(which gcc)"
echo "Using PKG_CONFIG=$PKG_CONFIG"
gcc --version | head -1
if ! printf '%s\n' 'int main(void){return 0;}' | gcc -x c - -c -o /tmp/aqemu_cc_probe.o; then
  echo "ERROR: MSYS2 gcc cannot compile. Run as Admin:"
  echo "  powershell -ExecutionPolicy Bypass -File scripts/fix_msys2_gcc_admin.ps1"
  exit 1
fi

"$PKG_CONFIG" --modversion glib-2.0

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

rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

# Win32 Meson looks on PATH for `diff`; put MSYS tools on a Windows-style PATH
export PATH="C:/msys64/mingw64/bin:C:/msys64/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
# Ensure `diff` is next to other mingw tools
if [[ -x /usr/bin/diff.exe && ! -e /mingw64/bin/diff.exe ]]; then
  cp -f /usr/bin/diff.exe /mingw64/bin/diff.exe
fi
command -v diff
diff --version | head -1

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

echo "Building softmmu targets: ${TARGETS}"

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --cc=gcc \
  --cxx=g++ \
  --target-list="${TARGETS}" \
  "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"

echo "=== configure OK, building ==="
ninja -j"${JOBS}"
ninja install

# Build special targets when building 'all'
if [[ "${TARGET_ARG}" == "all" || "${TARGET_ARG}" == "ALL" ]]; then
  echo "=== Building special target: applesoc (ChefKiss Inferno) ==="
  aqemu_build_applesoc "${PREFIX}" "${JOBS}"

  echo "=== Building special target: reims (steelbrain Reims vGPU) ==="
  aqemu_build_reims "${PREFIX}" "${JOBS}"
fi

echo "Installed to ${PREFIX}"
ls -la "${PREFIX}"/bin/qemu-system-* "${PREFIX}"/qemu-system-* "${PREFIX}"/bin/qemu-img* "${PREFIX}"/qemu-img* 2>/dev/null || true
echo "Firmware share:"
ls -la "${PREFIX}/share/bios-256k.bin" "${PREFIX}/share/edk2-x86_64-code.fd" 2>/dev/null || \
  ls -la "${PREFIX}/share/qemu/bios-256k.bin" 2>/dev/null || \
  echo "WARNING: bios/EDK2 missing under ${PREFIX}/share — Store package will not boot guests"

aqemu_qemu_verify_install "${PREFIX}" "${VERIFY_TARGET}"
deploy_qemu_dlls "${PREFIX}"
echo "Windows MSYS2 QEMU build and runtime DLL deployment completed successfully!"
