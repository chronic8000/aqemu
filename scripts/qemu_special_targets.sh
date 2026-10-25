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
_aqemu_script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=msys_prefix.sh
source "${_aqemu_script_dir}/msys_prefix.sh"
export AQEMU_MSYS_ROOT="$(aqemu_resolve_msys_root)"

# MSYS git-remote-https aborts when a MinGW bin directory is first on PATH:
# Windows loads that directory's OpenSSL DLLs into the MSYS helper.
# GitHub Actions also injects GIT_CONFIG_* values that the helper rejects.
aqemu_git_clone() {
  local url="$1"
  local dest="$2"
  echo "Cloning ${url}"
  (
    local i
    unset GIT_CONFIG_COUNT GIT_CONFIG_PARAMETERS GIT_CONFIG_GLOBAL GIT_CONFIG_SYSTEM || true
    for i in 0 1 2 3 4 5 6 7; do
      unset "GIT_CONFIG_KEY_${i}" "GIT_CONFIG_VALUE_${i}" || true
    done
    export PATH="/usr/bin:${PATH}"
    git clone --depth 1 "${url}" "${dest}"
  )
}

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
    export PATH="${AQEMU_MSYS_ROOT}/clangarm64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/ucrt64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/ucrt64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "MINGW64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/mingw64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
  fi

  if command -v meson >/dev/null 2>&1; then
    local cur_ver
    cur_ver="$(meson --version 2>/dev/null || echo 0.0.0)"
    if python3 -c "
import sys
v = '${cur_ver}'.split('.')
try:
    maj, min = int(v[0]), int(v[1])
    sys.exit(0 if (maj > 1 or (maj == 1 and min >= 5)) else 1)
except Exception:
    sys.exit(1)
" 2>/dev/null; then
      echo "Found suitable meson ${cur_ver}: $(command -v meson)"
      return 0
    else
      echo "Found meson ${cur_ver}, but >= 1.5.0 required for Inferno. Upgrading..."
    fi
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

  # Upgrade or install via pip if meson is missing or outdated
  if command -v pip3 >/dev/null 2>&1; then
    echo "Installing/upgrading meson via pip3..."
    pip3 install --break-system-packages --upgrade "meson>=1.5.0" 2>/dev/null || \
    sudo pip3 install --break-system-packages --upgrade "meson>=1.5.0" 2>/dev/null || true
  elif command -v pip >/dev/null 2>&1; then
    echo "Installing/upgrading meson via pip..."
    pip install --break-system-packages --upgrade "meson>=1.5.0" 2>/dev/null || true
  fi

  if command -v meson >/dev/null 2>&1; then
    echo "Installed meson: $(command -v meson) ($(meson --version))"
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
    sudo apt-get update && sudo apt-get install -y meson python3-pip || true
    sudo pip3 install --break-system-packages --upgrade "meson>=1.5.0" 2>/dev/null || true
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

# True when this compiler's objdump can read the static archive. An archive
# built by the other MinGW toolchain (ARM64 vs x86_64) is the wrong format.
aqemu_cc_reads_archive() {
  local cc="$1"
  local archive="$2"
  local objdump
  [[ -f "${archive}" ]] || return 1
  objdump="$("${cc}" -print-prog-name=objdump 2>/dev/null || true)"
  if [[ -z "${objdump}" || "${objdump}" == "objdump" ]]; then
    objdump="objdump"
  fi
  local dump host
  dump="$("${objdump}" -f "${archive}" 2>/dev/null || true)"
  [[ -n "${dump}" ]] || return 1
  host="$("${cc}" -dumpmachine 2>/dev/null || true)"
  if [[ "${host}" == aarch64* || "${host}" == arm64* ]]; then
    [[ "${dump}" == *aarch64* ]]
  elif [[ "${host}" == *x86_64* || "${host}" == *amd64* ]]; then
    [[ "${dump}" == *x86-64* || "${dump}" == *x86_64* ]]
  fi
}

# Meson on an ARM64 Windows PC records the machine CPU even when the
# compiler is the x86_64 MinGW toolchain. A blank --cross-prefix keeps the
# unprefixed gcc/ar names and still writes the requested host CPU.
aqemu_mingw_host_cpu_args() {
  case "${MSYSTEM:-}" in
    UCRT64|MINGW64|CLANG64)
      printf '%s\n' --cross-prefix= --cpu=x86_64
      ;;
    CLANGARM64)
      printf '%s\n' --cross-prefix= --cpu=aarch64
      ;;
  esac
}

aqemu_ensure_lzfse() {
  local root="$1"
  local lzfse_dir="${root}/third_party/lzfse"
  local lzfse_build="${root}/third_party/lzfse-build"
  local lzfse_install="${root}/third_party/lzfse-install"

  # Ensure common MSYS2 bin paths are on PATH
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/clangarm64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/ucrt64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/ucrt64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "MINGW64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/mingw64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
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
  elif [[ -d "${AQEMU_MSYS_ROOT}/clangarm64/include" ]]; then
    msys_prefix="${AQEMU_MSYS_ROOT}/clangarm64"
  fi

  local c_compiler="${CC:-}"
  if [[ -z "${c_compiler}" ]]; then
    if command -v gcc >/dev/null 2>&1; then
      c_compiler="gcc"
    elif command -v clang >/dev/null 2>&1; then
      c_compiler="clang"
    else
      c_compiler="cc"
    fi
  fi

  # A header or archive from another toolchain (clangarm64 vs ucrt64) still
  # "exists", and an empty -llzfse link succeeds without pulling objects in.
  # Inferno's real link then fails with "could not link liblzfse".
  local found_h=0
  local lzfse_archive="${lzfse_install}/lib/liblzfse.a"
  if aqemu_cc_reads_archive "${c_compiler}" "${lzfse_archive}"; then
    for inc_dir in \
      "${lzfse_install}/include" \
      "${msys_prefix}/include" \
      /usr/include \
      /usr/local/include; do
      if [[ -n "${inc_dir}" && -f "${inc_dir}/lzfse.h" ]]; then
        echo "Found lzfse.h in ${inc_dir} for $(${c_compiler} -dumpmachine 2>/dev/null || echo "${c_compiler}")"
        found_h=1
        break
      fi
    done
  else
    echo "liblzfse is missing or was built for a different compiler; rebuilding"
    rm -f "${lzfse_archive}"
  fi

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
    if [[ ! -d "${lzfse_dir}" || ! -f "${lzfse_dir}/src/lzfse_decode.c" ]]; then
      rm -rf "${lzfse_dir}"
      aqemu_git_clone "https://github.com/lzfse/lzfse.git" "${lzfse_dir}" || return 1
    fi
    if [[ ! -f "${lzfse_dir}/src/lzfse_decode.c" ]]; then
      echo "ERROR: lzfse source is missing after clone: ${lzfse_dir}" >&2
      return 1
    fi

    mkdir -p "${lzfse_build}" "${lzfse_install}/include" "${lzfse_install}/lib" "${lzfse_install}/lib/pkgconfig"

    local ar_tool
    ar_tool="$("${c_compiler}" -print-prog-name=ar 2>/dev/null || true)"
    if [[ -z "${ar_tool}" ]]; then
      ar_tool="ar"
    fi
    if command -v llvm-ar >/dev/null 2>&1; then
      ar_tool="llvm-ar"
    elif command -v ar >/dev/null 2>&1; then
      ar_tool="ar"
    fi

    echo "Compiling lzfse static library using ${c_compiler}..."
    (
      cd "${lzfse_dir}" || exit 1
      "${c_compiler}" -O3 -fPIC -c \
        src/lzfse_decode.c src/lzfse_decode_base.c src/lzfse_encode.c \
        src/lzfse_encode_base.c src/lzfse_fse.c src/lzvn_decode_base.c \
        src/lzvn_encode_base.c
      "${ar_tool}" rcs "${lzfse_install}/lib/liblzfse.a" *.o
      cp -f src/lzfse.h "${lzfse_install}/include/"
      rm -f *.o
    ) || return 1

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
    export PATH="${AQEMU_MSYS_ROOT}/clangarm64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/clangarm64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "UCRT64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/ucrt64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/ucrt64/bin:/usr/bin:${PATH}"
  elif [[ "${MSYSTEM:-}" == "MINGW64" ]]; then
    export PATH="${AQEMU_MSYS_ROOT}/mingw64/bin:${AQEMU_MSYS_ROOT}/usr/bin:/mingw64/bin:/usr/bin:${PATH}"
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
      sudo apt-get update && sudo apt-get install -y nettle-dev libnettle-dev libhogweed-dev libgmp-dev || true
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
  # Keep each host toolchain's tree. An x86_64 configure left in the ARM64
  # directory makes the next ninja regenerate the wrong compiler.
  local inferno_host="x86_64"
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    inferno_host="aarch64"
  fi
  local inferno_build="${root}/third_party/inferno-build-${inferno_host}"
  local inferno_stage="${root}/third_party/inferno-stage-${inferno_host}"
  local lzfse_install="${root}/third_party/lzfse-install"

  if [[ ! -d "${inferno_src}" || ! -f "${inferno_src}/configure" ]]; then
    echo "Cloning ChefKiss Inferno repository (Apple Silicon / iOS)..."
    aqemu_git_clone "https://github.com/ChefKissInc/Inferno.git" "${inferno_src}" || return 1
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

  # Relax nettle >= 3.10 requirement to >= 3.8 so Ubuntu 24.04 (nettle 3.9.1) succeeds
  if [[ -f "${inferno_src}/meson.build" ]]; then
    python3 -c "
p = '${inferno_src}/meson.build'
content = open(p).read()
if \"version: '>=3.10'\" in content:
    print('Patching nettle version requirement in Inferno meson.build from >=3.10 to >=3.8...')
    content = content.replace(\"version: '>=3.10'\", \"version: '>=3.8'\")
    open(p, 'w').write(content)
" || true
  fi

  # A previous ARM64 configure in this directory makes ninja regenerate
  # that tree after an x86_64 configure failure, which is the symlink and
  # "no aarch64 binary" errors.
  rm -rf "${inferno_stage}" "${inferno_build}"
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
  local inferno_cflags="${lzfse_inc_flag}"
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    inferno_cflags="${inferno_cflags} -fno-vectorize -fno-slp-vectorize"
  fi
  local host_arg
  while IFS= read -r host_arg; do
    [[ -n "${host_arg}" ]] && conf_args+=("${host_arg}")
  done < <(aqemu_mingw_host_cpu_args)

  conf_args+=(
    --disable-whpx
    --enable-lzfse
    --enable-nettle
    --extra-cflags="${inferno_cflags}"
    --extra-ldflags="${lzfse_lib_flag} -lhogweed -lnettle -lgmp"
  )

  # clangarm64 has no windmc. Inferno's Windows guest agent still runs
  # windmc while Meson is loading, and the ucrt64 copy is a tool, not a
  # target binary.
  if [[ "${MSYSTEM:-}" == "CLANGARM64" && -x /ucrt64/bin/windmc.exe ]]; then
    export PATH="${PATH}:/ucrt64/bin"
  fi

  echo "Configuring ChefKiss Inferno..."
  "${inferno_src}/configure" "${conf_args[@]}" || return 1

  echo "Compiling ChefKiss Inferno with ${jobs} parallel jobs..."
  ninja -C "${inferno_build}" -j"${jobs}"
  ninja -C "${inferno_build}" install

  # The guest binary is qemu-system-aarch64 even when the host is x86_64.
  # Install copies it into the stage prefix; a Windows install that only
  # filled the build tree is still a successful compile.
  local bin_64=""
  local candidate
  for candidate in \
    "${inferno_stage}/bin/qemu-system-aarch64.exe" \
    "${inferno_stage}/bin/qemu-system-aarch64" \
    "${inferno_stage}/qemu-system-aarch64.exe" \
    "${inferno_stage}/qemu-system-aarch64" \
    "${inferno_build}/qemu-system-aarch64.exe" \
    "${inferno_build}/qemu-system-aarch64" \
    "${inferno_build}/aarch64-softmmu/qemu-system-aarch64.exe" \
    "${inferno_build}/aarch64-softmmu/qemu-system-aarch64"; do
    if [[ -f "${candidate}" ]]; then
      bin_64="${candidate}"
      break
    fi
  done
  if [[ -z "${bin_64}" ]]; then
    bin_64="$(find "${inferno_stage}" "${inferno_build}" \( -name 'qemu-system-aarch64.exe' -o -name 'qemu-system-aarch64' \) -type f 2>/dev/null | head -1 || true)"
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

# Give Windows the queue-family helper that metal2vulkan defines only inside
# its Unix dma-buf module. The calls in texture rebind stay in the shared code.
aqemu_patch_metal2vulkan_windows() {
  local cargo_home="${CARGO_HOME:-}"
  if [[ -z "${cargo_home}" ]]; then
    if [[ -n "${USERPROFILE:-}" ]]; then
      cargo_home="${USERPROFILE}/.cargo"
    else
      cargo_home="${HOME}/.cargo"
    fi
  fi
  local py="python3"
  if ! command -v python3 >/dev/null 2>&1; then
    py="python"
  fi
  local checkout texture
  shopt -s nullglob
  for checkout in "${cargo_home}/git/checkouts"/metal2vulkan-*/720ef1c; do
    texture="${checkout}/crates/metal2vulkan-runtime/src/texture.rs"
    [[ -f "${texture}" ]] || continue
    "${py}" -c '
import pathlib, sys
p = pathlib.Path(sys.argv[1])
text = p.read_text(encoding="utf-8")
if "fn foreign_acquirer" in text and "not(unix)" in text:
    sys.exit(0)
needle = "impl Device {\n"
at = text.find(needle)
if at < 0:
    sys.exit("metal2vulkan texture.rs has no impl Device")
insert = """impl Device {
    // AQEMU_WIN_FOREIGN_ACQUIRER: Unix dma_buf.rs is not built on Windows.
    #[cfg(not(unix))]
    pub(super) fn foreign_acquirer(&self) -> u32 {
        match &self.shared().queue_families[..] {
            [family] => *family,
            _ => vk::QUEUE_FAMILY_IGNORED,
        }
    }

"""
text = text[:at] + insert + text[at + len(needle):]
p.write_text(text, encoding="utf-8", newline="\n")
print("Patched", p)
' "${texture}"
  done
  shopt -u nullglob
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
  local reims_host="x86_64"
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    reims_host="aarch64"
  fi
  local reims_build="${root}/third_party/reims-build-${reims_host}"
  local reims_stage="${root}/third_party/reims-stage-${reims_host}"

  local reims_src=""
  if [[ -f "${reims_dir}/vendor/qemu/configure" ]]; then
    reims_src="${reims_dir}/vendor/qemu"
  elif [[ -f "${reims_dir}/configure" ]]; then
    reims_src="${reims_dir}"
  else
    echo "Cloning steelbrain reims-vgpu repository..."
    aqemu_git_clone "https://github.com/steelbrain/reims-vgpu.git" "${reims_dir}" || true
    if [[ -d "${reims_dir}" ]]; then
      git -C "${reims_dir}" submodule update --init --depth 1 vendor/qemu || true
    fi
    if [[ -f "${reims_dir}/vendor/qemu/configure" ]]; then
      reims_src="${reims_dir}/vendor/qemu"
    elif [[ -f "${reims_dir}/configure" ]]; then
      reims_src="${reims_dir}"
    fi
  fi

  if [[ -z "${reims_src}" || ! -f "${reims_src}/configure" ]]; then
    echo "WARN: steelbrain reims-vgpu sources not available — skipping Reims build"
    return 0
  fi

  # MSYS uname stays x86_64 under Prism. The ARM64 toolchain is CLANGARM64.
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    for rust_file in \
      "${reims_dir}/crates/reims-vgpu-vulkan/src/device.rs" \
      "${reims_dir}/crates/reims-vgpu/src/backend/vulkan/caps/push_descriptor.rs"; do
      if [[ -f "${rust_file}" ]]; then
        sed -i 's/\*const i8/\*const std::ffi::c_char/g' "${rust_file}"
      fi
    done
  fi

  rm -rf "${reims_stage}" "${reims_build}"
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

  # vendor/qemu dropped -Dreims_vgpu_backend. The Vulkan or Metal choice is
  # the library `cargo xtask dist --platform` builds; QEMU then takes that
  # tree through pkg-config and --enable-pvg.
  local reims_platform="${REIMS_VGPU_BACKEND}"
  # A separate tree per host. The x86_64 archive must not be replaced by the
  # aarch64 one, and pkg-config must be handed the archive this compiler can link.
  local reims_dist="${reims_dir}/target/dist-${reims_platform}-${reims_host}"
  # metal2vulkan keeps Device::foreign_acquirer in its Unix-only dma-buf module.
  # Windows still type-checks the rebind calls, so the checkout needs the same
  # method outside that module. Re-applied on every build; cargo checkouts are
  # not part of this tree.
  aqemu_patch_metal2vulkan_windows

  echo "Building reims-vgpu library (platform ${reims_platform})..."
  # cargo xtask dist runs `make` and `clang` as Windows programs. MSYS only
  # has mingw32-make, and the EFI ROM's clang/lld live in the clangarm64
  # toolchain even when QEMU itself is built with ucrt64 gcc.
  local reims_tool_dir="${root}/third_party/.bin"
  mkdir -p "${reims_tool_dir}"
  local host_bin="/ucrt64/bin"
  local host_cc="gcc.exe"
  local host_cxx="g++.exe"
  if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
    host_bin="/clangarm64/bin"
    host_cc="clang.exe"
    host_cxx="clang++.exe"
    if ! pacman -Q mingw-w64-clang-aarch64-rust >/dev/null 2>&1; then
      pacman -S --needed --noconfirm mingw-w64-clang-aarch64-rust
    fi
  fi
  # third_party/.bin/make.exe may be the other toolchain's make. An ARM64
  # xtask that starts an x64 make, with clangarm64's DLLs first on PATH, dies
  # with STATUS_INVALID_IMAGE_FORMAT. Use this host's mingw32-make.
  local host_make="${host_bin}/mingw32-make.exe"
  if [[ -x "${host_make}" ]]; then
    cp -f "${host_make}" "${reims_tool_dir}/make.exe"
  fi
  if command -v cygpath >/dev/null 2>&1; then
    export PATH="$(cygpath -m "${host_bin}"):$(cygpath -m "${reims_tool_dir}"):$(cygpath -m /usr/bin):${PATH}"
    export MAKE="$(cygpath -m "${host_make}")"
    export CC="$(cygpath -m "${host_bin}/${host_cc}")"
    export CXX="$(cygpath -m "${host_bin}/${host_cxx}")"
    export EFI_CC="$(cygpath -m /clangarm64/bin/clang.exe)"
    export LD="$(cygpath -m /clangarm64/bin/lld-link.exe)"
    export OBJCOPY="$(cygpath -m /clangarm64/bin/llvm-objcopy.exe)"
    # SDL's CMake probe nests past MAX_PATH when Cargo's target dir is the
    # long sandbox path. A short directory keeps the aarch64 build writable.
    if [[ "${MSYSTEM:-}" == "CLANGARM64" ]]; then
      export CARGO_TARGET_DIR="$(cygpath -m "${root}/../rvg-a64")"
      mkdir -p "${CARGO_TARGET_DIR}"
      # The guest pointer code emits ldsetal. Windows's aarch64 baseline
      # does not enable LSE; this host's Snapdragon does.
      export RUSTFLAGS="${RUSTFLAGS:-} -C target-feature=+lse,+rcpc"
    fi
    if [[ -n "${CARGO_TARGET_DIR:-}" ]]; then
      rm -rf "${CARGO_TARGET_DIR}/release/build"/sdl3-sys-*
    fi
  fi
  (
    cd "${reims_dir}"
    cargo xtask dist --platform "${reims_platform}" --out "${reims_dist}"
  ) || return 1
  # ucrt64 pkg-config.exe is a Windows program. It does not search MSYS /c/... paths,
  # and it splits PKG_CONFIG_PATH on ';' because ':' is the drive separator.
  local reims_pc="${reims_dist}/lib/pkgconfig"
  local pc_sep=":"
  if command -v cygpath >/dev/null 2>&1; then
    reims_pc="$(cygpath -m "${reims_pc}")"
    pc_sep=";"
  fi
  export PKG_CONFIG_PATH="${reims_pc}${pc_sep}${PKG_CONFIG_PATH:-}"

  local conf_args=(
    --prefix="${reims_stage}"
    --target-list="x86_64-softmmu"
    --enable-pvg
  )
  local host_arg
  while IFS= read -r host_arg; do
    [[ -n "${host_arg}" ]] && conf_args+=("${host_arg}")
  done < <(aqemu_mingw_host_cpu_args)

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
  if ! "${reims_src}/configure" "${conf_args[@]}"; then
    echo "WARN: steelbrain Reims vGPU configure failed — skipping optional Reims target"
    return 1
  fi

  echo "Compiling steelbrain Reims vGPU with ${jobs} parallel jobs..."
  if ! ninja -C "${reims_build}" -j"${jobs}"; then
    echo "WARN: steelbrain Reims vGPU compilation failed — skipping optional Reims target"
    return 1
  fi
  ninja -C "${reims_build}" install || true

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
