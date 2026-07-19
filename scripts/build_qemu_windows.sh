#!/usr/bin/env bash
# Windows (MSYS2 MinGW64) QEMU bundle build for AQEMU
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
PREFIX="${1:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build-win"

if [[ ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Run: git submodule update --init --depth 1 third_party/qemu"
  exit 1
fi

mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

EXTRA=(--enable-vnc --disable-docs --disable-guest-agent)
if pkg-config --exists spice-server 2>/dev/null; then
  EXTRA+=(--enable-spice)
fi

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --target-list=x86_64-softmmu,i386-softmmu,aarch64-softmmu \
  "${EXTRA[@]}"

ninja -C "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"
ninja -C "${BUILD_DIR}" install
echo "Installed to ${PREFIX}"
