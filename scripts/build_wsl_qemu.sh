#!/usr/bin/env bash
# Build the Linux ELFs AQEMU copies into the selected WSL distro.
# Output: third_party/wsl-qemu/<uname -m>/qemu-system-reims3d
#         third_party/wsl-qemu/<uname -m>/qemu-system-applesoc
set -euo pipefail

# Allow callers that strip CR into /tmp to keep the real repo root.
if [[ -z "${AQEMU_ROOT:-}" ]]; then
  ROOT="$(cd "$(dirname "$0")/.." && pwd)"
else
  ROOT="${AQEMU_ROOT}"
fi
ARCH="$(uname -m)"
case "${ARCH}" in
  arm64) ARCH=aarch64 ;;
  amd64) ARCH=x86_64 ;;
esac
OUT="${ROOT}/third_party/wsl-qemu/${ARCH}"
WORK="${HOME}/aqemu-wsl-build"
JOBS="$(nproc 2>/dev/null || echo 4)"
mkdir -p "${OUT}" "${WORK}"

echo "=== WSL QEMU ELFs for ${ARCH} ==="
echo "Output: ${OUT}"

# Same feature set as scripts/qemu_feature_flags.sh (slirp, vnc, spice, curl,
# libusb, usb-redir, gnutls, zstd, kvm, no SDL/GTK). On Linux that helper
# adds --enable-kvm instead of --enable-whpx.
export PKG_CONFIG="${PKG_CONFIG:-pkg-config}"
if [[ -x "${HOME}/.cargo/bin/cargo" ]]; then
  export PATH="${HOME}/.cargo/bin:${PATH}"
fi
# The shared script is checked out with CR LF. Strip those before sourcing.
# Upstream system emulators are configured below and must not abort when a
# Linux package the Windows builds require (libjpeg, libiscsi, …) is absent.
# shellcheck source=qemu_feature_flags.sh
if [[ "${AQEMU_WSL_ONLY:-all}" != "upstream" ]]; then
source <(tr -d '\r' < "${ROOT}/scripts/qemu_feature_flags.sh")
aqemu_qemu_feature_flags
spice_ok=0
for flag in "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"; do
  if [[ "${flag}" == "--enable-spice" ]]; then
    spice_ok=1
  fi
done
if [[ "${spice_ok}" -ne 1 ]]; then
  echo "ERROR: SPICE is required. Install libspice-server-dev so pkg-config finds spice-server." >&2
  exit 1
fi
fi

build_reims() {
  local reims="${ROOT}/third_party/reims-vgpu"
  local src="${reims}/vendor/qemu"
  local dist="${WORK}/reims-dist"
  local build="${WORK}/reims-qemu"
  local prefix="${WORK}/reims-prefix"
  if [[ ! -f "${src}/configure" ]]; then
    echo "ERROR: Reims QEMU sources missing at ${src}" >&2
    return 1
  fi
  export CARGO_TARGET_DIR="${WORK}/reims-cargo"
  mkdir -p "${CARGO_TARGET_DIR}"
  # Guest atomics use ldsetal. This Snapdragon has LSE; the aarch64 baseline does not.
  case "$(uname -m)" in
    aarch64|arm64)
      export RUSTFLAGS="${RUSTFLAGS:-} -C target-feature=+lse,+rcpc"
      ;;
  esac
  # The EFI option ROM links with lld-link. A preset LD=ld (GNU ld) ignores
  # the Makefile default and cannot take the /nologo /subsystem flags.
  export LD="$(command -v lld-link)"
  export OBJCOPY="$(command -v llvm-objcopy)"
  echo "=== cargo xtask dist vulkan ==="
  (
    cd "${reims}"
    cargo xtask dist --platform vulkan --out "${dist}"
  )
  export PKG_CONFIG_PATH="${dist}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"
  mkdir -p "${build}" "${prefix}"
  # Always reconfigure when PVG / device wiring may have changed.
  if [[ "${AQEMU_WSL_REIMS_RECONFIG:-0}" == "1" || ! -f "${build}/build.ninja" ]]; then
    rm -rf "${build}"
    mkdir -p "${build}"
    echo "=== configure Reims QEMU ==="
    (
      cd "${build}"
      # Match aqemu_build_reims: x86_64-softmmu, --enable-pvg, then the shared extras.
      "${src}/configure" \
        --prefix="${prefix}" \
        --target-list=x86_64-softmmu \
        --enable-pvg \
        "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"
    )
  else
    echo "=== resume Reims QEMU ==="
  fi
  echo "=== ninja Reims ==="
  ninja -C "${build}" -j"${JOBS}"
  ninja -C "${build}" install
  local bin="${prefix}/bin/qemu-system-x86_64"
  if [[ ! -f "${bin}" ]]; then
    echo "ERROR: ${bin} was not produced" >&2
    return 1
  fi
  if ! "${bin}" -device help 2>/dev/null | grep -q 'reims-vgpu-pci\|apple-gfx-pci'; then
    echo "ERROR: ${bin} has no apple-gfx-pci / reims-vgpu-pci device (CONFIG_MAC_PVG missing?)" >&2
    return 1
  fi
  cp -f "${bin}" "${OUT}/qemu-system-reims3d"
  chmod 755 "${OUT}/qemu-system-reims3d"
  echo "Installed ${OUT}/qemu-system-reims3d"
}

build_applesoc() {
  local src="${ROOT}/third_party/inferno"
  local build="${WORK}/inferno-qemu"
  local prefix="${WORK}/inferno-prefix"
  if [[ ! -f "${src}/configure" ]]; then
    echo "ERROR: Inferno sources missing at ${src}" >&2
    return 1
  fi
  rm -rf "${build}" "${prefix}"
  mkdir -p "${build}" "${prefix}"
  echo "=== configure Apple SoC QEMU ==="
  local flag
  local conf_args=(
    --prefix="${prefix}"
    --target-list=aarch64-softmmu,arm-softmmu
  )
  # Inferno has no docs option and must not take WHPX. Same filter as
  # aqemu_build_applesoc. System liblzfse supplies LZFSE on Linux.
  for flag in "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"; do
    if [[ "${flag}" == "--disable-docs" || "${flag}" == "--enable-docs" || "${flag}" == "--enable-whpx" ]]; then
      continue
    fi
    conf_args+=("${flag}")
  done
  conf_args+=(
    --disable-whpx
    --enable-lzfse
    --enable-nettle
  )
  (
    cd "${build}"
    "${src}/configure" "${conf_args[@]}"
  )
  echo "=== ninja Apple SoC ==="
  ninja -C "${build}" -j"${JOBS}"
  ninja -C "${build}" install
  local bin="${prefix}/bin/qemu-system-aarch64"
  if [[ ! -f "${bin}" ]]; then
    echo "ERROR: ${bin} was not produced" >&2
    return 1
  fi
  cp -f "${bin}" "${OUT}/qemu-system-applesoc"
  chmod 755 "${OUT}/qemu-system-applesoc"
  echo "Installed ${OUT}/qemu-system-applesoc"
}

build_upstream() {
  local win_src="${ROOT}/third_party/qemu"
  local src="${WORK}/qemu-src"
  local build="${WORK}/upstream-qemu"
  local prefix="${HOME}/aqemu-wsl-qemu"
  if [[ ! -f "${win_src}/configure" && ! -f "${win_src}/meson.build" ]]; then
    echo "ERROR: QEMU sources missing at ${win_src}" >&2
    return 1
  fi
  # The Windows checkout is CR LF. WSL exec fails on those shebangs, and
  # configure's option parser is a shell script. Copy onto ext4 and strip
  # CR from the scripts QEMU actually executes.
  if [[ ! -f "${src}/configure" ]]; then
    echo "=== copying QEMU sources onto the Linux filesystem ==="
    rm -rf "${src}"
    mkdir -p "${src}"
    tar -C "${win_src}" --exclude .git --exclude .configure-wsl.sh -cf - . | tar -C "${src}" -xf -
    find "${src}" -type f \( -name '*.sh' -o -name '*.py' -o -name 'configure' -o -path '*/scripts/*' \) -print0 \
      | xargs -0 sed -i 's/\r$//'
  fi
  # Windows git checks these out as text files. QEMU needs the real links
  # (libvhost-user and libvduse headers) when the Linux build compiles them.
  git -C "${win_src}" ls-files -s | grep '^120000 ' | while read -r _mode hash _stage path; do
    target="$(git -C "${win_src}" cat-file -p "${hash}")"
    target="${target//$'\r'/}"
    target="${target//$'\n'/}"
    rm -rf "${src}/${path}"
    ln -s "${target}" "${src}/${path}"
  done
  # shellcheck source=qemu_softmmu_targets.sh
  source <(tr -d '\r' < "${ROOT}/scripts/qemu_softmmu_targets.sh")
  local targets
  targets="$(qemu_softmmu_target_list "${src}")"
  echo "=== upstream QEMU targets: ${targets} ==="
  mkdir -p "${prefix}" "${OUT}"
  local conf=(
    --prefix="${prefix}"
    --target-list="${targets}"
    --disable-werror
    --disable-docs
    --disable-guest-agent
    --disable-sdl
    --disable-gtk
    --enable-kvm
    --enable-slirp
    --enable-spice
    --enable-libusb
    --enable-gnutls
    --enable-nettle
    --enable-png
    --enable-zstd
  )
  if [[ ! -f "${build}/build.ninja" ]]; then
    rm -rf "${build}"
    mkdir -p "${build}"
    (
      cd "${build}"
      bash "${src}/configure" "${conf[@]}"
    )
  fi
  echo "=== ninja upstream QEMU ==="
  ninja -C "${build}" -j"${JOBS}"
  ninja -C "${build}" install
  local bin
  for bin in "${prefix}/bin"/qemu-system-*; do
    [[ -f "${bin}" ]] || continue
    local base
    base="$(basename "${bin}")"
    # Leave the Linux Reims and Inferno ELFs in this folder alone.
    if [[ "${base}" == *reims* || "${base}" == *applesoc* || "${base}" == *inferno* ]]; then
      continue
    fi
    cp -f "${bin}" "${OUT}/${base}"
    chmod 755 "${OUT}/${base}"
  done
  echo "Installed upstream QEMU into ${prefix}/bin and ${OUT}"
}

case "${AQEMU_WSL_ONLY:-all}" in
  reims) build_reims ;;
  applesoc) build_applesoc ;;
  upstream) build_upstream ;;
  *)
    build_reims
    build_applesoc
    ;;
esac
echo "=== done ==="
ls -l "${OUT}"
