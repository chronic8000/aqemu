# Resolve the MSYS2 root from the active shell.
# MSYSTEM_PREFIX looks like /ucrt64 or D:/msys64/ucrt64. Inside MSYS2, cygpath -m /
# is the install root. This does not assume a drive letter.

aqemu_resolve_msys_root() {
	if [[ -n "${MSYSTEM_PREFIX:-}" ]]; then
		local parent
		parent="$(dirname "${MSYSTEM_PREFIX}")"
		if [[ -d "${parent}/usr/bin" || -d "${parent}/usr" ]]; then
			printf '%s\n' "${parent}"
			return 0
		fi
	fi
	if [[ -n "${MSYS2_ROOT:-}" && -d "${MSYS2_ROOT}" ]]; then
		printf '%s\n' "${MSYS2_ROOT}"
		return 0
	fi
	if command -v cygpath >/dev/null 2>&1 && [[ -x /usr/bin/bash ]]; then
		cygpath -m /
		return 0
	fi
	return 1
}
