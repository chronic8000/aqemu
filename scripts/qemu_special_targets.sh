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
  if command -v cygpath >/dev/null 2>&1; then
    case "${MSYSTEM:-}" in
      CLANGARM64) export PATH="$(cygpath -m /clangarm64/bin):/clangarm64/bin:/usr/bin:${PATH}" ;;
      UCRT64)     export PATH="$(cygpath -m /ucrt64/bin):/ucrt64/bin:/usr/bin:${PATH}" ;;
      MINGW64)    export PATH="$(cygpath -m /mingw64/bin):/mingw64/bin:/usr/bin:${PATH}" ;;
    esac
  elif [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
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

  # 1. Prefer user .cargo/bin paths first (e.g. rustup toolchain)
  for cand in \
    "${HOME}/.cargo/bin" \
    "${USERPROFILE:-}/.cargo/bin" \
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

  # 2. Check if cargo is already in PATH
  if command -v cargo >/dev/null 2>&1; then
    echo "Found cargo: $(command -v cargo)"
    return 0
  fi

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

aqemu_ensure_lzfse() {
  local root="$1"
  local lzfse_dir="${root}/third_party/lzfse"
  local lzfse_build="${root}/third_party/lzfse-build"
  local lzfse_install="${root}/third_party/lzfse-install"

  # Ensure common MSYS2 bin paths are on PATH
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    export PATH="C:/msys64/clangarm64/bin:C:/msys64/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
    export PATH="C:/msys64/ucrt64/bin:C:/msys64/usr/bin:/ucrt64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "MINGW64" ]]; then
    export PATH="C:/msys64/mingw64/bin:C:/msys64/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
  fi

  # Determine MSYS2 prefix if applicable
  local msys_prefix=""
  if [[ -n "${MINGW_PREFIX:-}" && -d "${MINGW_PREFIX}/include" ]]; then
    msys_prefix="${MINGW_PREFIX}"
  elif [[ "${MSYSTEM:-}" == "CLANGARM64" && -d "/clangarm64/include" ]]; then
    msys_prefix="/clangarm64"
  elif [[ "${MSYSTEM:-}" == "UCRT64" && -d "/ucrt64/include" ]]; then
    msys_prefix="/ucrt64"
  elif [[ "${MSYSTEM:-}" == "MINGW64" && -d "/mingw64/include" ]]; then
    msys_prefix="/mingw64"
  elif [[ -d "C:/msys64/clangarm64/include" ]]; then
    msys_prefix="C:/msys64/clangarm64"
  fi

  # 1. Check if lzfse.h and liblzfse.a / lzfse library already exist
  local found_h=0
  for inc_dir in \
    "${msys_prefix}/include" \
    /clangarm64/include \
    /ucrt64/include \
    /mingw64/include \
    /usr/include \
    /usr/local/include \
    "${lzfse_install}/include"; do
    if [[ -n "${inc_dir}" && -f "${inc_dir}/lzfse.h" ]]; then
      echo "Found lzfse.h in ${inc_dir}"
      found_h=1
      break
    fi
  done

  # 2. Try system package manager if missing on Linux
  if [[ ${found_h} -eq 0 ]]; then
    if command -v apt-get >/dev/null 2>&1; then
      echo "Installing lzfse via apt..."
      sudo apt-get update && sudo apt-get install -y lzfse liblzfse-dev || true
      for inc_dir in /usr/include /usr/local/include; do
        if [[ -f "${inc_dir}/lzfse.h" ]]; then
          found_h=1
          break
        fi
      done
    fi
  fi

  # 3. If still not installed or missing static library, build from source
  if [[ ${found_h} -eq 0 || ! -f "${lzfse_install}/lib/liblzfse.a" ]]; then
    echo "======================================================================"
    echo "  Building lzfse (Apple LZFSE compression library for Inferno)"
    echo "======================================================================"
    if [[ ! -d "${lzfse_dir}" || ! -f "${lzfse_dir}/CMakeLists.txt" ]]; then
      echo "Cloning lzfse repository..."
      git clone --depth 1 https://github.com/lzfse/lzfse.git "${lzfse_dir}"
    fi

    mkdir -p "${lzfse_build}" "${lzfse_install}/include" "${lzfse_install}/lib" "${lzfse_install}/lib/pkgconfig"

    local c_compiler="${CC:-}"
    if [[ -z "${c_compiler}" ]]; then
      if command -v clang >/dev/null 2>&1; then
        c_compiler="clang"
      elif command -v gcc >/dev/null 2>&1; then
        c_compiler="gcc"
      else
        c_compiler="cc"
      fi
    fi

    local ar_tool="ar"
    if command -v llvm-ar >/dev/null 2>&1; then
      ar_tool="llvm-ar"
    elif command -v ar >/dev/null 2>&1; then
      ar_tool="ar"
    fi

    echo "Compiling lzfse static library using ${c_compiler}..."
    (
      cd "${lzfse_dir}"
      "${c_compiler}" -O3 -fPIC -c \
        src/lzfse_decode.c src/lzfse_decode_base.c src/lzfse_encode.c \
        src/lzfse_encode_base.c src/lzfse_fse.c src/lzvn_decode_base.c \
        src/lzvn_encode_base.c
      "${ar_tool}" rcs "${lzfse_install}/lib/liblzfse.a" *.o
      cp -f src/lzfse.h "${lzfse_install}/include/"
      rm -f *.o
    )

    # Generate pkg-config metadata
    cat <<EOF > "${lzfse_install}/lib/pkgconfig/lzfse.pc"
prefix=${lzfse_install}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: lzfse
Description: LZFSE compression library
Version: 1.0
Libs: -L\${libdir} -llzfse
Cflags: -I\${includedir}
EOF

    # In MSYS2, also copy into toolchain sysroot so all subprojects/tools find it automatically
    if [[ -n "${msys_prefix}" ]]; then
      if [[ -d "${msys_prefix}/include" && -w "${msys_prefix}/include" ]]; then
        echo "Installing lzfse headers and library into MSYS2 sysroot (${msys_prefix})..."
        cp -f "${lzfse_install}/include/lzfse.h" "${msys_prefix}/include/" 2>/dev/null || true
        cp -f "${lzfse_install}/lib/liblzfse.a" "${msys_prefix}/lib/" 2>/dev/null || true
        mkdir -p "${msys_prefix}/lib/pkgconfig"
        cp -f "${lzfse_install}/lib/pkgconfig/lzfse.pc" "${msys_prefix}/lib/pkgconfig/" 2>/dev/null || true
      fi
    fi
  fi

  # Export compiler and linker search paths
  export CFLAGS="-I${lzfse_install}/include ${CFLAGS:-}"
  export CXXFLAGS="-I${lzfse_install}/include ${CXXFLAGS:-}"
  export LDFLAGS="-L${lzfse_install}/lib ${LDFLAGS:-}"
  export PKG_CONFIG_PATH="${lzfse_install}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

  return 0
}

aqemu_ensure_nettle() {
  local root="$1"

  # Ensure common MSYS2 bin paths are on PATH
  if command -v cygpath >/dev/null 2>&1; then
    case "${MSYSTEM:-}" in
      CLANGARM64) export PATH="$(cygpath -m /clangarm64/bin):/clangarm64/bin:/usr/bin:${PATH}" ;;
      UCRT64)     export PATH="$(cygpath -m /ucrt64/bin):/ucrt64/bin:/usr/bin:${PATH}" ;;
      MINGW64)    export PATH="$(cygpath -m /mingw64/bin):/mingw64/bin:/usr/bin:${PATH}" ;;
    esac
  elif [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    export PATH="C:/msys64/clangarm64/bin:C:/msys64/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
    export PATH="C:/msys64/ucrt64/bin:C:/msys64/usr/bin:/ucrt64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "MINGW64" ]]; then
    export PATH="C:/msys64/mingw64/bin:C:/msys64/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
  fi

  local pkg_tool="${PKG_CONFIG:-pkg-config}"
  if "$pkg_tool" --exists nettle hogweed gmp 2>/dev/null; then
    echo "Found nettle, hogweed, and gmp via pkg-config"
    return 0
  fi

  if command -v pacman >/dev/null 2>&1; then
    case "${MSYSTEM:-}" in
      CLANGARM64)
        echo "Installing missing nettle/gmp dependencies for CLANGARM64..."
        pacman -S --needed --noconfirm mingw-w64-clang-aarch64-nettle mingw-w64-clang-aarch64-gmp 2>/dev/null || \
        pacman -S --needed --noconfirm mingw-w64-clang-aarch64-nettle3 mingw-w64-clang-aarch64-gmp 2>/dev/null || true
        ;;
      UCRT64)
        echo "Installing missing nettle/gmp dependencies for UCRT64..."
        pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-nettle mingw-w64-ucrt-x86_64-gmp 2>/dev/null || \
        pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-nettle3 mingw-w64-ucrt-x86_64-gmp 2>/dev/null || true
        ;;
      MINGW64)
        echo "Installing missing nettle/gmp dependencies for MINGW64..."
        pacman -S --needed --noconfirm mingw-w64-x86_64-nettle mingw-w64-x86_64-gmp 2>/dev/null || \
        pacman -S --needed --noconfirm mingw-w64-x86_64-nettle3 mingw-w64-x86_64-gmp 2>/dev/null || true
        ;;
      *)
        pacman -S --needed --noconfirm nettle gmp 2>/dev/null || true
        ;;
    esac
  fi

  if command -v apt-get >/dev/null 2>&1; then
    if ! "$pkg_tool" --exists nettle hogweed 2>/dev/null; then
      echo "Installing nettle/gmp via apt..."
      sudo apt-get update && sudo apt-get install -y libnettle-dev libhogweed-dev libgmp-dev || true
    fi
  fi

  return 0
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
  aqemu_ensure_lzfse "${root}"
  aqemu_ensure_nettle "${root}"

  local inferno_src="${root}/third_party/inferno"
  local inferno_build="${root}/third_party/inferno-build"
  local inferno_stage="${root}/third_party/inferno-stage"
  local lzfse_install="${root}/third_party/lzfse-install"

  if [[ ! -d "${inferno_src}" || ! -f "${inferno_src}/configure" ]]; then
    echo "Cloning ChefKiss Inferno repository (Apple Silicon / iOS)..."
    git clone --depth 1 https://github.com/ChefKissInc/Inferno.git "${inferno_src}"
  else
    echo "Using existing ChefKiss Inferno source at ${inferno_src}"
  fi

  # Ensure Inferno's required C library submodules (util/mlib) are initialized
  if [[ ! -f "${inferno_src}/util/mlib/m-algo.h" ]]; then
    echo "Initializing Inferno submodule: util/mlib..."
    git -C "${inferno_src}" submodule update --init --depth 1 util/mlib
  fi

  # Ensure hw/arm/apple-silicon links against liblzfse, nettle, hogweed, and gmp
  if [[ -f "${inferno_src}/hw/arm/apple-silicon/meson.build" ]]; then
    python3 -c "
p = '${inferno_src}/hw/arm/apple-silicon/meson.build'
content = open(p).read()
if 'hogweed' not in content:
    print('Adding nettle, hogweed, gmp dependencies to hw/arm/apple-silicon/meson.build...')
    content = content.replace('if_true: [tasn1, liblzfse]', 'if_true: [tasn1, liblzfse, nettle, hogweed, gmp]')
    content = content.replace('if_true: tasn1', 'if_true: [tasn1, liblzfse, nettle, hogweed, gmp]')
    open(p, 'w').write(content)
" || true
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

  local lzfse_inc_flag="-I${lzfse_install}/include"
  local lzfse_lib_flag="-L${lzfse_install}/lib"
  if command -v cygpath >/dev/null 2>&1; then
    lzfse_inc_flag="-I$(cygpath -m "${lzfse_install}/include")"
    lzfse_lib_flag="-L$(cygpath -m "${lzfse_install}/lib")"
  fi

  local conf_args=(
    --prefix="${inferno_stage}"
    --target-list="aarch64-softmmu,arm-softmmu"
  )

  if [[ -n "${CC:-}" ]]; then
    conf_args+=(--cc="${CC}")
  fi
  if [[ -n "${CXX:-}" ]]; then
    conf_args+=(--cxx="${CXX}")
  fi
  if [[ -n "${AQEMU_QEMU_EXTRA_CONFIGURE:-}" ]]; then
    for flag in "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"; do
      # Inferno removed 'docs' and Apple Silicon emulation is strictly TCG (no WHPX)
      if [[ "$flag" == "--disable-docs" || "$flag" == "--enable-docs" || "$flag" == "--enable-whpx" ]]; then
        continue
      fi
      conf_args+=("$flag")
    done
  fi
  conf_args+=(
    --disable-whpx
    --enable-lzfse
    --enable-nettle
    --extra-cflags="${lzfse_inc_flag}"
    --extra-ldflags="${lzfse_lib_flag} -lhogweed -lnettle -lgmp"
  )

  echo "Configuring ChefKiss Inferno..."
  "${inferno_src}/configure" "${conf_args[@]}"

  echo "Compiling ChefKiss Inferno with ${jobs} parallel jobs..."
  ninja -C "${inferno_build}" -j"${jobs}"
  ninja -C "${inferno_build}" install

  # Copy built 64-bit binary to prefix/bin as qemu-system-applesoc, qemu-system-inferno, qemu-system-aarch64-inferno
  local bin_64=""
  if [[ -f "${inferno_stage}/bin/qemu-system-aarch64.exe" ]]; then
    bin_64="${inferno_stage}/bin/qemu-system-aarch64.exe"
  elif [[ -f "${inferno_stage}/bin/qemu-system-aarch64" ]]; then
    bin_64="${inferno_stage}/bin/qemu-system-aarch64"
  elif [[ -f "${inferno_stage}/qemu-system-aarch64.exe" ]]; then
    bin_64="${inferno_stage}/qemu-system-aarch64.exe"
  elif [[ -f "${inferno_stage}/qemu-system-aarch64" ]]; then
    bin_64="${inferno_stage}/qemu-system-aarch64"
  fi

  if [[ -n "${bin_64}" ]]; then
    local ext=""
    [[ "${bin_64}" == *.exe ]] && ext=".exe"
    cp -f "${bin_64}" "${prefix}/bin/qemu-system-applesoc${ext}"
    cp -f "${bin_64}" "${prefix}/bin/qemu-system-inferno${ext}"
    cp -f "${bin_64}" "${prefix}/bin/qemu-system-aarch64-inferno${ext}"
    echo "Successfully installed: ${prefix}/bin/qemu-system-applesoc${ext} (and aliases)"
  else
    echo "ERROR: Failed to find compiled aarch64 binary in ${inferno_stage}" >&2
    return 1
  fi

  # Copy built 32-bit binary if available
  local bin_32=""
  if [[ -f "${inferno_stage}/bin/qemu-system-arm.exe" ]]; then
    bin_32="${inferno_stage}/bin/qemu-system-arm.exe"
  elif [[ -f "${inferno_stage}/bin/qemu-system-arm" ]]; then
    bin_32="${inferno_stage}/bin/qemu-system-arm"
  elif [[ -f "${inferno_stage}/qemu-system-arm.exe" ]]; then
    bin_32="${inferno_stage}/qemu-system-arm.exe"
  elif [[ -f "${inferno_stage}/qemu-system-arm" ]]; then
    bin_32="${inferno_stage}/qemu-system-arm"
  fi

  if [[ -n "${bin_32}" ]]; then
    local ext32=""
    [[ "${bin_32}" == *.exe ]] && ext32=".exe"
    cp -f "${bin_32}" "${prefix}/bin/qemu-system-applesoc32${ext32}"
    cp -f "${bin_32}" "${prefix}/bin/qemu-system-arm-inferno${ext32}"
    echo "Successfully installed: ${prefix}/bin/qemu-system-applesoc32${ext32} (and aliases)"
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

  # On aarch64 / ARM64, patch *const i8 -> *const std::ffi::c_char in Rust crates
  if [[ "$(uname -m 2>/dev/null)" == "aarch64" || "$(uname -m 2>/dev/null)" == "arm64" ]]; then
    for rust_file in \
      "${reims_dir}/crates/reims-vgpu-vulkan/src/device.rs" \
      "${reims_dir}/crates/reims-vgpu/src/backend/vulkan/caps/push_descriptor.rs"; do
      if [[ -f "${rust_file}" ]]; then
        sed -i 's/\*const i8/\*const std::ffi::c_char/g' "${rust_file}"
      fi
    done
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

  # Copy built binary to prefix/bin as qemu-system-reimsvgpu / qemu-system-reims / qemu-system-reims3d / qemu-system-x86_64-reims
  local bin_x86=""
  if [[ -f "${reims_stage}/bin/qemu-system-x86_64.exe" ]]; then
    bin_x86="${reims_stage}/bin/qemu-system-x86_64.exe"
  elif [[ -f "${reims_stage}/bin/qemu-system-x86_64" ]]; then
    bin_x86="${reims_stage}/bin/qemu-system-x86_64"
  elif [[ -f "${reims_stage}/qemu-system-x86_64.exe" ]]; then
    bin_x86="${reims_stage}/qemu-system-x86_64.exe"
  elif [[ -f "${reims_stage}/qemu-system-x86_64" ]]; then
    bin_x86="${reims_stage}/qemu-system-x86_64"
  fi

  if [[ -n "${bin_x86}" ]]; then
    local ext=""
    [[ "${bin_x86}" == *.exe ]] && ext=".exe"
    cp -f "${bin_x86}" "${prefix}/bin/qemu-system-reimsvgpu${ext}"
    cp -f "${bin_x86}" "${prefix}/bin/qemu-system-reims${ext}"
    cp -f "${bin_x86}" "${prefix}/bin/qemu-system-reims3d${ext}"
    cp -f "${bin_x86}" "${prefix}/bin/qemu-system-x86_64-reims${ext}"
    echo "Successfully installed: ${prefix}/bin/qemu-system-reimsvgpu${ext} (and aliases)"
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
