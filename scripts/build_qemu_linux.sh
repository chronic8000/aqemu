#!/usr/bin/env bash
# Build bundled QEMU targets for Linux / Raspberry Pi (AQEMU)
# Usage: scripts/build_qemu_linux.sh [PREFIX]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
PREFIX="${1:-${ROOT}/third_party/qemu-install}"
BUILD_DIR="${ROOT}/third_party/qemu-build"

if [[ ! -f "${QEMU_SRC}/configure" && ! -f "${QEMU_SRC}/meson.build" ]]; then
  echo "QEMU sources missing. Run: git submodule update --init --depth 1 third_party/qemu"
  exit 1
fi

mkdir -p "${BUILD_DIR}" "${PREFIX}"
cd "${BUILD_DIR}"

# Targets used by AQEMU wizard / templates
TARGETS="x86_64-softmmu,i386-softmmu,aarch64-softmmu"

EXTRA=()
if pkg-config --exists spice-server 2>/dev/null; then
  EXTRA+=(--enable-spice)
else
  echo "WARNING: spice-server not found; building without SPICE server (VNC fallback in AQEMU)."
fi

"${QEMU_SRC}/configure" \
  --prefix="${PREFIX}" \
  --target-list="${TARGETS}" \
  --enable-vnc \
  --disable-docs \
  --disable-guest-agent \
  "${EXTRA[@]}"

ninja -C "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"
ninja -C "${BUILD_DIR}" install

echo "Installed QEMU bundle to ${PREFIX}"
ls -la "${PREFIX}/bin"/qemu-system-* "${PREFIX}/bin"/qemu-img 2>/dev/null || true
