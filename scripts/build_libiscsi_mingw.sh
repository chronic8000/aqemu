#!/usr/bin/env bash
# MSYS2 does not ship libiscsi. QEMU's iSCSI disk driver needs the
# pkg-config module libiscsi >= 1.9.0, so build upstream 1.20.3 into the
# active MinGW prefix (CLANGARM64 or UCRT64).
#
# Caller must set ROOT, PKG_CONFIG, and MSYSTEM.

LIBISCSI_TAG="1.20.3"

aqemu_build_libiscsi() {
	local prefix_unix cc host src patch
	case "${MSYSTEM:-}" in
		CLANGARM64)
			prefix_unix=/clangarm64
			cc="${CC:-clang}"
			host=aarch64-w64-mingw32
			;;
		UCRT64)
			prefix_unix=/ucrt64
			cc="${CC:-gcc}"
			host=x86_64-w64-mingw32
			;;
		MINGW64)
			prefix_unix=/mingw64
			cc="${CC:-gcc}"
			host=x86_64-w64-mingw32
			;;
		*)
			echo "ERROR: libiscsi must be built from a CLANGARM64 or UCRT64 shell (MSYSTEM=${MSYSTEM:-unset})." >&2
			return 1
			;;
	esac

	if "${PKG_CONFIG}" --exists 'libiscsi >= 1.9.0' 2>/dev/null; then
		echo "OK: libiscsi ($("${PKG_CONFIG}" --modversion libiscsi 2>/dev/null || true))"
		return 0
	fi

	if ! command -v autoconf >/dev/null 2>&1 || ! command -v automake >/dev/null 2>&1 || ! command -v patch >/dev/null 2>&1; then
		echo "Installing autoconf, automake, libtool, make, and patch"
		pacman -S --needed --noconfirm autoconf automake libtool make patch
	fi

	src="${ROOT}/third_party/libiscsi"
	patch="${ROOT}/scripts/patches/libiscsi-mingw-threads.patch"
	if [[ ! -f "${src}/configure.ac" ]]; then
		rm -rf "${src}"
		echo "Cloning libiscsi ${LIBISCSI_TAG}"
		git clone --depth 1 --branch "${LIBISCSI_TAG}" https://github.com/sahlberg/libiscsi.git "${src}"
	fi
	if ! grep -q 'MinGW has winpthreads' "${src}/include/iscsi-multithreading.h"; then
		echo "Applying MinGW thread patch"
		patch -d "${src}" -p1 --binary < "${patch}" || return 1
	fi

	echo "Building libiscsi ${LIBISCSI_TAG} for ${MSYSTEM} into ${prefix_unix}"
	# One tree is shared by ARM64 and x64. Drop the other compiler's objects first.
	if [[ -f "${src}/Makefile" ]]; then
		make -C "${src}" distclean || true
	fi
	(
		cd "${src}"
		./autogen.sh
	) || return 1
	(
		cd "${src}"
		./configure --host="${host}" --prefix="${prefix_unix}" --disable-werror \
			--disable-test-tool --disable-tests --disable-examples \
			CC="${cc}"
	) || return 1
	make -C "${src}" -j"$(nproc 2>/dev/null || echo 4)" || return 1
	make -C "${src}" install || return 1

	if ! "${PKG_CONFIG}" --exists 'libiscsi >= 1.9.0' 2>/dev/null; then
		echo "ERROR: libiscsi installed but pkg-config still cannot see libiscsi >= 1.9.0." >&2
		echo "PKG_CONFIG=${PKG_CONFIG} PKG_CONFIG_PATH=${PKG_CONFIG_PATH:-}" >&2
		return 1
	fi
	echo "OK: libiscsi ($("${PKG_CONFIG}" --modversion libiscsi 2>/dev/null || true))"
}
