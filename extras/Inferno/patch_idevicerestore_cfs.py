#!/usr/bin/env python3
"""Pin companion idevicerestore to ChefKiss-era 74e3bd9 (once).

Inferno iOS 14 ramrod needs CreateFilesystemPartitions=true. Companion
1.0.0-271-g45145e9 always arrives at ramrod as CFS=false (GPT 78) even
with --erase. Working public logs use 74e3bd9. This script clones that
commit once, applies the ChefKiss N104DEV->AP patch, and installs it.
Later Restores see 74e3bd9 in --version and skip the rebuild.
"""
from __future__ import print_function

import os
import re
import shutil
import struct
import subprocess
import sys

MARKER = "/usr/local/etc/aqemu-idr-74e3bd9"
LIMD_MARKER = "/usr/local/etc/aqemu-limd-c194db38"
PIN_VER = "74e3bd9"
PIN_SHA = "74e3bd9286d16fc1290abde061ee00831d5b36f8"
PIN_SRC = "/tmp/aqemu-idr-74e3bd9"
IDR_GIT = "https://github.com/libimobiledevice/idevicerestore.git"
# Same-day stack as idevicerestore 74e3bd9 (2025-11-24). Companion .so files
# are newer (45145e9 era) and ramrod still sees CFS=false without LD_PRELOAD.
LIMD_SHA = "c194db388fb7f1f65ef0a08817f26ed0e8d078b3"
LIMD_GIT = "https://github.com/libimobiledevice/libimobiledevice.git"
LIMD_SRC = "/tmp/aqemu-limd-c194db38"
PLIST_SHA = "18e5b22a71f85091127cc063db79c8df687c582c"
PLIST_GIT = "https://github.com/libimobiledevice/libplist.git"
PLIST_SRC = "/tmp/aqemu-plist-18e5b22"
DUMP_TAG = "AQEMU StartRestore options"
MATCH_TAG = "AQEMU match-219"
USB_TCP_TAG = "AQEMU USB-TCP v4"
KEY = b"CreateFilesystemPartitions\x00"
DEFAULT_BIN = "/usr/local/bin/idevicerestore"


def run(cmd, cwd=None):
	print("AQEMU: run %s" % " ".join(cmd))
	sys.stdout.flush()
	env = os.environ.copy()
	env["GIT_TERMINAL_PROMPT"] = "0"
	subprocess.check_call(cmd, cwd=cwd, env=env)


def idr_version(binary=DEFAULT_BIN):
	try:
		out = subprocess.check_output(
			[binary, "--version"], stderr=subprocess.STDOUT
		).decode("ascii", "replace")
		return out
	except Exception as e:
		return "(%s)" % e


def pkgconfig_env():
	paths = [
		"/usr/local/lib/pkgconfig",
		"/usr/local/share/pkgconfig",
		"/usr/lib/x86_64-linux-gnu/pkgconfig",
		"/usr/lib/pkgconfig",
		"/usr/share/pkgconfig",
	]
	old = os.environ.get("PKG_CONFIG_PATH", "")
	if old:
		paths.append(old)
	env = os.environ.copy()
	env["PKG_CONFIG_PATH"] = ":".join(paths)
	env.pop("PKG_CONFIG_LIBDIR", None)
	env["GIT_TERMINAL_PROMPT"] = "0"
	env["DEBIAN_FRONTEND"] = "noninteractive"
	return env


def ver_tuple(s):
	parts = []
	for p in s.split("."):
		try:
			parts.append(int(p))
		except ValueError:
			parts.append(0)
	return tuple(parts + [0, 0, 0])[:3]


def find_include_root(header_rel):
	"""Directory to pass as -I so #include <header_rel> works."""
	header_rel = header_rel.replace("\\", "/")
	leaf = os.path.basename(header_rel)
	parent = os.path.dirname(header_rel)
	for search in ("/usr/local/include", "/usr/include"):
		direct = os.path.join(search, header_rel)
		if os.path.isfile(direct):
			return search
		if not os.path.isdir(search):
			continue
		for dp, _, fns in os.walk(search):
			if leaf not in fns:
				continue
			if parent:
				if os.path.basename(dp) == os.path.basename(parent):
					return os.path.dirname(dp)
			else:
				return dp
	return None


def copy_tree_contents(src, dst):
	if not os.path.isdir(dst):
		os.makedirs(dst)
	for name in os.listdir(src):
		s = os.path.join(src, name)
		d = os.path.join(dst, name)
		if os.path.isdir(s):
			copy_tree_contents(s, d)
		else:
			shutil.copy2(s, d)


HDR_REPOS = (
	("libplist", "https://github.com/libimobiledevice/libplist.git"),
	("libimobiledevice-glue",
		"https://github.com/libimobiledevice/libimobiledevice-glue.git"),
	("libusbmuxd", "https://github.com/libimobiledevice/libusbmuxd.git"),
	("libtatsu", "https://github.com/libimobiledevice/libtatsu.git"),
	("libirecovery", "https://github.com/libimobiledevice/libirecovery.git"),
	("libimobiledevice",
		"https://github.com/libimobiledevice/libimobiledevice.git"),
)


def ensure_dev_headers(needed):
	"""Companion often has /usr/local/lib/*.so but no matching headers."""
	missing = []
	for header_rel in needed:
		root = find_include_root(header_rel)
		if root:
			print("AQEMU: header %s via -I%s" % (header_rel, root))
		else:
			missing.append(header_rel)
	if not missing:
		return True
	print("AQEMU: missing headers: %s" % ", ".join(missing))
	print("AQEMU: cloning libimobiledevice include trees into /usr/local/include (once)")
	inc = "/usr/local/include"
	if not os.path.isdir(inc):
		os.makedirs(inc)
	ok = True
	for name, url in HDR_REPOS:
		tmp = "/tmp/aqemu-hdr-" + name
		try:
			if os.path.isdir(tmp):
				shutil.rmtree(tmp)
			run(["git", "clone", "--depth", "1", url, tmp])
			src = os.path.join(tmp, "include")
			if not os.path.isdir(src):
				print("AQEMU: %s has no include/" % name)
				ok = False
				continue
			copy_tree_contents(src, inc)
			print("AQEMU: copied %s include/ -> %s" % (name, inc))
		except (subprocess.CalledProcessError, OSError) as e:
			print("AQEMU: header clone %s failed: %s" % (name, e))
			ok = False
	still = [h for h in needed if not find_include_root(h)]
	if still:
		print("AQEMU: still missing headers after clone: %s" % ", ".join(still))
		return False
	return ok


def pc_needs_rewrite(path, min_version):
	if not os.path.isfile(path):
		return True
	text = open(path, "r").read()
	if "AQEMU stub" in text:
		return True
	for line in text.splitlines():
		if line.lower().startswith("version:"):
			got = line.split(":", 1)[1].strip()
			if ver_tuple(got) < ver_tuple(min_version):
				print("AQEMU: %s Version %s < %s, rewriting" % (path, got, min_version))
				return True
			print("AQEMU: pkg-config %s already Version %s" % (os.path.basename(path), got))
			return False
	return True


def write_pc(name, lib, version, header_rel):
	d = "/usr/local/lib/pkgconfig"
	path = os.path.join(d, name + ".pc")
	if not pc_needs_rewrite(path, version):
		return
	inc = find_include_root(header_rel) or "/usr/local/include"
	hdr = os.path.join(inc, header_rel)
	if not os.path.isfile(hdr):
		print("AQEMU: header missing %s (writing %s.pc Version %s anyway)" % (
			hdr, name, version))
	if not os.path.isdir(d):
		os.makedirs(d)
	body = (
		"prefix=/usr/local\n"
		"exec_prefix=${prefix}\n"
		"libdir=${prefix}/lib\n"
		"includedir=%s\n"
		"\n"
		"Name: %s\n"
		"Description: %s (AQEMU stub for 74e3bd9 pin)\n"
		"Version: %s\n"
		"Libs: -L${libdir} -l%s\n"
		"Cflags: -I${includedir}\n"
	) % (inc, name, name, version, lib)
	open(path, "w").write(body)
	print("AQEMU: wrote %s Version %s includedir=%s" % (path, version, inc))


def find_lib_dir(lib):
	"""Directory containing lib<lib>.so* (not a prefix of another lib)."""
	prefix = "lib%s.so" % lib
	for d in (
		"/usr/local/lib",
		"/usr/lib/x86_64-linux-gnu",
		"/lib/x86_64-linux-gnu",
		"/usr/lib",
	):
		if not os.path.isdir(d):
			continue
		try:
			names = os.listdir(d)
		except OSError:
			continue
		for fn in names:
			if fn == prefix or fn.startswith(prefix + "."):
				return d
	return None


def pc_exists(mod, env):
	return subprocess.call(
		["pkg-config", "--exists", mod],
		env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
	) == 0


def write_pc_sys(name, lib, version, header_rel, libdir):
	d = "/usr/local/lib/pkgconfig"
	path = os.path.join(d, name + ".pc")
	if not pc_needs_rewrite(path, version):
		return
	inc = find_include_root(header_rel) or "/usr/include"
	if not os.path.isdir(d):
		os.makedirs(d)
	body = (
		"prefix=/usr\n"
		"exec_prefix=${prefix}\n"
		"libdir=%s\n"
		"includedir=%s\n"
		"\n"
		"Name: %s\n"
		"Description: %s (AQEMU stub for 74e3bd9 pin)\n"
		"Version: %s\n"
		"Libs: -L${libdir} -l%s\n"
		"Cflags: -I${includedir}\n"
	) % (libdir, inc, name, name, version, lib)
	open(path, "w").write(body)
	print("AQEMU: wrote %s Version %s libdir=%s includedir=%s" % (
		path, version, libdir, inc))


def ensure_system_dev():
	"""libzip/zlib/libcurl are distro .so files; companion often lacks -dev .pc."""
	env = pkgconfig_env()
	mods = (
		("libzip", "zip", "1.7.3", "zip.h", "libzip-dev"),
		("zlib", "z", "1.2.11", "zlib.h", "zlib1g-dev"),
		("libcurl", "curl", "7.81.0", "curl/curl.h", "libcurl4-openssl-dev"),
	)
	need_apt = []
	for mod, lib, ver, hdr, pkg in mods:
		if pc_exists(mod, env) and find_include_root(hdr):
			print("AQEMU: pkg-config %s already present" % mod)
			continue
		need_apt.append(pkg)
	if need_apt:
		pkgs = list(dict.fromkeys(need_apt))
		print("AQEMU: apt-get install %s (for 74e3bd9 configure)" % " ".join(pkgs))
		cmd = ["apt-get", "install", "-y"] + pkgs
		try:
			subprocess.check_call(cmd, env=env)
		except subprocess.CalledProcessError:
			print("AQEMU: apt-get install failed; retrying after apt-get update")
			try:
				subprocess.check_call(["apt-get", "update", "-qq"], env=env)
				subprocess.check_call(cmd, env=env)
			except subprocess.CalledProcessError as e:
				print("AQEMU: apt-get still failed: %s" % e)
	env = pkgconfig_env()
	if not find_include_root("zip.h"):
		tmp = "/tmp/aqemu-hdr-libzip"
		try:
			if os.path.isdir(tmp):
				shutil.rmtree(tmp)
			run(["git", "clone", "--depth", "1",
				"https://github.com/nih-at/libzip.git", tmp])
			src = os.path.join(tmp, "lib", "zip.h")
			inc = "/usr/local/include"
			if not os.path.isdir(inc):
				os.makedirs(inc)
			if os.path.isfile(src):
				shutil.copy2(src, os.path.join(inc, "zip.h"))
				print("AQEMU: copied zip.h -> %s" % inc)
			real_conf = os.path.join(tmp, "lib", "zipconf.h")
			dst_conf = os.path.join(inc, "zipconf.h")
			if os.path.isfile(real_conf):
				shutil.copy2(real_conf, dst_conf)
			elif not os.path.isfile(dst_conf):
				open(dst_conf, "w").write(
					"#ifndef _HAD_ZIPCONF_H\n#define _HAD_ZIPCONF_H\n"
					"#include <stdint.h>\n"
					"typedef int8_t zip_int8_t;\n"
					"typedef uint8_t zip_uint8_t;\n"
					"typedef int16_t zip_int16_t;\n"
					"typedef uint16_t zip_uint16_t;\n"
					"typedef int32_t zip_int32_t;\n"
					"typedef uint32_t zip_uint32_t;\n"
					"typedef int64_t zip_int64_t;\n"
					"typedef uint64_t zip_uint64_t;\n"
					"#define ZIP_LIBZIP_VERSION \"1.7.3\"\n"
					"#endif\n"
				)
				print("AQEMU: wrote minimal zipconf.h")
		except (subprocess.CalledProcessError, OSError) as e:
			print("AQEMU: libzip header fallback failed: %s" % e)
	env = pkgconfig_env()
	for mod, lib, ver, hdr, pkg in mods:
		if pc_exists(mod, env):
			try:
				out = subprocess.check_output(
					["pkg-config", "--modversion", mod],
					stderr=subprocess.STDOUT, env=env
				).decode("ascii", "replace")
				print("AQEMU: pkg-config %s => %s" % (mod, out.strip()))
			except subprocess.CalledProcessError:
				print("AQEMU: pkg-config %s exists (no version)" % mod)
			continue
		libdir = find_lib_dir(lib) or "/usr/lib/x86_64-linux-gnu"
		print("AQEMU: stubbing %s.pc (lib in %s)" % (mod, libdir))
		write_pc_sys(mod, lib, ver, hdr, libdir)


def ensure_pkgconfig():
	"""Companion libs live in /usr/local but .pc files are often missing.

	74e3bd9 configure.ac wants libimobiledevice-1.0 >= 1.4.0, libplist-2.0 >= 2.6.0,
	libimobiledevice-glue-1.0 >= 1.3.0. The installed SONAME (e.g. .so.6) is not
	the pkg-config Version; a stub of 1.0.6 makes configure fail.
	"""
	needed = (
		"libirecovery.h",
		"libimobiledevice/libimobiledevice.h",
		"libimobiledevice-glue/glue.h",
		"usbmuxd.h",
		"plist/plist.h",
		"libtatsu/tatsu.h",
	)
	ensure_dev_headers(needed)
	# Versions must satisfy 74e3bd9 configure.ac (not the .so.N SONAME).
	write_pc("libirecovery-1.0", "irecovery-1.0", "1.3.1", "libirecovery.h")
	write_pc("libimobiledevice-1.0", "imobiledevice-1.0", "1.4.0",
		"libimobiledevice/libimobiledevice.h")
	write_pc("libimobiledevice-glue-1.0", "imobiledevice-glue-1.0", "1.3.0",
		"libimobiledevice-glue/glue.h")
	write_pc("libusbmuxd-2.0", "usbmuxd-2.0", "2.0.2", "usbmuxd.h")
	write_pc("libplist-2.0", "plist-2.0", "2.6.0", "plist/plist.h")
	write_pc("libtatsu-1.0", "tatsu", "1.0.5", "libtatsu/tatsu.h")
	write_pc("libtatsu", "tatsu", "1.0.5", "libtatsu/tatsu.h")
	ensure_system_dev()
	env = pkgconfig_env()
	print("AQEMU: PKG_CONFIG_PATH=%s" % env.get("PKG_CONFIG_PATH", ""))
	try:
		out = subprocess.check_output(
			["ls", "-la", "/usr/local/lib/pkgconfig"],
			stderr=subprocess.STDOUT, env=env
		).decode("ascii", "replace")
		print("AQEMU: /usr/local/lib/pkgconfig:\n%s" % out)
	except Exception as e:
		print("AQEMU: ls pkgconfig: %s" % e)
	for mod in ("libirecovery-1.0", "libimobiledevice-1.0", "libplist-2.0",
			"libimobiledevice-glue-1.0", "libzip", "zlib", "libcurl"):
		try:
			out = subprocess.check_output(
				["pkg-config", "--modversion", mod],
				stderr=subprocess.STDOUT, env=env
			).decode("ascii", "replace")
			print("AQEMU: pkg-config %s => %s" % (mod, out.strip()))
		except subprocess.CalledProcessError as e:
			err = e.output.decode("ascii", "replace") if e.output else str(e)
			print("AQEMU: pkg-config %s failed: %s" % (mod, err))
	return env


def apply_chefkiss(src_root):
	path = os.path.join(src_root, "src", "restore.c")
	text = open(path, "r").read()
	if "INFO: model is" in text:
		print("AQEMU: ChefKiss DEV->AP already in restore.c")
		return True
	anchor = "restore_get_irecv_device"
	idx = text.find(anchor)
	if idx < 0:
		print("AQEMU: restore_get_irecv_device not in restore.c")
		return False
	chunk = text[idx : idx + 4000]
	needle = "\tplist_get_string_val(node, &model);\n"
	insert = (
		needle
		+ '\tlogger(LL_INFO, "INFO: model is %s\\n", model);\n'
		+ '\tif (strstr(model, "DEV")) {\n'
		+ '\t\tstrncpy(strstr(model, "DEV"), "AP\\0", 3);\n'
		+ "\t}\n"
	)
	if needle not in chunk:
		needle = "        plist_get_string_val(node, &model);\n"
		insert = (
			needle
			+ '        logger(LL_INFO, "INFO: model is %s\\n", model);\n'
			+ '        if (strstr(model, "DEV")) {\n'
			+ '                strncpy(strstr(model, "DEV"), "AP\\0", 3);\n'
			+ "        }\n"
		)
	if needle not in chunk:
		print("AQEMU: cannot find plist_get_string_val(node, &model) near restore_get_irecv_device")
		return False
	chunk2 = chunk.replace(needle, insert, 1)
	open(path, "w").write(text[:idx] + chunk2 + text[idx + len(chunk) :])
	print("AQEMU: applied ChefKiss N104DEV->AP in restore.c")
	return True


def apply_match_219(src_root):
	"""Short marker only. Do not dump StartRestore XML (Inferno #219 did not)."""
	path = os.path.join(src_root, "src", "restore.c")
	text = open(path, "r").read()
	if MATCH_TAG in text:
		print("AQEMU: %s already in restore.c" % MATCH_TAG)
		return True
	needle = 'logger(LL_INFO, "INFO: model is %s\\n", model);\n'
	if needle not in text:
		print("AQEMU: ChefKiss model log not in restore.c (cannot add %s)" % MATCH_TAG)
		return False
	insert = needle + '\tlogger(LL_INFO, "%s\\n");\n' % MATCH_TAG
	open(path, "w").write(text.replace(needle, insert, 1))
	print("AQEMU: %s (ChefKiss only, no XML dump / option rewrite)" % MATCH_TAG)
	return True


def apply_usb_tcp_cfs(src_root):
	"""v4: ramrod ignores RestoreOptions CFS boolean (v2/v3 proved it).

	Send CreateFilesystemPartitions as integer 1 inside RestoreOptions, and
	also as a boolean on the top-level StartRestore message via restored_send.
	"""
	path = os.path.join(src_root, "src", "restore.c")
	text = open(path, "r").read()
	if USB_TCP_TAG in text:
		print("AQEMU: %s already in restore.c" % USB_TCP_TAG)
		return True
	needle = "restore_error = restored_start_restore(restore, opts, client->restore->protocol_version);"
	if needle not in text:
		print("AQEMU: restored_start_restore call not in restore.c (cannot force CFS)")
		return False
	insert = (
		"\t{\n"
		"\t\tplist_t aqemu_mini = plist_new_dict();\n"
		'\t\tplist_t aqemu_uuid = plist_dict_get_item(opts, "UUID");\n'
		'\t\tplist_dict_set_item(aqemu_mini, "AutoBootDelay", plist_new_uint(0));\n'
		'\t\tplist_dict_set_item(aqemu_mini, "CreateFilesystemPartitions", plist_new_uint(1));\n'
		'\t\tplist_dict_set_item(aqemu_mini, "AuthInstallRestoreBehavior", plist_new_string("Erase"));\n'
		"\t\tif (aqemu_uuid)\n"
		'\t\t\tplist_dict_set_item(aqemu_mini, "UUID", plist_copy(aqemu_uuid));\n'
		'\t\tplist_dict_set_item(aqemu_mini, "SystemImage", plist_new_bool(1));\n'
		'\t\tplist_dict_set_item(aqemu_mini, "FlashNOR", plist_new_bool(1));\n'
		"\t\tplist_free(opts);\n"
		"\t\topts = aqemu_mini;\n"
		"\t}\n"
		"\t{\n"
		"\t\tplist_t aqemu_msg = plist_new_dict();\n"
		'\t\tplist_dict_set_item(aqemu_msg, "Request", plist_new_string("StartRestore"));\n'
		'\t\tplist_dict_set_item(aqemu_msg, "CreateFilesystemPartitions", plist_new_bool(1));\n'
		'\t\tplist_dict_set_item(aqemu_msg, "RestoreOptions", plist_copy(opts));\n'
		'\t\tplist_dict_set_item(aqemu_msg, "RestoreProtocolVersion", plist_new_uint(client->restore->protocol_version));\n'
		"\t\t{\n"
		"\t\t\tchar *aqemu_xml = NULL;\n"
		"\t\t\tuint32_t aqemu_xlen = 0;\n"
		"\t\t\tplist_to_xml(aqemu_msg, &aqemu_xml, &aqemu_xlen);\n"
		"\t\t\tif (aqemu_xml) {\n"
		'\t\t\t\tlogger(LL_INFO, "' + USB_TCP_TAG + ' XML (%u bytes):\\n%s\\n",'
		" aqemu_xlen, aqemu_xml);\n"
		"\t\t\t\tfree(aqemu_xml);\n"
		"\t\t\t}\n"
		"\t\t}\n"
		"\t\trestore_error = restored_send(restore, aqemu_msg);\n"
		"\t\tplist_free(aqemu_msg);\n"
		"\t}\n"
	)
	open(path, "w").write(text.replace(needle, insert, 1))
	print("AQEMU: %s (top-level CFS bool + RestoreOptions CFS integer 1)" % USB_TCP_TAG)
	return True


def apply_opts_dump(src_root):
	"""Log StartRestore XML immediately before it is sent."""
	path = os.path.join(src_root, "src", "restore.c")
	text = open(path, "r").read()
	if DUMP_TAG in text:
		print("AQEMU: StartRestore XML dump already in restore.c")
		return True
	needle = "restore_error = restored_start_restore(restore, opts, client->restore->protocol_version);"
	if needle not in text:
		print("AQEMU: restored_start_restore call not in restore.c (cannot add XML dump)")
		return False
	dump_log = (
		'\t\t\tlogger(LL_INFO, "%s (%%u bytes):\\n%%s\\n", aqemu_xlen, aqemu_xml);\n'
		% DUMP_TAG
	)
	insert = (
		"\t{\n"
		"\t\tchar *aqemu_xml = NULL;\n"
		"\t\tuint32_t aqemu_xlen = 0;\n"
		"\t\tplist_to_xml(opts, &aqemu_xml, &aqemu_xlen);\n"
		"\t\tif (aqemu_xml) {\n"
		+ dump_log
		+ "\t\t\tfree(aqemu_xml);\n"
		+ "\t\t}\n"
		+ "\t}\n"
		+ "\t" + needle
	)
	open(path, "w").write(text.replace(needle, insert, 1))
	print("AQEMU: added StartRestore XML dump before restored_start_restore")
	return True


def apply_inferno_ios14_opts(src_root):
	"""74e3bd9 sends iOS 18 keys. Inferno iOS 14 ramrod then shows CFS=false.

	Host XML dump already has CreateFilesystemPartitions true; ramrod still
	takes verify_storage_for_update. Strip AEA/iOS18 keys and request Erase.
	"""
	path = os.path.join(src_root, "src", "restore.c")
	text = open(path, "r").read()
	if "AQEMU CFS-first" in text:
		print("AQEMU: Inferno iOS14 option strip already in restore.c")
		return True
	orig = text
	for key in (
		"HostHasFixFor99053849",
		"SystemImageFormat",
		"WaitForDeviceConnectionToFinishStateMachine",
	):
		text, n = re.subn(
			r'\n[ \t]*plist_dict_set_item\(opts, "%s", [^;]+;\n' % key,
			"\n",
			text,
			count=1,
		)
		if n == 0:
			print("AQEMU: did not find %s set_item in restore.c" % key)
	text, n_async = re.subn(
		r'\n[ \t]*plist_t async_data_types = plist_new_dict\(\);'
		r'(?:\n[ \t]*plist_dict_set_item\(async_data_types, "[^"]+", plist_new_bool\([01]\)\);){5}'
		r'\n[ \t]*plist_dict_set_item\(opts, "SupportedAsyncDataTypes", async_data_types\);',
		"\n\t/* AQEMU Inferno iOS14 opts: stripped iOS18 keys */",
		text,
		count=1,
	)
	if n_async == 0:
		print("AQEMU: SupportedAsyncDataTypes block did not match")
	text, n_auth = re.subn(
		r'//[ \t]*plist_dict_set_item\(opts, "AuthInstallRestoreBehavior", plist_new_string\("Erase"\)\);',
		'plist_dict_set_item(opts, "AuthInstallRestoreBehavior", plist_new_string("Erase")); /* AQEMU Inferno iOS14 opts */',
		text,
		count=1,
	)
	if n_auth == 0:
		cfs = 'plist_dict_set_item(opts, "CreateFilesystemPartitions", plist_new_bool(1));'
		if cfs in text:
			text = text.replace(
				cfs,
				cfs + '\n\tplist_dict_set_item(opts, "AuthInstallRestoreBehavior", plist_new_string("Erase")); /* AQEMU Inferno iOS14 opts */',
				1)
			print("AQEMU: inserted AuthInstallRestoreBehavior=Erase next to CFS")
		else:
			print("AQEMU: could not set AuthInstallRestoreBehavior=Erase")
			return False
	# USB-over-TCP can drop the tail of the StartRestore XML. Ramrod parsed
	# UUID (near the end) but CreateFilesystemPartitions after it as false.
	# Set CFS first so it lands in the first USB packet. libplist set_item
	# replaces in place, so the later CFS set_item keeps this position.
	opts_new = "plist_t opts = plist_new_dict();"
	if "AQEMU CFS-first" not in text and opts_new in text:
		text = text.replace(
			opts_new,
			opts_new
			+ '\n\tlogger(LL_INFO, "AQEMU CFS-first\\n");\n'
			+ '\tplist_dict_set_item(opts, "CreateFilesystemPartitions", plist_new_bool(1));',
			1)
	if text == orig:
		print("AQEMU: Inferno iOS14 option strip made no changes")
		return False
	if "AEAWrappedDiskImage" in text:
		print("AQEMU: AEAWrappedDiskImage still in restore.c after strip")
		return False
	open(path, "w").write(text)
	print("AQEMU: stripped iOS18 keys, AuthInstallRestoreBehavior=Erase, CFS-first")
	return True


def binary_has_aea(binary):
	try:
		return b"AEAWrappedDiskImage" in open(binary, "rb").read()
	except OSError:
		return True


def binary_has_cfs_first(binary):
	try:
		return b"AQEMU CFS-first" in open(binary, "rb").read()
	except OSError:
		return False


def git_checkout_sha(url, dest, sha):
	if os.path.isdir(dest):
		shutil.rmtree(dest)
	os.makedirs(dest)
	run(["git", "init"], cwd=dest)
	run(["git", "remote", "add", "origin", url], cwd=dest)
	try:
		run(["git", "fetch", "--depth", "1", "origin", sha], cwd=dest)
	except subprocess.CalledProcessError:
		print("AQEMU: shallow fetch failed; trying full clone")
		shutil.rmtree(dest)
		run(["git", "clone", url, dest])
		run(["git", "checkout", sha], cwd=dest)
		return
	run(["git", "checkout", "FETCH_HEAD"], cwd=dest)


def autogen_install(src, env, extra=""):
	pc = env.get("PKG_CONFIG_PATH", "/usr/local/lib/pkgconfig")
	autogen = os.path.join(src, "autogen.sh")
	if os.path.isfile(autogen):
		subprocess.check_call(
			["bash", "-c",
			 "export PKG_CONFIG_PATH='%s'; unset PKG_CONFIG_LIBDIR; ./autogen.sh %s"
			 % (pc, extra)],
			cwd=src, env=env)
	else:
		subprocess.check_call(["autoreconf", "-fi"], cwd=src, env=env)
		subprocess.check_call(
			["bash", "-c",
			 "export PKG_CONFIG_PATH='%s'; unset PKG_CONFIG_LIBDIR; ./configure %s"
			 % (pc, extra)],
			cwd=src, env=env)
	subprocess.check_call(["make", "-j", str(os.cpu_count() or 2)], cwd=src, env=env)
	subprocess.check_call(["make", "install"], cwd=src, env=env)


def pin_era_libs():
	"""Install libplist + libimobiledevice from 2025-11-24 (match 74e3bd9)."""
	if os.path.isfile(LIMD_MARKER):
		print("AQEMU: already pinned libimobiledevice %s" % LIMD_SHA[:8])
		return 0
	print("AQEMU: ONE-TIME libplist+libimobiledevice @ 2025-11-24 (CFS=false on newer .so)")
	print("AQEMU: later Restores skip this. A few minutes, needs git/autotools/network.")
	try:
		env = pkgconfig_env()
		git_checkout_sha(PLIST_GIT, PLIST_SRC, PLIST_SHA)
		print("AQEMU: autogen/make libplist %s..." % PLIST_SHA[:8])
		sys.stdout.flush()
		autogen_install(PLIST_SRC, env)
		env = ensure_pkgconfig()
		git_checkout_sha(LIMD_GIT, LIMD_SRC, LIMD_SHA)
		print("AQEMU: autogen/make libimobiledevice %s..." % LIMD_SHA[:8])
		sys.stdout.flush()
		autogen_install(LIMD_SRC, env, "--without-cython")
		subprocess.call(["ldconfig"])
	except (subprocess.CalledProcessError, OSError) as e:
		print("AQEMU: FATAL: era lib install failed: %s" % e)
		return 1
	try:
		d = os.path.dirname(LIMD_MARKER)
		if d and not os.path.isdir(d):
			os.makedirs(d)
		open(LIMD_MARKER, "w").write(LIMD_SHA + "\n")
	except OSError:
		pass
	print("AQEMU: pinned libimobiledevice %s (ldconfig done)" % LIMD_SHA[:8])
	return 0


def binary_has_dump(binary):
	try:
		return DUMP_TAG.encode("ascii") in open(binary, "rb").read()
	except OSError:
		return False


def pin_74e3bd9(binary):
	if pin_era_libs() != 0:
		return 1
	ver = idr_version(binary)
	print("AQEMU: current idevicerestore --version:\n%s" % ver.strip())
	blob = b""
	if os.path.isfile(binary):
		blob = open(binary, "rb").read()
	need_idr = (PIN_VER not in ver
		or MATCH_TAG.encode("ascii") not in blob
		or USB_TCP_TAG.encode("ascii") not in blob)
	if not need_idr:
		print("AQEMU: already ChefKiss-era %s with %s - not rebuilding" % (PIN_VER, USB_TCP_TAG))
		return 0
	if PIN_VER in ver:
		print("AQEMU: rebuilding 74e3bd9 once (USB-TCP v4: top-level CFS + integer 1)")
	print("AQEMU: ONE-TIME install of idevicerestore %s (Inferno GPT 78 / CFS=false)" % PIN_VER)
	print("AQEMU: later Restores skip this. Needs git/autotools/network on companion.")
	try:
		if os.path.isdir(PIN_SRC):
			shutil.rmtree(PIN_SRC)
		os.makedirs(PIN_SRC)
		run(["git", "init"], cwd=PIN_SRC)
		run(["git", "remote", "add", "origin", IDR_GIT], cwd=PIN_SRC)
		try:
			run(["git", "fetch", "--depth", "1", "origin", PIN_SHA], cwd=PIN_SRC)
		except subprocess.CalledProcessError:
			print("AQEMU: shallow fetch failed; trying full clone")
			shutil.rmtree(PIN_SRC)
			run(["git", "clone", IDR_GIT, PIN_SRC])
			run(["git", "checkout", PIN_SHA], cwd=PIN_SRC)
		else:
			run(["git", "checkout", "FETCH_HEAD"], cwd=PIN_SRC)
		if not apply_chefkiss(PIN_SRC):
			return 1
		if not apply_match_219(PIN_SRC):
			return 1
		if not apply_usb_tcp_cfs(PIN_SRC):
			return 1
		if os.path.isfile(binary):
			bak_old = binary + ".aqemu-pre-74e3bd9"
			if not os.path.isfile(bak_old):
				shutil.copy2(binary, bak_old)
				print("AQEMU: saved previous binary as %s" % bak_old)
		cfg_env = ensure_pkgconfig()
		print("AQEMU: autogen/make (a few minutes, once)...")
		sys.stdout.flush()
		autogen = os.path.join(PIN_SRC, "autogen.sh")
		pc = cfg_env.get("PKG_CONFIG_PATH", "/usr/local/lib/pkgconfig")
		if os.path.isfile(autogen):
			subprocess.check_call(
				["bash", "-c",
				 "export PKG_CONFIG_PATH='%s'; unset PKG_CONFIG_LIBDIR; ./autogen.sh" % pc],
				cwd=PIN_SRC, env=cfg_env)
		else:
			subprocess.check_call(["autoreconf", "-fi"], cwd=PIN_SRC, env=cfg_env)
			subprocess.check_call(["./configure"], cwd=PIN_SRC, env=cfg_env)
		subprocess.check_call(["make", "-j", str(os.cpu_count() or 2)], cwd=PIN_SRC, env=cfg_env)
		subprocess.check_call(["make", "install"], cwd=PIN_SRC, env=cfg_env)
		subprocess.call(["ldconfig"])
	except (subprocess.CalledProcessError, OSError) as e:
		print("AQEMU: FATAL: 74e3bd9 install failed: %s" % e)
		print("AQEMU: companion needs git, autotools, and network (github.com).")
		return 1
	ver2 = idr_version(binary)
	print("AQEMU: installed idevicerestore --version:\n%s" % ver2.strip())
	if PIN_VER not in ver2:
		print("AQEMU: FATAL: install did not yield %s" % PIN_VER)
		return 1
	try:
		d = os.path.dirname(MARKER)
		if d and not os.path.isdir(d):
			os.makedirs(d)
		open(MARKER, "w").write(PIN_SHA + "\n")
	except OSError:
		pass
	print("AQEMU: pinned %s - this Restore will use it (no rebuild next time)" % PIN_VER)
	return 0


def load_phdrs(data):
	if data[:4] != b"\x7fELF" or data[4] != 2:
		return []
	phoff = struct.unpack_from("<Q", data, 32)[0]
	phentsize = struct.unpack_from("<H", data, 54)[0]
	phnum = struct.unpack_from("<H", data, 56)[0]
	phdrs = []
	for n in range(phnum):
		o = phoff + n * phentsize
		p_type = struct.unpack_from("<I", data, o)[0]
		if p_type != 1:
			continue
		p_offset = struct.unpack_from("<Q", data, o + 8)[0]
		p_vaddr = struct.unpack_from("<Q", data, o + 16)[0]
		p_filesz = struct.unpack_from("<Q", data, o + 32)[0]
		phdrs.append((p_offset, p_vaddr, p_filesz))
	return phdrs


def file_to_va(phdrs, off):
	for p_offset, p_vaddr, p_filesz in phdrs:
		if p_offset <= off < p_offset + p_filesz:
			return p_vaddr + (off - p_offset)
	return None


def va_to_file(phdrs, va):
	for p_offset, p_vaddr, p_filesz in phdrs:
		if p_vaddr <= va < p_vaddr + p_filesz:
			return p_offset + (va - p_vaddr)
	return None


def find_leas(data, target, phdrs):
	hits = []
	n = len(data)
	i = 0
	while i + 7 <= n:
		b0 = data[i]
		if b0 in (0x48, 0x4C) and data[i + 1] == 0x8D and (data[i + 2] & 0xC7) == 0x05:
			disp = struct.unpack_from("<i", data, i + 3)[0]
			rip_va = file_to_va(phdrs, i + 7)
			if rip_va is not None:
				tf = va_to_file(phdrs, rip_va + disp)
				if tf == target:
					hits.append(i)
			elif i + 7 + disp == target:
				hits.append(i)
		i += 1
	return hits


def cfs_bool_at_lea(data, lea):
	"""Return (state, patch_off) for the plist_new_bool that feeds this LEA."""
	# mov edi/esi/eax, imm32 ; call rel32 ; lea rsi, CFS
	if lea >= 10 and data[lea - 5] == 0xE8 and data[lea - 10] in (0xBF, 0xBE, 0xB8):
		imm = struct.unpack_from("<I", data, lea - 9)[0]
		if imm == 0:
			return "false", lea - 10
		if imm == 1:
			return "true", lea - 10
	# xor edi,edi / xor eax,eax ; call ; lea
	if lea >= 7 and data[lea - 5] == 0xE8:
		if lea >= 7 and data[lea - 7 : lea - 5] == b"\x31\xff":
			return "false_xor", lea - 7
		if lea >= 9 and data[lea - 9 : lea - 5] in (b"\x31\xc0\x89\xc7", b"\x31\xf6\x89\xf7"):
			return "false_xor", lea - 9
	# and eax/edi, FLAG_ERASE  (0x01/0x02/0x04/0x08) shortly before the call
	if lea >= 8 and data[lea - 5] == 0xE8:
		start = max(0, lea - 24)
		i = start
		while i + 3 <= lea - 5:
			if data[i] == 0x83 and data[i + 1] in (0xE0, 0xE7) and data[i + 2] in (0x01, 0x02, 0x04, 0x08):
				return "flag_erase_and", i
			i += 1
	return "unknown", None


def patch_at_lea(data, lea):
	state, off = cfs_bool_at_lea(data, lea)
	pre = bytes(data[max(0, lea - 24) : lea + 24])
	print("AQEMU: LEA 0x%x around %s" % (lea, pre.hex()))
	if state == "true":
		print("AQEMU: LEA 0x%x: CFS is mov imm,1 immediately before plist_new_bool" % lea)
		return 0
	if state == "false" and off is not None:
		data[off + 1] = 1
		print("AQEMU: LEA 0x%x: patched mov imm32,0 -> 1 at 0x%x" % (lea, off))
		return 1
	if state == "false_xor" and off is not None:
		if data[off : off + 2] == b"\x31\xff":
			data[off] = 0xB0
			data[off + 1] = 0x01
			print("AQEMU: LEA 0x%x: patched xor edi,edi -> mov al,1 at 0x%x" % (lea, off))
			return 1
		data[off : off + 4] = b"\x6a\x01\x5f\x90"
		print("AQEMU: LEA 0x%x: patched xor->true at 0x%x" % (lea, off))
		return 1
	if state == "flag_erase_and" and off is not None:
		data[off + 1] = 0xC8
		data[off + 2] = 0x01
		print("AQEMU: LEA 0x%x: patched FLAG_ERASE and -> or $1 at 0x%x" % (lea, off))
		return 1
	print("AQEMU: LEA 0x%x: CFS bool arg %s (no patch)" % (lea, state))
	return 0


def analyze(data, phdrs, key_off):
	leas = find_leas(data, key_off, phdrs)
	if not leas:
		return "no_lea", leas
	states = [cfs_bool_at_lea(data, lea)[0] for lea in leas]
	if all(s == "true" for s in states):
		return "ok", leas
	if any(s in ("false", "false_xor", "flag_erase_and") for s in states):
		return "needs_patch", leas
	return "unknown", leas


def first_call_after(data, lea, limit=24):
	start = lea + 7
	end = min(len(data), lea + limit)
	i = start
	while i + 5 <= end:
		if data[i] == 0xE8:
			return i
		i += 1
	return None


def nop_first_call_after(data, lea, label):
	"""NOP only the first call shortly after this key's LEA (that key's set_item).

	The old 'last call in 72 bytes' window overlapped the next keys and NOP'd
	UUID / CreateFilesystemPartitions. Do not use that.
	"""
	off = first_call_after(data, lea, 24)
	if off is None:
		print("AQEMU: no nearby call after LEA 0x%x (%s)" % (lea, label))
		return 0
	if data[off : off + 5] == b"\x90\x90\x90\x90\x90":
		print("AQEMU: %s set_item already NOP at 0x%x" % (label, off))
		return 0
	data[off : off + 5] = b"\x90\x90\x90\x90\x90"
	print("AQEMU: NOP first set_item for %s at 0x%x" % (label, off))
	return 1


def uuid_set_item_intact(data, phdrs, cfs_leas):
	"""True if a UUID LEA near CFS still has a real call (not NOP'd)."""
	off = 0
	near = cfs_leas[0] if cfs_leas else 0
	while True:
		off = data.find(b"UUID\x00", off)
		if off < 0:
			print("AQEMU: UUID string not in binary (cannot verify set_item)")
			return True
		leas = find_leas(data, off, phdrs)
		for lea in leas:
			if abs(lea - near) > 0x800:
				continue
			call = first_call_after(data, lea, 24)
			if call is None:
				print("AQEMU: UUID LEA 0x%x has no nearby call" % lea)
				return False
			if data[call : call + 5] == b"\x90\x90\x90\x90\x90":
				print("AQEMU: UUID set_item was NOP'd at 0x%x (refusing this patch)" % call)
				return False
			print("AQEMU: UUID set_item still present at 0x%x" % call)
			return True
		off += 1
	return True


def strip_ios18_keys(data, phdrs):
	# SupportedAsyncDataTypes sits immediately before UUID/CFS. Never NOP it.
	n = 0
	for label in (
		b"SystemImageFormat\x00",
		b"HostHasFixFor99053849\x00",
		b"WaitForDeviceConnectionToFinishStateMachine\x00",
	):
		off = data.find(label)
		if off < 0:
			print("AQEMU: %s not in binary (ok)" % label[:-1].decode("ascii"))
			continue
		leas = find_leas(data, off, phdrs)
		print("AQEMU: %s at 0x%x, %d LEA(s)" % (label[:-1].decode("ascii"), off, len(leas)))
		for lea in leas:
			n += nop_first_call_after(data, lea, label[:-1].decode("ascii"))
	print("AQEMU: skipping SupportedAsyncDataTypes NOP (too close to UUID/CFS)")
	return n


def write_marker(tag):
	try:
		d = os.path.dirname(MARKER)
		if d and not os.path.isdir(d):
			os.makedirs(d)
		open(MARKER, "w").write(tag + "\n")
	except OSError as e:
		print("AQEMU: marker write failed: %s" % e)


def main():
	args = [a for a in sys.argv[1:] if not a.startswith("-")]
	binary = args[0] if args else DEFAULT_BIN

	# Do NOT restore .aqemu.bak (that was 45145e9 and would undo 74e3bd9).
	if pin_74e3bd9(binary) != 0:
		return 5

	if not os.path.isfile(binary):
		print("AQEMU: missing %s after pin" % binary)
		return 1

	data = bytearray(open(binary, "rb").read())
	key_off = data.find(KEY)
	if key_off < 0:
		print("AQEMU: CreateFilesystemPartitions string not in %s" % binary)
		return 1

	phdrs = load_phdrs(data)
	status, leas = analyze(data, phdrs, key_off)
	print("AQEMU: CFS string at 0x%x, %d LEA(s), status=%s" % (key_off, len(leas), status))
	try:
		out = subprocess.check_output(
			["nm", "-D", binary], stderr=subprocess.STDOUT
		).decode("ascii", "replace")
		for line in out.splitlines():
			if "plist_dict_set_item" in line or "restored_start_restore" in line:
				print("AQEMU: nm -D %s" % line.strip())
	except Exception as e:
		print("AQEMU: nm -D skipped: %s" % e)

	print("AQEMU: skipping binary CFS/iOS18 NOPs (74e3bd9 is used as-is)")
	write_marker("pinned-%s" % PIN_VER)
	# Ubuntu fs.protected_regular: root-owned wrap blocks later scp as bob.
	subprocess.call(["sudo", "-n", "rm", "-f", "/tmp/aqemu-idr-wrap.sh"])
	return 0


if __name__ == "__main__":
	sys.exit(main())
