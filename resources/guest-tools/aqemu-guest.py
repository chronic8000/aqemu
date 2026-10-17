#!/usr/bin/env python3
"""Send files copied inside the guest to AQEMU over the virtio serial port."""
import os
import shutil
import subprocess
import time
from urllib.parse import unquote, urlparse

PORT = "/dev/virtio-ports/org.aqemu.files"
if os.name == "nt":
    PORT = r"\\.\org.aqemu.files"


def clipboard_files():
    commands = (
        ["wl-paste", "-n", "-t", "text/uri-list"],
        ["xclip", "-selection", "clipboard", "-t", "text/uri-list", "-o"],
    )
    for cmd in commands:
        if shutil.which(cmd[0]) is None:
            continue
        try:
            out = subprocess.check_output(cmd, stderr=subprocess.DEVNULL, timeout=2)
        except Exception:
            continue
        paths = []
        for raw in out.splitlines():
            line = raw.decode("utf-8", "replace").strip()
            if not line.startswith("file://"):
                continue
            path = unquote(urlparse(line).path)
            if os.path.isfile(path):
                paths.append(path)
        return paths
    return []


def send(fp, paths):
    fp.write(b"AQEMU1\nFROM-GUEST\n")
    fp.write(("%d\n" % len(paths)).encode("utf-8"))
    for path in paths:
        name = os.path.basename(path).replace("\n", "_").replace("\r", "_")
        with open(path, "rb") as src:
            data = src.read()
        if len(data) > 64 * 1024 * 1024:
            continue
        fp.write(name.encode("utf-8") + b"\n")
        fp.write(("%d\n" % len(data)).encode("utf-8"))
        fp.write(data)
    fp.flush()


def main():
    last = None
    while True:
        try:
            with open(PORT, "r+b", buffering=0) as fp:
                while True:
                    paths = clipboard_files()
                    sig = tuple(paths)
                    if paths and sig != last:
                        send(fp, paths)
                        last = sig
                    time.sleep(0.8)
        except Exception:
            time.sleep(2)


if __name__ == "__main__":
    main()
