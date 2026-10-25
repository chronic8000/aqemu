#!/usr/bin/env bash
# Build AQEMU for Windows x86_64 from the active MSYS2 UCRT64 or MINGW64 shell.
# Prefix, compilers, and Qt plugins come from MINGW_PREFIX (set by MSYS2).
# Usage: ./scripts/build_aqemu_win64.sh [--deps]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT}/build_win"

case "${MSYSTEM:-}" in
  UCRT64|MINGW64|CLANG64) ;;
  *)
    echo "Start this from the MSYS2 UCRT64 shell so the x86_64 prefix is active."
    echo "Current MSYSTEM='${MSYSTEM:-}' MINGW_PREFIX='${MINGW_PREFIX:-}'"
    exit 1
    ;;
esac
if [[ -z "${MINGW_PREFIX:-}" || ! -d "${MINGW_PREFIX}" ]]; then
  echo "MINGW_PREFIX is not set."
  exit 1
fi

export PATH="${MINGW_PREFIX}/bin:${PATH}"
compiler_path() {
  local name="$1"
  if [[ -f "${MINGW_PREFIX}/bin/${name}.exe" ]]; then
    printf '%s\n' "${MINGW_PREFIX}/bin/${name}.exe"
  elif [[ -f "${MINGW_PREFIX}/bin/${name}" ]]; then
    printf '%s\n' "${MINGW_PREFIX}/bin/${name}"
  else
    echo "No ${name} compiler in ${MINGW_PREFIX}/bin" >&2
    exit 1
  fi
}
export CC="$(compiler_path cc)"
export CXX="$(compiler_path c++)"
export PKG_CONFIG="${MINGW_PREFIX}/bin/pkg-config"
export PKG_CONFIG_PATH="${MINGW_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

if [[ "${1:-}" == "--deps" ]]; then
  echo "Installing packages for ${MSYSTEM} (${MINGW_PREFIX})..."
  if [[ -f /etc/pacman.conf ]] && grep -q '^#DisableDownloadTimeout' /etc/pacman.conf 2>/dev/null; then
    sed -i 's/^#DisableDownloadTimeout/DisableDownloadTimeout/' /etc/pacman.conf 2>/dev/null || true
  fi
  if [[ -f /etc/pacman.conf ]] && grep -q '^ParallelDownloads' /etc/pacman.conf 2>/dev/null; then
    sed -i 's/^ParallelDownloads.*/ParallelDownloads = 1/' /etc/pacman.conf 2>/dev/null || true
  fi
  case "${MSYSTEM}" in
    UCRT64) triplet="mingw-w64-ucrt-x86_64" ;;
    MINGW64) triplet="mingw-w64-x86_64" ;;
    CLANG64) triplet="mingw-w64-clang-x86_64" ;;
  esac
  DEPS=(
    "${triplet}-toolchain"
    "${triplet}-qt5-base"
    "${triplet}-qt5-tools"
    "${triplet}-cmake"
    "${triplet}-ninja"
    "${triplet}-pkgconf"
    "${triplet}-libvncserver"
    "${triplet}-spice-gtk"
  )
  ok=0
  for attempt in 1 2 3; do
    if pacman -S --needed --noconfirm "${DEPS[@]}"; then
      ok=1
      break
    fi
    echo "pacman attempt ${attempt} failed; retrying..."
    sleep 20
  done
  if [[ "${ok}" -ne 1 ]]; then
    echo "pacman could not install the ${MSYSTEM} packages."
    exit 1
  fi
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

win_prefix="$(cygpath -m "${MINGW_PREFIX}")"
win_cc="$(cygpath -m "${CC}")"
win_cxx="$(cygpath -m "${CXX}")"
win_root="$(cygpath -m "${ROOT}")"

CMAKE_FLAGS=(
  -G Ninja
  -DCMAKE_BUILD_TYPE=Release
  -DCMAKE_PREFIX_PATH="${win_prefix}"
  -DCMAKE_C_COMPILER="${win_cc}"
  -DCMAKE_CXX_COMPILER="${win_cxx}"
)

if [[ "${AQEMU_SKIP_SPICE:-}" != "1" ]] && "${PKG_CONFIG}" --exists spice-client-glib-2.0; then
  CMAKE_FLAGS+=(-DAQEMU_WITH_SPICE_GTK=ON)
fi

QEMU_BUNDLE="${ROOT}/third_party/qemu-install-x64"
if [[ ! -d "${QEMU_BUNDLE}/bin" && -d "${ROOT}/third_party/qemu-install/bin" ]]; then
  QEMU_BUNDLE="${ROOT}/third_party/qemu-install"
fi
if [[ -d "${QEMU_BUNDLE}/bin" ]]; then
  CMAKE_FLAGS+=(-DAQEMU_BUNDLE_QEMU=ON "-DAQEMU_QEMU_PREFIX=$(cygpath -m "${QEMU_BUNDLE}")")
fi

echo "Configuring x86_64 AQEMU with prefix ${MINGW_PREFIX}"
MSYS2_ARG_CONV_EXCL='*' cmake "${CMAKE_FLAGS[@]}" "${win_root}"
ninja -j"$(nproc 2>/dev/null || echo 4)"

echo "Deploying runtime libraries from ${MINGW_PREFIX}"
WDQ="$(command -v windeployqt-qt5 || command -v windeployqt || true)"
if [[ -n "${WDQ}" ]]; then
  "${WDQ}" --no-translations --compiler-runtime "${BUILD_DIR}/aqemu.exe" || true
fi

shopt -s nullglob
for plat in \
  "${MINGW_PREFIX}"/share/qt5/plugins/platforms/qwindows.dll \
  "${MINGW_PREFIX}"/lib/qt5/plugins/platforms/qwindows.dll \
  "${MINGW_PREFIX}"/qt5/plugins/platforms/qwindows.dll
do
  if [[ -f "${plat}" ]]; then
    mkdir -p "${BUILD_DIR}/platforms"
    cp -f "${plat}" "${BUILD_DIR}/platforms/"
    break
  fi
done

if [[ -d "${QEMU_BUNDLE}/bin" ]]; then
  cp -f "${QEMU_BUNDLE}/bin"/*.exe "${BUILD_DIR}/" 2>/dev/null || true
  cp -f "${QEMU_BUNDLE}/bin"/*.dll "${BUILD_DIR}/" 2>/dev/null || true
  if [[ -d "${QEMU_BUNDLE}/share" ]]; then
    mkdir -p "${BUILD_DIR}/share"
    cp -rf "${QEMU_BUNDLE}/share"/* "${BUILD_DIR}/share/" 2>/dev/null || true
  fi
fi

for _pass in 1 2 3 4; do
  copied=0
  for bin in "${BUILD_DIR}"/*.dll "${BUILD_DIR}"/*.exe "${BUILD_DIR}"/platforms/*.dll; do
    [[ -f "${bin}" ]] || continue
    while read -r base; do
      [[ -n "${base}" ]] || continue
      if [[ -f "${MINGW_PREFIX}/bin/${base}" && ! -f "${BUILD_DIR}/${base}" ]]; then
        cp -f "${MINGW_PREFIX}/bin/${base}" "${BUILD_DIR}/"
        copied=$((copied + 1))
      fi
    done < <("${MINGW_PREFIX}/bin/objdump" -p "${bin}" 2>/dev/null | awk '/DLL Name:/{ print $3 }')
  done
  if [[ "${copied}" -eq 0 ]]; then
    break
  fi
done

if [[ -d "${QEMU_BUNDLE}/bin" ]]; then
  cp -f "${BUILD_DIR}"/*.dll "${QEMU_BUNDLE}/bin/" 2>/dev/null || true
fi

echo "=== x86_64 build succeeded ==="
echo "Executable: ${BUILD_DIR}/aqemu.exe"
