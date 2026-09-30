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
	# Upstream QEMU is a submodule. AQEMU patches (device selection, and any
	# later files in third_party/patches) must land before configure, on every host.
	local _aqemu_root
	_aqemu_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
	if [[ -f "${_aqemu_root}/third_party/qemu/meson.build" || -f "${_aqemu_root}/third_party/qemu/configure" ]]; then
		bash "${_aqemu_root}/scripts/qemu_apply_patches.sh"
	fi

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
			if [[ "${MSYSTEM:-}" == "CLANGARM64" || "$(uname -m 2>/dev/null)" == "aarch64" ]]; then
				echo "NOTE: WHPX hypervisor is x86_64-only in QEMU — skipped on Windows on ARM (TCG used)"
			else
				AQEMU_QEMU_EXTRA_CONFIGURE+=(--enable-whpx)
				echo "OK: whpx (Windows Hypervisor Platform)"
			fi
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
	local n=0
	n="$(find "${prefix}" -maxdepth 2 -name 'qemu-system-*' -type f 2>/dev/null | wc -l | tr -d ' ')"
	echo "VERIFY: qemu-system-* count = ${n}"
	return 0
}
