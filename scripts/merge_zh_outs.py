#!/usr/bin/env python3
"""Merge agent zh_out_*.json maps into the translation cache and rebuild .ts files."""
from __future__ import annotations

import html
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "third_party" / ".bin"
CACHE = BIN / "zh_translation_cache.json"


def normalize_key(text: str) -> str:
    # Agent chunks may still contain TS entities (&amp;, &lt;, …).
    return html.unescape(text.replace("&apos;", "'"))


def main() -> int:
    cache = json.loads(CACHE.read_text(encoding="utf-8")) if CACHE.exists() else {}
    before = len(cache)
    for i in range(4):
        p = BIN / f"zh_out_{i}.json"
        if not p.exists():
            print(f"missing {p.name}", flush=True)
            continue
        data = json.loads(p.read_text(encoding="utf-8"))
        if not isinstance(data, dict):
            print(f"bad format {p.name}", flush=True)
            continue
        added = 0
        for k, v in data.items():
            if not isinstance(k, str) or not isinstance(v, str):
                continue
            if not v.strip():
                continue
            key = normalize_key(k)
            if key not in cache:
                added += 1
            cache[key] = v.strip()
        print(f"merged {p.name}: {len(data)} keys, {added} new", flush=True)
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    CACHE.write_text(
        json.dumps(cache, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(f"cache {before} -> {len(cache)}", flush=True)
    gen = ROOT / "scripts" / "generate_chinese_translations.py"
    return subprocess.call([sys.executable, str(gen), "--offline"])


if __name__ == "__main__":
    raise SystemExit(main())
