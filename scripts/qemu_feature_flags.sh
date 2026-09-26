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
aqemu_qemu_feature_flags() {
	AQEMU_QEMU_EXTRA_CONFIGURE=(
		--enable-vnc
		--enable-slirp
		--disable-docs
		--disable-guest-agent
		--disable-werror
	)

	# Hard requirements — fail the build rather than ship a half-broken emulator.
	# Note: pkg-config module is "slirp" (not "libslirp") on MSYS2/Fedora.
	aqemu_qemu_require_pkg slirp \
		"pacman -S mingw-w64-x86_64-libslirp   (Windows)  /  apt install libslirp-dev (Linux)"
	aqemu_qemu_require_pkg spice-server \
		"pacman -S mingw-w64-x86_64-spice       (Windows)  /  apt install libspice-server-dev (Linux)"
	AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-spice)

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
			AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-whpx)
			echo "OK: whpx (Windows Hypervisor Platform)"
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
aqemu_qemu_verify_install() {
	local prefix="$1"
	local bin=""
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
	if [[ -z "$bin" ]]; then
		echo "ERROR: qemu-system-x86_64 not found under ${prefix}" >&2
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
	local n=0
	n="$(find "${prefix}" -maxdepth 2 -name 'qemu-system-*' -type f 2>/dev/null | wc -l | tr -d ' ')"
	echo "VERIFY: qemu-system-* count = ${n}"
	return 0
}
