#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
QEMU="${ROOT}/third_party/reims-vgpu/vendor/qemu"
PATCHDIR="${ROOT}/third_party/patches"
cd "${QEMU}"
for p in "${PATCHDIR}"/000*.patch; do
  name="$(basename "$p")"
  echo "Applying ${name} to vendor/qemu"
  if git apply --check "$p" 2>/dev/null; then
    git apply "$p"
    echo "  applied"
  elif git apply --reverse --check "$p" 2>/dev/null; then
    echo "  already applied"
  else
    echo "  WARN: could not apply cleanly; continuing"
  fi
done
grep -n 'Windows cannot create\|Developer Mode\|sys.exit(0)' scripts/symlink-install-tree.py || true
