#!/usr/bin/env python3
"""Build the aarch64 (or native) libimobiledevice stack into host-tools/<arch>.

Pins match extras/Inferno/patch_idevicerestore_cfs.py:
  idevicerestore 74e3bd9 + ChefKiss N104DEV->AP
  libimobiledevice c194db38
  libplist 18e5b22
The other libraries are the last commit on or before 2025-11-24, the same day
as that idevicerestore. usbmuxd is the upstream daemon that reads
USBMUXD_DEFAULT_DEVICE_MODE.
"""
from __future__ import print_function

import importlib.util
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ERA = "2025-11-25 00:00"
SRC = "/tmp/aqemu-host-tools-src"

PINNED = {
	"libplist": (
		"https://github.com/libimobiledevice/libplist.git",
		"18e5b22a71f85091127cc063db79c8df687c582c",
		"--without-cython",
	),
	"libimobiledevice-glue": (
		"https://github.com/libimobiledevice/libimobiledevice-glue.git",
		None,
		"",
	),
	"libusbmuxd": (
		"https://github.com/libimobiledevice/libusbmuxd.git",
		None,
		"",
	),
	"libtatsu": (
		"https://github.com/libimobiledevice/libtatsu.git",
		None,
		"",
	),
	"libimobiledevice": (
		"https://github.com/libimobiledevice/libimobiledevice.git",
		"c194db388fb7f1f65ef0a08817f26ed0e8d078b3",
		"--without-cython",
	),
	"libirecovery": (
		"https://github.com/libimobiledevice/libirecovery.git",
		None,
		"",
	),
	"usbmuxd": (
		"https://github.com/libimobiledevice/usbmuxd.git",
		None,
		"",
	),
	"ideviceinstaller": (
		"https://github.com/libimobiledevice/ideviceinstaller.git",
		None,
		"",
	),
	"idevicerestore": (
		"https://github.com/libimobiledevice/idevicerestore.git",
		"74e3bd9286d16fc1290abde061ee00831d5b36f8",
		"",
	),
}

ORDER = [
	"libplist",
	"libimobiledevice-glue",
	"libusbmuxd",
	"libtatsu",
	"libimobiledevice",
	"libirecovery",
	"usbmuxd",
	"ideviceinstaller",
	"idevicerestore",
]


def run(cmd, cwd=None, env=None):
	print("AQEMU: run %s" % " ".join(cmd), flush=True)
	subprocess.check_call(cmd, cwd=cwd, env=env)


def arch_name():
	m = os.uname().machine
	if m in ("aarch64", "arm64"):
		return "aarch64"
	if m in ("x86_64", "amd64"):
		return "x86_64"
	sys.exit("unsupported machine %s" % m)


def checkout(url, dest, sha):
	if os.path.isdir(os.path.join(dest, ".git")):
		print("AQEMU: reuse %s" % dest, flush=True)
	else:
		if os.path.isdir(dest):
			shutil.rmtree(dest)
		run(["git", "clone", url, dest])
	if sha:
		run(["git", "checkout", "--force", sha], cwd=dest)
		return
	out = subprocess.check_output(
		["git", "rev-list", "-n", "1", "--before=%s" % ERA, "HEAD"],
		cwd=dest,
	).decode().strip()
	if not out:
		print("AQEMU: no commit before %s in %s; staying on HEAD" % (ERA, dest))
		return
	print("AQEMU: era pin %s" % out, flush=True)
	run(["git", "checkout", "--force", out], cwd=dest)


def load_patcher():
	path = os.path.join(HERE, "patch_idevicerestore_cfs.py")
	spec = importlib.util.spec_from_file_location("aqemu_idr_patch", path)
	mod = importlib.util.module_from_spec(spec)
	spec.loader.exec_module(mod)
	return mod


def tree_has(dest, needle):
	for root, dirs, files in os.walk(dest):
		if ".git" in dirs:
			dirs.remove(".git")
		for fn in files:
			if not fn.endswith((".c", ".h")):
				continue
			try:
				blob = open(os.path.join(root, fn), "rb").read()
			except OSError:
				continue
			if needle in blob:
				return True
	return False


def build_one(name, prefix, env):
	url, sha, extra = PINNED[name]
	dest = os.path.join(SRC, name)
	checkout(url, dest, sha)
	if name == "usbmuxd":
		if not tree_has(dest, b"USBMUXD_DEFAULT_DEVICE_MODE"):
			print("AQEMU: era usbmuxd has no device-mode env; using master", flush=True)
			run(["git", "checkout", "--force", "master"], cwd=dest, env=env)
			if not tree_has(dest, b"USBMUXD_DEFAULT_DEVICE_MODE"):
				sys.exit("AQEMU: usbmuxd master has no USBMUXD_DEFAULT_DEVICE_MODE")
	if name == "idevicerestore":
		patcher = load_patcher()
		if not patcher.apply_chefkiss(dest):
			sys.exit("AQEMU: ChefKiss patch failed")
		if not patcher.apply_match_219(dest):
			print("AQEMU: match-219 marker not applied (continuing)")
	pc = os.path.join(prefix, "lib", "pkgconfig")
	flags = "--prefix=%s" % prefix
	if extra:
		flags += " " + extra
	if name == "usbmuxd":
		flags += " --with-udevrulesdir=%s/udev --with-systemdsystemunitdir=%s/systemd" % (
			prefix, prefix)
	env = env.copy()
	env["PKG_CONFIG_PATH"] = pc + (":" + env["PKG_CONFIG_PATH"] if env.get("PKG_CONFIG_PATH") else "")
	env.pop("PKG_CONFIG_LIBDIR", None)
	env["LD_LIBRARY_PATH"] = os.path.join(prefix, "lib")
	autogen = os.path.join(dest, "autogen.sh")
	if os.path.isfile(autogen):
		subprocess.check_call(
			["bash", "-lc", "export PKG_CONFIG_PATH='%s'; export LD_LIBRARY_PATH='%s'; ./autogen.sh %s" % (
				env["PKG_CONFIG_PATH"], env["LD_LIBRARY_PATH"], flags)],
			cwd=dest, env=env)
	else:
		run(["autoreconf", "-fi"], cwd=dest, env=env)
		subprocess.check_call(
			["bash", "-lc", "export PKG_CONFIG_PATH='%s'; ./configure %s" % (
				env["PKG_CONFIG_PATH"], flags)],
			cwd=dest, env=env)
	jobs = str(os.cpu_count() or 2)
	run(["make", "-j", jobs], cwd=dest, env=env)
	run(["make", "install"], cwd=dest, env=env)


def main():
	arch = arch_name()
	prefix = os.path.join(HERE, "host-tools", arch)
	os.makedirs(prefix, exist_ok=True)
	start = sys.argv[1] if len(sys.argv) > 1 else ORDER[0]
	if start not in ORDER:
		sys.exit("unknown package %s" % start)
	env = os.environ.copy()
	env["GIT_TERMINAL_PROMPT"] = "0"
	for name in ORDER[ORDER.index(start):]:
		print("AQEMU: ===== %s =====" % name, flush=True)
		build_one(name, prefix, env)
	idr = os.path.join(prefix, "bin", "idevicerestore")
	blob = open(idr, "rb").read()
	if b"INFO: model is" not in blob:
		sys.exit("AQEMU: built idevicerestore is missing the DEV->AP log string")
	if b"74e3bd9" not in blob and b"74e3bd9286d16fc1290abde061ee00831d5b36f8" not in blob:
		print("AQEMU: warning: 74e3bd9 string not in the binary (version banner may differ)")
	bridge_src = os.path.join(HERE, "aqemu-usb-bridge.c")
	bridge = os.path.join(prefix, "bin", "aqemu-usb-bridge")
	run(["gcc", "-O2", "-Wall", "-Wextra", "-pthread", "-o", bridge, bridge_src])
	print("AQEMU: host tools installed in %s" % prefix, flush=True)
	return 0


if __name__ == "__main__":
	sys.exit(main())
