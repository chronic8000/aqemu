# Resolve the MSYS2 root. Prefer the prefix already in the environment.
# MSYSTEM_PREFIX looks like D:/msys64/ucrt64. MSYS2_ROOT is the root itself.
# C:/msys64 is only the last resort.

aqemu_resolve_msys_root() {
	if [[ -n "${MSYSTEM_PREFIX:-}" ]]; then
		local parent
		parent="$(dirname "${MSYSTEM_PREFIX}")"
		if [[ -d "${parent}" ]]; then
			printf '%s\n' "${parent}"
			return 0
		fi
	fi
	if [[ -n "${MSYS2_ROOT:-}" && -d "${MSYS2_ROOT}" ]]; then
		printf '%s\n' "${MSYS2_ROOT}"
		return 0
	fi
	printf '%s\n' "C:/msys64"
}
