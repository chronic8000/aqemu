#!/usr/bin/env bash
# Internal helper: Store-channel aqemu.exe only (skip unit tests that may not link).
# Usage from MSYS2: AQEMU_STORE_BUILD=1 ./scripts/_store_build_aqemu.sh {woa|win64}
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TARGET="${1:-}"

case "$TARGET" in
  woa)
    export MSYSTEM=CLANGARM64
    BUILD_DIR="${ROOT}/build_woa"
    SCRIPT="${ROOT}/scripts/build_aqemu_woa.sh"
    QEMU_BUNDLE="${ROOT}/third_party/qemu-install-arm64"
    ;;
  win64)
    export MSYSTEM=UCRT64
    BUILD_DIR="${ROOT}/build_win"
    SCRIPT="${ROOT}/scripts/build_aqemu_win64.sh"
    QEMU_BUNDLE="${ROOT}/third_party/qemu-install"
    ;;
  *)
    echo "Usage: $0 {woa|win64}" >&2
    exit 2
    ;;
esac

export AQEMU_STORE_BUILD=1
export CHERE_INVOKING=1
# Only load profile when launched outside an MSYS login shell (re-sourcing breaks
# non-interactive runs on `read` of printer registry keys under set -e).
if [[ -z "${MINGW_PREFIX:-}" ]]; then
  # shellcheck disable=SC1091
  source /etc/profile
fi
if [[ -z "${MINGW_PREFIX:-}" || ! -d "${MINGW_PREFIX}" ]]; then
  echo "MINGW_PREFIX not set for MSYSTEM=${MSYSTEM:-}" >&2
  exit 1
fi
export PATH="${MINGW_PREFIX}/bin:${PATH}"

# Configure + attempt full build; ignore test-link failures, then build aqemu target.
set +e
"$SCRIPT"
set -e

cd "${BUILD_DIR}"
ninja aqemu

WDQ="$(command -v windeployqt-qt5 || command -v windeployqt || true)"
if [[ -n "${WDQ}" ]]; then
  "${WDQ}" --no-translations --compiler-runtime "${BUILD_DIR}/aqemu.exe" || true
fi

if [[ -d "${QEMU_BUNDLE}/bin" ]]; then
  cp -f "${QEMU_BUNDLE}/bin"/*.exe "${BUILD_DIR}/" 2>/dev/null || true
  cp -f "${QEMU_BUNDLE}/bin"/*.dll "${BUILD_DIR}/" 2>/dev/null || true
  if [[ -d "${QEMU_BUNDLE}/share" ]]; then
    mkdir -p "${BUILD_DIR}/share"
    cp -rf "${QEMU_BUNDLE}/share"/* "${BUILD_DIR}/share/" 2>/dev/null || true
  fi
fi

# Reims aliases (same binary, alternate names used by AQEMU UI)
if [[ -f "${BUILD_DIR}/qemu-system-reims3d.exe" ]]; then
  for a in qemu-system-reims.exe qemu-system-reimsvgpu.exe qemu-system-x86_64-reims.exe; do
    cp -f "${BUILD_DIR}/qemu-system-reims3d.exe" "${BUILD_DIR}/${a}"
  done
fi

echo "=== Store ${TARGET} aqemu ready ==="
ls -la "${BUILD_DIR}/aqemu.exe"
echo "qemu-system count: $(ls "${BUILD_DIR}"/qemu-system-*.exe 2>/dev/null | wc -l)"
