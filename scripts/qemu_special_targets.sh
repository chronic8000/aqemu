#!/usr/bin/env bash
# ==============================================================================
# Special QEMU Targets Builder: applesoc (Inferno) and reims (Reims vGPU)
# ==============================================================================
# Shared helper functions to clone, configure, build, stage, and deploy:
#   1. qemu-system-applesoc (ChefKiss Inferno - Apple Silicon / iOS)
#   2. qemu-system-reimsvgpu / qemu-system-reims3d (steelbrain Reims vGPU)
#
# These are external forks of QEMU that live in separate Git repositories.
# Building into isolated staging directories prevents overwriting upstream
# qemu-system-aarch64 and qemu-system-x86_64 binaries.
# ==============================================================================
set -euo pipefail

aqemu_is_special_target() {
  local target="${1:-}"
  case "${target}" in
    applesoc|inferno|reims|reims3d|reimsvgpu)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

aqemu_build_applesoc() {
  local prefix="${1:?Install prefix required}"
  local jobs="${2:-$(nproc 2>/dev/null || echo 4)}"
  local root
  root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

  echo "======================================================================"
  echo "  Building Special Target: applesoc (ChefKiss Inferno Apple Silicon)"
  echo "  Install Prefix: ${prefix}"
  echo "======================================================================"

  local inferno_src="${root}/third_party/inferno"
  local inferno_build="${root}/third_party/inferno-build"
  local inferno_stage="${root}/third_party/inferno-stage"

  if [[ ! -d "${inferno_src}" || ! -f "${inferno_src}/configure" ]]; then
    echo "Cloning ChefKiss Inferno repository (Apple Silicon / iOS)..."
    git clone --depth 1 https://github.com/ChefKissInc/Inferno.git "${inferno_src}"
  else
    echo "Using existing ChefKiss Inferno source at ${inferno_src}"
  fi

  rm -rf "${inferno_stage}"
  mkdir -p "${inferno_build}" "${inferno_stage}" "${prefix}/bin"
  cd "${inferno_build}"

  # Ensure feature flags are loaded
  if [[ -z "${AQEMU_QEMU_EXTRA_CONFIGURE+x}" ]]; then
    if [[ -f "${root}/scripts/qemu_feature_flags.sh" ]]; then
      # shellcheck source=qemu_feature_flags.sh
      source "${root}/scripts/qemu_feature_flags.sh"
      aqemu_qemu_feature_flags
    fi
  fi

  local conf_args=(
    --prefix="${inferno_stage}"
    --target-list="aarch64-softmmu"
  )

  if [[ -n "${CC:-}" ]]; then
    conf_args+=(--cc="${CC}")
  fi
  if [[ -n "${CXX:-}" ]]; then
    conf_args+=(--cxx="${CXX}")
  fi
  if [[ -n "${AQEMU_QEMU_EXTRA_CONFIGURE:-}" ]]; then
    conf_args+=("${AQEMU_QEMU_EXTRA_CONFIGURE[@]}")
  fi

  echo "Configuring ChefKiss Inferno..."
  "${inferno_src}/configure" "${conf_args[@]}"

  echo "Compiling ChefKiss Inferno with ${jobs} parallel jobs..."
  ninja -C "${inferno_build}" -j"${jobs}"
  ninja -C "${inferno_build}" install

  # Copy built binary to prefix/bin as qemu-system-applesoc
  if [[ -f "${inferno_stage}/bin/qemu-system-aarch64.exe" ]]; then
    cp -f "${inferno_stage}/bin/qemu-system-aarch64.exe" "${prefix}/bin/qemu-system-applesoc.exe"
    echo "Successfully installed: ${prefix}/bin/qemu-system-applesoc.exe"
  elif [[ -f "${inferno_stage}/bin/qemu-system-aarch64" ]]; then
    cp -f "${inferno_stage}/bin/qemu-system-aarch64" "${prefix}/bin/qemu-system-applesoc"
    echo "Successfully installed: ${prefix}/bin/qemu-system-applesoc"
  elif [[ -f "${inferno_stage}/qemu-system-aarch64.exe" ]]; then
    cp -f "${inferno_stage}/qemu-system-aarch64.exe" "${prefix}/bin/qemu-system-applesoc.exe"
    echo "Successfully installed: ${prefix}/bin/qemu-system-applesoc.exe"
  elif [[ -f "${inferno_stage}/qemu-system-aarch64" ]]; then
    cp -f "${inferno_stage}/qemu-system-aarch64" "${prefix}/bin/qemu-system-applesoc"
    echo "Successfully installed: ${prefix}/bin/qemu-system-applesoc"
  else
    echo "ERROR: Failed to find compiled aarch64 binary in ${inferno_stage}" >&2
    return 1
  fi

  # Merge any share data without overwriting existing upstream files
  if [[ -d "${inferno_stage}/share" ]]; then
    mkdir -p "${prefix}/share"
    cp -rn "${inferno_stage}/share"/* "${prefix}/share/" 2>/dev/null || true
  fi

  echo "ChefKiss Inferno (qemu-system-applesoc) build completed successfully!"
}

aqemu_build_reims() {
  local prefix="${1:?Install prefix required}"
  local jobs="${2:-$(nproc 2>/dev/null || echo 4)}"
  local root
  root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

  echo "======================================================================"
  echo "  Building Special Target: reims (steelbrain Reims vGPU)"
  echo "  Install Prefix: ${prefix}"
  echo "======================================================================"

  local reims_dir="${root}/third_party/reims-vgpu"
  local reims_build="${root}/third_party/reims-build"
  local reims_stage="${root}/third_party/reims-stage"

  local reims_src=""
  if [[ -f "${reims_dir}/vendor/qemu/configure" ]]; then
    reims_src="${reims_dir}/vendor/qemu"
  elif [[ -f "${reims_dir}/configure" ]]; then
    reims_src="${reims_dir}"
  else
    echo "Cloning steelbrain qemu-reims-vgpu repository..."
    git clone --depth 1 -b host-reims-vgpu-vmapple https://github.com/steelbrain/qemu-reims-vgpu.git "${reims_dir}"
    reims_src="${reims_dir}"
  fi

  rm -rf "${reims_stage}"
  mkdir -p "${reims_build}" "${reims_stage}" "${prefix}/bin"
  cd "${reims_build}"

  # Ensure feature flags are loaded
  if [[ -z "${AQEMU_QEMU_EXTRA_CONFIGURE+x}" ]]; then
    if [[ -f "${root}/scripts/qemu_feature_flags.sh" ]]; then
      # shellcheck source=qemu_feature_flags.sh
      source "${root}/scripts/qemu_feature_flags.sh"
      aqemu_qemu_feature_flags
    fi
  fi

  local conf_args=(
    --prefix="${reims_stage}"
    --target-list="x86_64-softmmu"
  )

  if [[ -n "${CC:-}" ]]; then
    conf_args+=(--cc="${CC}")
  fi
  if [[ -n "${CXX:-}" ]]; then
    conf_args+=(--cxx="${CXX}")
  fi
  if [[ -n "${AQEMU_QEMU_EXTRA_CONFIGURE:-}" ]]; then
    conf_args+=("${AQEMU_QEMU_EXTRA_CONFIGURE[@]}")
  fi

  echo "Configuring steelbrain Reims vGPU..."
  "${reims_src}/configure" "${conf_args[@]}"

  echo "Compiling steelbrain Reims vGPU with ${jobs} parallel jobs..."
  ninja -C "${reims_build}" -j"${jobs}"
  ninja -C "${reims_build}" install

  # Copy built binary to prefix/bin as qemu-system-reimsvgpu / qemu-system-reims3d
  if [[ -f "${reims_stage}/bin/qemu-system-x86_64.exe" ]]; then
    cp -f "${reims_stage}/bin/qemu-system-x86_64.exe" "${prefix}/bin/qemu-system-reimsvgpu.exe"
    cp -f "${reims_stage}/bin/qemu-system-x86_64.exe" "${prefix}/bin/qemu-system-reims.exe"
    cp -f "${reims_stage}/bin/qemu-system-x86_64.exe" "${prefix}/bin/qemu-system-reims3d.exe"
    echo "Successfully installed: ${prefix}/bin/qemu-system-reimsvgpu.exe (and aliases)"
  elif [[ -f "${reims_stage}/bin/qemu-system-x86_64" ]]; then
    cp -f "${reims_stage}/bin/qemu-system-x86_64" "${prefix}/bin/qemu-system-reims3d"
    cp -f "${reims_stage}/bin/qemu-system-x86_64" "${prefix}/bin/qemu-system-reimsvgpu"
    cp -f "${reims_stage}/bin/qemu-system-x86_64" "${prefix}/bin/qemu-system-reims"
    echo "Successfully installed: ${prefix}/bin/qemu-system-reims3d (and aliases)"
  elif [[ -f "${reims_stage}/qemu-system-x86_64.exe" ]]; then
    cp -f "${reims_stage}/qemu-system-x86_64.exe" "${prefix}/bin/qemu-system-reimsvgpu.exe"
    cp -f "${reims_stage}/qemu-system-x86_64.exe" "${prefix}/bin/qemu-system-reims.exe"
    cp -f "${reims_stage}/qemu-system-x86_64.exe" "${prefix}/bin/qemu-system-reims3d.exe"
    echo "Successfully installed: ${prefix}/bin/qemu-system-reimsvgpu.exe (and aliases)"
  elif [[ -f "${reims_stage}/qemu-system-x86_64" ]]; then
    cp -f "${reims_stage}/qemu-system-x86_64" "${prefix}/bin/qemu-system-reims3d"
    cp -f "${reims_stage}/qemu-system-x86_64" "${prefix}/bin/qemu-system-reimsvgpu"
    cp -f "${reims_stage}/qemu-system-x86_64" "${prefix}/bin/qemu-system-reims"
    echo "Successfully installed: ${prefix}/bin/qemu-system-reims3d (and aliases)"
  else
    echo "ERROR: Failed to find compiled x86_64 binary in ${reims_stage}" >&2
    return 1
  fi

  # Merge any share data without overwriting existing upstream files
  if [[ -d "${reims_stage}/share" ]]; then
    mkdir -p "${prefix}/share"
    cp -rn "${reims_stage}/share"/* "${prefix}/share/" 2>/dev/null || true
  fi

  echo "steelbrain Reims vGPU build completed successfully!"
}

aqemu_build_special_target() {
  local target="${1:?Target required}"
  local prefix="${2:?Install prefix required}"
  local jobs="${3:-$(nproc 2>/dev/null || echo 4)}"
  case "${target}" in
    applesoc|inferno)
      aqemu_build_applesoc "${prefix}" "${jobs}"
      ;;
    reims|reims3d|reimsvgpu)
      aqemu_build_reims "${prefix}" "${jobs}"
      ;;
    *)
      echo "Unknown special target: ${target}" >&2
      return 1
      ;;
  esac
}

aqemu_build_all_special_targets() {
  local prefix="${1:?Install prefix required}"
  local jobs="${2:-$(nproc 2>/dev/null || echo 4)}"
  aqemu_build_applesoc "${prefix}" "${jobs}"
  aqemu_build_reims "${prefix}" "${jobs}"
}
