#!/usr/bin/env python3
"""Build Chinese Qt .ts catalogs from English.ts.

Written Chinese for a GUI is Simplified (mainland/Singapore), Traditional
Taiwan, and Traditional Hong Kong. Spoken topolects share those written forms.
"""
from __future__ import annotations

import argparse
import html
import json
import re
import sys
import time
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGLISH_TS = ROOT / "resources" / "translations" / "English.ts"
OUT_DIR = ROOT / "resources" / "translations"
CACHE_PATH = ROOT / "third_party" / ".bin" / "zh_translation_cache.json"

# Long HTML / license blobs stay unfinished, matching French/German practice.
MAX_TRANSLATE_CHARS = 220
SKIP_PREFIXES = (
    "<!DOCTYPE HTML",
    "<html",
    "<!DOCTYPE",
)

# High-value UI strings translated by hand when the free API fails or returns junk.
SEED: dict[str, str] = {
    "About AQEMU": "关于 AQEMU",
    "About": "关于",
    "Language:": "语言：",
    "Language": "语言",
    "English": "英语",
    "Settings": "设置",
    "Advanced Settings": "高级设置",
    "Save": "保存",
    "Cancel": "取消",
    "OK": "确定",
    "Apply": "应用",
    "Close": "关闭",
    "Open": "打开",
    "New": "新建",
    "Delete": "删除",
    "Remove": "移除",
    "Add": "添加",
    "Edit": "编辑",
    "Help": "帮助",
    "File": "文件",
    "Exit": "退出",
    "Quit": "退出",
    "Start": "启动",
    "Stop": "停止",
    "Pause": "暂停",
    "Reset": "重置",
    "Power Off": "关机",
    "Reboot": "重启",
    "Snapshot": "快照",
    "Snapshots": "快照",
    "Create": "创建",
    "Browse": "浏览",
    "Browse...": "浏览...",
    "Path:": "路径：",
    "Name:": "名称：",
    "Type:": "类型：",
    "Size:": "大小：",
    "Memory": "内存",
    "CPU": "CPU",
    "Machine": "机器",
    "Accelerator": "加速器",
    "Network": "网络",
    "Display": "显示",
    "Audio": "音频",
    "Storage": "存储",
    "USB": "USB",
    "Boot": "引导",
    "General": "常规",
    "Devices": "设备",
    "Emulator": "模拟器",
    "Virtual Machine": "虚拟机",
    "New Virtual Machine": "新建虚拟机",
    "Delete Virtual Machine": "删除虚拟机",
    "Load VM": "加载虚拟机",
    "Save VM": "保存虚拟机",
    "VM Information": "虚拟机信息",
    "Warning": "警告",
    "Error": "错误",
    "Information": "信息",
    "Question": "询问",
    "Yes": "是",
    "No": "否",
    "Enabled": "已启用",
    "Disabled": "已禁用",
    "Default": "默认",
    "None": "无",
    "Auto": "自动",
    "Custom": "自定义",
    "Search": "搜索",
    "Filter": "筛选",
    "Refresh": "刷新",
    "Copy": "复制",
    "Paste": "粘贴",
    "Cut": "剪切",
    "Select All": "全选",
    "Options": "选项",
    "Advanced": "高级",
    "Basic": "基本",
    "Next": "下一步",
    "Back": "上一步",
    "Finish": "完成",
    "Continue": "继续",
    "Skip": "跳过",
    "Install": "安装",
    "Uninstall": "卸载",
    "Update": "更新",
    "Download": "下载",
    "Upload": "上传",
    "Import": "导入",
    "Export": "导出",
    "CD-ROM": "CD-ROM",
    "Hard Disk": "硬盘",
    "Floppy": "软盘",
    "Image": "镜像",
    "Firmware": "固件",
    "UEFI": "UEFI",
    "BIOS": "BIOS",
    "Keyboard": "键盘",
    "Mouse": "鼠标",
    "Tablet": "手写板",
    "Shared Folder": "共享文件夹",
    "Clipboard": "剪贴板",
    "Fullscreen": "全屏",
    "Screenshot": "截图",
    "Log": "日志",
    "Console": "控制台",
    "Monitor": "监视器",
    "Port": "端口",
    "Host": "主机",
    "Guest": "客户机",
    "User": "用户",
    "Password": "密码",
    "Username": "用户名",
    "Architecture": "架构",
    "Version": "版本",
    "License": "许可证",
    "Authors": "作者",
    "Credits": "致谢",
    "Documentation": "文档",
    "Website": "网站",
    "Report a Bug": "报告缺陷",
    "Check for Updates": "检查更新",
    "Preferences": "首选项",
    "Appearance": "外观",
    "Theme": "主题",
    "Icon Size": "图标大小",
    "Folder": "文件夹",
    "Directory": "目录",
    "File Name": "文件名",
    "Description": "描述",
    "Notes": "备注",
    "State": "状态",
    "Running": "运行中",
    "Stopped": "已停止",
    "Saved": "已保存",
    "Loading...": "正在加载...",
    "Please wait...": "请稍候...",
    "Are you sure?": "确定吗？",
    "This action cannot be undone.": "此操作无法撤消。",
    "Success": "成功",
    "Failed": "失败",
    "Not found": "未找到",
    "Invalid": "无效",
    "Required": "必填",
    "Optional": "可选",
}


def unescape_ts(text: str) -> str:
    return html.unescape(text.replace("&apos;", "'"))


def escape_ts(text: str) -> str:
    return (
        text.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&apos;")
    )


def should_skip(source: str) -> bool:
    s = source.strip()
    if not s:
        return True
    if len(s) > MAX_TRANSLATE_CHARS:
        return True
    low = s.lstrip().lower()
    return any(low.startswith(p.lower()) for p in SKIP_PREFIXES)


def load_cache() -> dict[str, str]:
    if CACHE_PATH.exists():
        return json.loads(CACHE_PATH.read_text(encoding="utf-8"))
    return {}


def save_cache(cache: dict[str, str]) -> None:
    CACHE_PATH.parent.mkdir(parents=True, exist_ok=True)
    CACHE_PATH.write_text(
        json.dumps(cache, ensure_ascii=False, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def translate_batch(texts: list[str], cache: dict[str, str]) -> None:
    pending = [t for t in texts if t not in cache]
    if not pending:
        return

    # Prefer offline Argos when installed; otherwise MyMemory one string at a time.
    use_argos = False
    try:
        import argostranslate.translate  # type: ignore

        probe = argostranslate.translate.translate("Save", "en", "zh")
        use_argos = bool(probe)
    except Exception:
        use_argos = False

    if use_argos:
        import argostranslate.translate  # type: ignore

        for i, src in enumerate(pending, 1):
            try:
                dst = argostranslate.translate.translate(src, "en", "zh")
                if isinstance(dst, str) and dst.strip():
                    cache[src] = dst.strip()
            except Exception as exc:
                print(f"WARN: argos failed '{src[:60]}': {exc}", file=sys.stderr, flush=True)
            if i % 25 == 0 or i == len(pending):
                save_cache(cache)
                print(f"translated {i}/{len(pending)}", flush=True)
        save_cache(cache)
        return

    from deep_translator import MyMemoryTranslator

    translator = MyMemoryTranslator(source="en-US", target="zh-CN")
    for i, src in enumerate(pending, 1):
        try:
            dst = translator.translate(src)
            if isinstance(dst, str) and dst.strip():
                cache[src] = dst.strip()
        except Exception as exc:
            print(f"WARN: failed '{src[:60]}': {exc}", file=sys.stderr, flush=True)
            time.sleep(1.0)
        if i % 10 == 0 or i == len(pending):
            save_cache(cache)
            print(f"translated {i}/{len(pending)}", flush=True)
        time.sleep(0.25)


def collect_sources(tree: ET.ElementTree) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for msg in tree.getroot().iter("message"):
        src_el = msg.find("source")
        if src_el is None or src_el.text is None:
            continue
        src = src_el.text
        if should_skip(src) or src in seen:
            continue
        seen.add(src)
        out.append(src)
    return out


def fill_tree(tree: ET.ElementTree, mapping: dict[str, str], lang: str) -> int:
    root = tree.getroot()
    root.set("language", lang)
    root.set("sourcelanguage", "en")
    filled = 0
    for msg in root.iter("message"):
        src_el = msg.find("source")
        tr_el = msg.find("translation")
        if src_el is None or tr_el is None or src_el.text is None:
            continue
        src = src_el.text
        zh = mapping.get(src)
        if not zh:
            tr_el.text = None
            tr_el.set("type", "unfinished")
            continue
        tr_el.text = zh
        if "type" in tr_el.attrib:
            del tr_el.attrib["type"]
        filled += 1
    return filled


def write_ts(path: Path, tree: ET.ElementTree) -> None:
    # ElementTree loses the DOCTYPE; Qt accepts the XML without it.
    xml = ET.tostring(tree.getroot(), encoding="utf-8", xml_declaration=True)
    path.write_bytes(xml + b"\n")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=0, help="Translate at most N new strings")
    parser.add_argument("--offline", action="store_true", help="Only use cache/seed, no network")
    args = parser.parse_args()

    if not ENGLISH_TS.exists():
        print(f"missing {ENGLISH_TS}", file=sys.stderr)
        return 1

    # Preserve comments/formatting less important than shipping catalogs.
    tree = ET.parse(ENGLISH_TS)
    sources = collect_sources(tree)
    print(f"unique translatable sources: {len(sources)}")

    cache = load_cache()
    cache.update(SEED)

    if not args.offline:
        todo = [s for s in sources if s not in cache]
        if args.limit > 0:
            todo = todo[: args.limit]
        print(f"to translate: {len(todo)}")
        translate_batch(todo, cache)

    from opencc import OpenCC

    s2tw = OpenCC("s2tw")
    s2hk = OpenCC("s2hk")

    zh_cn = {s: cache[s] for s in sources if s in cache}
    zh_tw = {s: s2tw.convert(zh_cn[s]) for s in zh_cn}
    zh_hk = {s: s2hk.convert(zh_cn[s]) for s in zh_cn}

    # Hong Kong UI wording tweaks on top of Traditional.
    hk_overrides = {
        "Settings": "設定",
        "Advanced Settings": "進階設定",
        "Folder": "資料夾",
        "Directory": "目錄",
        "Hard Disk": "硬碟",
        "Software": "軟件",
        "File": "檔案",
        "Network": "網絡",
        "Memory": "記憶體",
        "Information": "資訊",
        "Default": "預設",
        "Video": "視像",
        "Program": "程式",
    }
    for en, hk in hk_overrides.items():
        if en in zh_cn:
            zh_hk[en] = hk

    targets = [
        ("Chinese_Simplified.ts", "zh_CN", zh_cn),
        ("Chinese_Traditional.ts", "zh_TW", zh_tw),
        ("Chinese_Hong_Kong.ts", "zh_HK", zh_hk),
    ]
    for name, code, mapping in targets:
        t = ET.parse(ENGLISH_TS)
        filled = fill_tree(t, mapping, code)
        out = OUT_DIR / name
        write_ts(out, t)
        print(f"wrote {out.name}: {filled}/{len(list(t.getroot().iter('message')))} messages")

    save_cache(cache)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
