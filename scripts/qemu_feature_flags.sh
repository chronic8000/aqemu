#!/usr/bin/env bash
# Shared QEMU feature flags for AQEMU bundles (every softmmu target gets these).
# Source from build_qemu_*.sh after PKG_CONFIG is set.
#
# Required for Store/portable: slirp (user networking), spice, vnc.
# Strongly desired: curl, libusb, usbredir, gnutls, zstd, fdt.

aqemu_qemu_require_pkg() {
	local mod="$1"
	local hint="$2"
	if ! "$PKG_CONFIG" --exists "$mod" 2>/dev/null; then
		echo "ERROR: pkg-config module '$mod' not found." >&2
		echo "Install: $hint" >&2
		return 1
	fi
	echo "OK: $mod $($PKG_CONFIG --modversion "$mod" 2>/dev/null || true)"
}

# Populate global array AQEMU_QEMU_EXTRA_CONFIGURE with --enable-* flags.
# Call as: aqemu_qemu_feature_flags   then use "${AQEMU_QEMU_EXTRA_CONFIGURE[@]}"
# Windows named pipes must listen without waiting, or QEMU never starts
# the guest. Apply third_party/patches before configure.
aqemu_apply_qemu_patches() {
	local src="${ROOT}/third_party/qemu"
	local dir="${ROOT}/third_party/patches"
	local p norm
	[[ -d "${dir}" ]] || return 0
	[[ -f "${src}/meson.build" || -f "${src}/configure" ]] || return 0
	shopt -s nullglob
	for p in "${dir}"/*.patch; do
		echo "Applying QEMU patch $(basename "${p}")"
		norm="$(mktemp)"
		tr -d '\r' < "${p}" > "${norm}"
		if git -C "${src}" apply --check "${norm}" >/dev/null 2>&1; then
			git -C "${src}" apply "${norm}"
		elif git -C "${src}" apply --reverse --check "${norm}" >/dev/null 2>&1; then
			echo "QEMU patch $(basename "${p}") already applied"
		else
			echo "ERROR: QEMU patch $(basename "${p}") did not apply" >&2
			git -C "${src}" apply --check "${norm}" >&2 || true
			rm -f "${norm}"
			return 1
		fi
		rm -f "${norm}"
	done
}

aqemu_qemu_feature_flags() {
	aqemu_apply_qemu_patches
	# Upstream QEMU is used as checked out, plus third_party/patches.
	AQEMU_QEMU_EXTRA_CONFIGURE=(
		--enable-vnc
		--enable-slirp
		--disable-docs
		--disable-guest-agent
		--disable-werror
	)

	# Hard requirements — fail the build rather than ship a half-broken emulator.
	# Note: pkg-config module may be "slirp" or "libslirp" depending on distro/MSYS2.
	if "$PKG_CONFIG" --exists slirp 2>/dev/null; then
		echo "OK: slirp ($($PKG_CONFIG --modversion slirp 2>/dev/null || true))"
	elif "$PKG_CONFIG" --exists libslirp 2>/dev/null; then
		echo "OK: libslirp ($($PKG_CONFIG --modversion libslirp 2>/dev/null || true))"
	else
		echo "ERROR: pkg-config module 'slirp' / 'libslirp' not found." >&2
		echo "Install: pacman -S mingw-w64-x86_64-libslirp (Windows) / pacman -S mingw-w64-clang-aarch64-libslirp (WoA) / apt install libslirp-dev (Linux)" >&2
		return 1
	fi

	# SPICE server: enable when available.
	if "$PKG_CONFIG" --exists spice-server 2>/dev/null; then
		AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-spice)
		echo "OK: spice-server ($($PKG_CONFIG --modversion spice-server 2>/dev/null || true))"
	else
		echo "WARN: spice-server pkg-config module not found — building without SPICE server (VNC embedded display remains available)"
	fi

	# Optional but enable when present (every target benefits).
	# Map QEMU --enable-* name -> pkg-config module name(s).
	_aqemu_try_enable() {
		local flag="$1"; shift
		local pc
		for pc in "$@"; do
			if "$PKG_CONFIG" --exists "$pc" 2>/dev/null; then
				AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-"$flag")
				echo "OK: $flag ($pc)"
				return 0
			fi
		done
		echo "WARN: $flag not found (tried: $*) — skipped"
		return 1
	}
	_aqemu_try_enable curl libcurl
	_aqemu_try_enable libusb libusb-1.0
	# QEMU configure flag is usb-redir (with hyphen)
	if "$PKG_CONFIG" --exists libusbredirparser-0.5 2>/dev/null || \
	   "$PKG_CONFIG" --exists libusbredirhost 2>/dev/null; then
		AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-usb-redir)
		echo "OK: usb-redir"
	else
		echo "WARN: usbredir not found — skipped"
	fi
	_aqemu_try_enable gnutls gnutls
	_aqemu_try_enable zstd libzstd

	# Platform accelerators when available
	case "$(uname -s 2>/dev/null || echo unknown)" in
		MINGW*|MSYS*|CYGWIN*|Windows_NT)
			# QEMU 11 compiles WHPX for target/i386 and target/arm. Windows on ARM
			# must pass --enable-whpx or an ARM guest stays on TCG.
			AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-whpx)
			echo "OK: whpx (Windows Hypervisor Platform, x64 and ARM64 hosts)"
			;;
		Linux)
			AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-kvm)
			echo "OK: kvm"
			;;
		Darwin)
			AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-hvf)
			echo "OK: hvf"
			;;
	esac

	# Headless Store/portable profile: no SDL/GTK chrome (AQEMU embeds via SPICE/VNC).
	# Still allow spice-app as a QEMU-side option.
	if [[ "${AQEMU_QEMU_ENABLE_SDL:-0}" != "1" ]]; then
		AQEMU_QEMU_EXTRA_CONFIGURE+=(--disable-sdl --disable-gtk)
		echo "OK: headless display profile (sdl/gtk off — AQEMU embeds SPICE/VNC)"
	fi

	echo "QEMU configure extras: ${AQEMU_QEMU_EXTRA_CONFIGURE[*]}"
}

# After install: refuse a broken bundle.
# Usage: aqemu_qemu_verify_install <prefix> [target]
aqemu_qemu_verify_install() {
	local prefix="$1"
	local target="${2:-}"
	local bin=""

	# If a specific target was given, probe for that emulator first
	if [[ -n "$target" ]]; then
		for cand in \
			"${prefix}/qemu-system-${target}.exe" \
			"${prefix}/bin/qemu-system-${target}.exe" \
			"${prefix}/qemu-system-${target}" \
			"${prefix}/bin/qemu-system-${target}"
		do
			if [[ -x "$cand" ]]; then
				bin="$cand"
				break
			fi
		done
	fi

	# Fallback to x86_64 or any installed qemu-system-*
	if [[ -z "$bin" ]]; then
		for cand in \
			"${prefix}/qemu-system-x86_64.exe" \
			"${prefix}/bin/qemu-system-x86_64.exe" \
			"${prefix}/qemu-system-x86_64" \
			"${prefix}/bin/qemu-system-x86_64"
		do
			if [[ -x "$cand" ]]; then
				bin="$cand"
				break
			fi
		done
	fi

	if [[ -z "$bin" ]]; then
		bin="$(find "${prefix}" -maxdepth 2 -name 'qemu-system-*' -type f 2>/dev/null | head -1 || true)"
	fi

	if [[ -z "$bin" || ! -x "$bin" ]]; then
		echo "ERROR: no qemu-system-* binary found under ${prefix}" >&2
		return 1
	fi

	local help
	help="$("$bin" -netdev help 2>&1 || true)"
	if ! grep -qE '(^|[[:space:]])user($|[[:space:]])' <<<"$help"; then
		echo "ERROR: '$bin' is missing netdev 'user' (libslirp). Every AQEMU guest needs this." >&2
		echo "$help" >&2
		return 1
	fi
	echo "VERIFY OK: $bin has -netdev user"
	case "$(uname -s 2>/dev/null || echo unknown)" in
		MINGW*|MSYS*|CYGWIN*|Windows_NT)
			local host_arch expect accel_bin accel_help
			host_arch="$(uname -m 2>/dev/null || echo unknown)"
			if [[ "${MSYSTEM:-}" == "CLANGARM64" || "$host_arch" == "aarch64" ]]; then
				expect="aarch64"
			else
				expect="x86_64"
			fi
			accel_bin=""
			for cand in \
				"${prefix}/bin/qemu-system-${expect}.exe" \
				"${prefix}/qemu-system-${expect}.exe" \
				"${prefix}/bin/qemu-system-${expect}" \
				"${prefix}/qemu-system-${expect}"
			do
				if [[ -x "$cand" ]]; then
					accel_bin="$cand"
					break
				fi
			done
			if [[ -n "$accel_bin" ]]; then
				accel_help="$("$accel_bin" -accel help 2>&1 || true)"
				if ! grep -q whpx <<<"$accel_help"; then
					echo "ERROR: '$accel_bin' was built without WHPX." >&2
					echo "$accel_help" >&2
					return 1
				fi
				echo "VERIFY OK: $accel_bin lists accelerator whpx"
			fi
			;;
	esac
	local n=0
	n="$(find "${prefix}" -maxdepth 2 -name 'qemu-system-*' -type f 2>/dev/null | wc -l | tr -d ' ')"
	echo "VERIFY: qemu-system-* count = ${n}"
	return 0
}
