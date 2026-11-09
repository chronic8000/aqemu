#!/usr/bin/env bash
set -euo pipefail
VENV=/tmp/aqemu-i18n
python3 -m venv "${VENV}"
"${VENV}/bin/pip" install -q deep-translator opencc-python-reimplemented
"${VENV}/bin/python" - <<'PY'
from deep_translator import GoogleTranslator
from opencc import OpenCC
print(GoogleTranslator(source="en", target="zh-CN").translate("Language"))
print(OpenCC("s2t").convert("语言"))
print("OK")
PY
