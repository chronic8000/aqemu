#!/usr/bin/env python3
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cache_path = root / "third_party" / ".bin" / "zh_translation_cache.json"
cache = json.loads(cache_path.read_text(encoding="utf-8")) if cache_path.exists() else {}
print("cache", len(cache), flush=True)
text = (root / "resources" / "translations" / "English.ts").read_text(encoding="utf-8")
sources = re.findall(r"<source>(.*?)</source>", text, flags=re.S)
unique = []
seen = set()
for s in sources:
    if s in seen:
        continue
    seen.add(s)
    if len(s) > 220:
        continue
    low = s.lstrip().lower()
    if low.startswith("<!doctype") or low.startswith("<html"):
        continue
    if not s.strip():
        continue
    unique.append(s)
todo = [s for s in unique if s not in cache]
print("todo", len(todo), "of", len(unique), flush=True)
out_dir = root / "third_party" / ".bin"
out_dir.mkdir(parents=True, exist_ok=True)
(out_dir / "zh_todo.json").write_text(
    json.dumps(todo, ensure_ascii=False, indent=1), encoding="utf-8"
)
n = 4
for i in range(n):
    chunk = todo[i::n]
    p = out_dir / f"zh_todo_{i}.json"
    p.write_text(json.dumps(chunk, ensure_ascii=False, indent=1), encoding="utf-8")
    print(p.name, len(chunk), flush=True)
