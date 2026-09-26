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

aqemu_ensure_meson() {
  local root="$1"

  # Ensure common MSYS2 bin paths are on PATH
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    export PATH="C:/msys64/clangarm64/bin:C:/msys64/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
    export PATH="C:/msys64/ucrt64/bin:C:/msys64/usr/bin:/ucrt64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "MINGW64" ]]; then
    export PATH="C:/msys64/mingw64/bin:C:/msys64/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
  fi

  if command -v meson >/dev/null 2>&1; then
    echo "Found meson: $(command -v meson)"
    return 0
  fi

  # Attempt pacman installation
  if command -v pacman >/dev/null 2>&1; then
    case "${MSYSTEM:-}" in
      CLANGARM64)
        echo "Installing missing dependency: mingw-w64-clang-aarch64-meson..."
        pacman -S --needed --noconfirm mingw-w64-clang-aarch64-meson mingw-w64-clang-aarch64-python 2>/dev/null || true
        ;;
      UCRT64)
        echo "Installing missing dependency: mingw-w64-ucrt-x86_64-meson..."
        pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-meson mingw-w64-ucrt-x86_64-python 2>/dev/null || true
        ;;
      MINGW64)
        echo "Installing missing dependency: mingw-w64-x86_64-meson..."
        pacman -S --needed --noconfirm mingw-w64-x86_64-meson mingw-w64-x86_64-python 2>/dev/null || true
        ;;
      *)
        pacman -S --needed --noconfirm meson 2>/dev/null || true
        ;;
    esac
  fi

  if command -v meson >/dev/null 2>&1; then
    echo "Installed meson: $(command -v meson)"
    return 0
  fi

  # Check existing QEMU pyvenv for meson
  local pyvenv_meson
  pyvenv_meson="$(find "${root}/third_party" -maxdepth 4 \( -name "meson" -o -name "meson.exe" \) -path "*/pyvenv/*" 2>/dev/null | head -1 || true)"
  if [[ -n "${pyvenv_meson}" ]]; then
    local pyvenv_bin
    pyvenv_bin="$(dirname "${pyvenv_meson}")"
    echo "Using meson from pyvenv at ${pyvenv_bin}"
    export PATH="${pyvenv_bin}:${PATH}"
    return 0
  fi

  # Check if python can run meson
  for py_cand in python3 python /clangarm64/bin/python.exe /ucrt64/bin/python.exe /mingw64/bin/python.exe; do
    if command -v "$py_cand" >/dev/null 2>&1 && "$py_cand" -c "import mesonbuild" 2>/dev/null; then
      local shim_dir="${root}/third_party/.bin"
      mkdir -p "${shim_dir}"
      cat <<EOF > "${shim_dir}/meson"
#!/usr/bin/env bash
exec "$py_cand" -m mesonbuild.mesonmain "\$@"
EOF
      chmod +x "${shim_dir}/meson"
      export PATH="${shim_dir}:${PATH}"
      echo "Configured meson wrapper using $py_cand at ${shim_dir}/meson"
      return 0
    fi
  done

  # Fallback for Debian/Ubuntu
  if command -v apt-get >/dev/null 2>&1; then
    echo "Installing meson via apt..."
    sudo apt-get update && sudo apt-get install -y meson || true
  fi

  if ! command -v meson >/dev/null 2>&1; then
    echo "ERROR: 'meson' command not found. Please run: pacman -S mingw-w64-clang-aarch64-meson" >&2
    return 1
  fi
}

aqemu_ensure_cargo() {
  local root="$1"

  # 1. Check if cargo is already in PATH
  if command -v cargo >/dev/null 2>&1; then
    echo "Found cargo: $(command -v cargo)"
    return 0
  fi

  # 2. Check common Windows user .cargo/bin paths
  for cand in \
    "${USERPROFILE:-}/.cargo/bin" \
    "${HOME}/.cargo/bin" \
    "/c/Users/${USER:-}/.cargo/bin" \
    "/c/Users/${USERNAME:-}/.cargo/bin" \
    /c/Users/*/.cargo/bin \
    /clangarm64/bin \
    /ucrt64/bin \
    /mingw64/bin; do
    if [[ -x "${cand}/cargo.exe" || -x "${cand}/cargo" ]]; then
      echo "Found cargo in ${cand}, adding to PATH..."
      export PATH="${cand}:${PATH}"
      return 0
    fi
  done

  # 3. Check pacman on MSYS2
  if command -v pacman >/dev/null 2>&1; then
    case "${MSYSTEM:-}" in
      CLANGARM64)
        echo "Installing missing dependency: mingw-w64-clang-aarch64-rust (provides cargo)..."
        pacman -S --needed --noconfirm mingw-w64-clang-aarch64-rust 2>/dev/null || true
        ;;
      UCRT64)
        echo "Installing missing dependency: mingw-w64-ucrt-x86_64-rust (provides cargo)..."
        pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-rust 2>/dev/null || true
        ;;
      MINGW64)
        echo "Installing missing dependency: mingw-w64-x86_64-rust (provides cargo)..."
        pacman -S --needed --noconfirm mingw-w64-x86_64-rust 2>/dev/null || true
        ;;
      *)
        pacman -S --needed --noconfirm rust 2>/dev/null || true
        ;;
    esac
    if command -v cargo >/dev/null 2>&1; then
      echo "Installed cargo: $(command -v cargo)"
      return 0
    fi
  fi

  # 4. Check apt-get on Debian/Ubuntu
  if command -v apt-get >/dev/null 2>&1; then
    echo "Installing cargo via apt..."
    sudo apt-get update && sudo apt-get install -y cargo rustc || true
  fi

  if command -v cargo >/dev/null 2>&1; then
    echo "Found cargo: $(command -v cargo)"
    return 0
  fi

  # 5. Check if rustup can be run
  if command -v rustup >/dev/null 2>&1; then
    rustup default stable || true
    if command -v cargo >/dev/null 2>&1; then
      return 0
    fi
  fi

  echo "ERROR: 'cargo' (Rust toolchain) is required to compile steelbrain Reims vGPU." >&2
  echo "Please install Rust (e.g. pacman -S mingw-w64-clang-aarch64-rust or https://rustup.rs)" >&2
  return 1
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

  aqemu_ensure_meson "${root}"

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

  aqemu_ensure_meson "${root}"
  aqemu_ensure_cargo "${root}"

  # Reims vGPU uses Vulkan on Windows and Linux (Metal is Apple macOS only)
  case "$(uname -s 2>/dev/null || echo unknown)" in
    Darwin)
      export REIMS_VGPU_BACKEND="${REIMS_VGPU_BACKEND:-metal}"
      ;;
    *)
      export REIMS_VGPU_BACKEND="${REIMS_VGPU_BACKEND:-vulkan}"
      ;;
  esac

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
    -Dreims_vgpu_backend="${REIMS_VGPU_BACKEND}"
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
