#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# WSL path when invoked via /mnt/c/...
if command -v wslpath >/dev/null 2>&1; then
  :
fi
VENV=/tmp/aqemu-i18n
if [[ ! -x "${VENV}/bin/python" ]]; then
  python3 -m venv "${VENV}"
  "${VENV}/bin/pip" install -q deep-translator opencc-python-reimplemented
fi
# Strip CR if the script was edited on Windows.
sed -i 's/\r$//' "${ROOT}/scripts/generate_chinese_translations.py" || true
cd "${ROOT}"
exec "${VENV}/bin/python" scripts/generate_chinese_translations.py "$@"
