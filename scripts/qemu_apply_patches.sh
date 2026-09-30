#!/usr/bin/env bash
# Apply AQEMU patches onto the upstream QEMU submodule.
# Idempotent: a tree that already contains the patch is left alone.
# Used by local build scripts and GitHub Actions on every host.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
QEMU_SRC="${ROOT}/third_party/qemu"
PATCH_DIR="${ROOT}/third_party/patches"

if [[ ! -f "${QEMU_SRC}/meson.build" && ! -f "${QEMU_SRC}/configure" ]]; then
	echo "QEMU source is not checked out; skipping patches"
	exit 0
fi

shopt -s nullglob
patches=( "${PATCH_DIR}"/*.patch )
if [[ ${#patches[@]} -eq 0 ]]; then
	exit 0
fi

for patch in "${patches[@]}"; do
	name="$(basename "${patch}")"
	if git -C "${QEMU_SRC}" apply --reverse --check "${patch}" >/dev/null 2>&1; then
		echo "QEMU patch already applied: ${name}"
		continue
	fi
	if git -C "${QEMU_SRC}" apply --check "${patch}" >/dev/null 2>&1; then
		git -C "${QEMU_SRC}" apply "${patch}"
		echo "Applied QEMU patch: ${name}"
		continue
	fi
	echo "ERROR: QEMU patch does not apply: ${patch}" >&2
	echo "The submodule pin and third_party/patches are out of step." >&2
	exit 1
done
