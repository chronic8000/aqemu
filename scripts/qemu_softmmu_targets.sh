#!/usr/bin/env bash
# Emit a comma-separated list of EVERY QEMU softmmu target (no linux-user/bsd-user).
# Usage: TARGETS="$(. scripts/qemu_softmmu_targets.sh)"  or  source + qemu_softmmu_target_list
qemu_softmmu_target_list() {
	local qemu_src="${1:?qemu source tree}"
	local targets_dir="${qemu_src}/configs/targets"
	if [[ ! -d "${targets_dir}" ]]; then
		echo "ERROR: missing ${targets_dir}" >&2
		return 1
	fi
	local list=""
	local f base
	for f in "${targets_dir}"/*-softmmu.mak; do
		[[ -f "$f" ]] || continue
		base="$(basename "$f" .mak)"
		if [[ -n "$list" ]]; then
			list+=",${base}"
		else
			list="${base}"
		fi
	done
	if [[ -z "$list" ]]; then
		echo "ERROR: no *-softmmu.mak targets found in ${targets_dir}" >&2
		return 1
	fi
	printf '%s\n' "$list"
}

# When executed directly, print the list for the repo's third_party/qemu.
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
	ROOT="$(cd "$(dirname "$0")/.." && pwd)"
	qemu_softmmu_target_list "${ROOT}/third_party/qemu"
fi
