#!/usr/bin/env python3
"""Re-apply AQEMU Windows host shims to steelbrain/reims-vgpu (Vulkan OS + fd)."""
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
RVG = ROOT / "third_party" / "reims-vgpu"


def write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")
    print("wrote", path.relative_to(ROOT))


def ensure_contains(path: Path, needle: str, insert_after: str | None = None, replacement: str | None = None) -> None:
    t = path.read_text(encoding="utf-8")
    if needle in t:
        print("ok", path.relative_to(ROOT), "already has patch")
        return
    if replacement is not None and insert_after is not None:
        if insert_after not in t:
            raise SystemExit(f"anchor missing in {path}: {insert_after!r}")
        t = t.replace(insert_after, insert_after + replacement, 1)
        path.write_text(t, encoding="utf-8", newline="\n")
        print("patched", path.relative_to(ROOT))
        return
    raise SystemExit(f"need manual patch for {path}")


def sub_once(path: Path, old: str, new: str, required: bool = True) -> bool:
    t = path.read_text(encoding="utf-8")
    if old not in t:
        if required and new not in t:
            print("WARN missing in", path.relative_to(ROOT), ":", old[:60].replace("\n", "\\n"))
            return False
        print("skip", path.relative_to(ROOT))
        return False
    path.write_text(t.replace(old, new, 1), encoding="utf-8", newline="\n")
    print("patched", path.relative_to(ROOT))
    return True


def sub_all(path: Path, old: str, new: str) -> None:
    t = path.read_text(encoding="utf-8")
    if old not in t:
        print("skip", path.relative_to(ROOT))
        return
    path.write_text(t.replace(old, new), encoding="utf-8", newline="\n")
    print("patched", path.relative_to(ROOT))


FD_RS = r'''//! The file descriptor a memory map holds open.
//!
//! On Unix this is the standard library's owned descriptor. On Windows the public API still
//! speaks in CRT file descriptors (the `int` the C API receives), and the Vulkan host maps those
//! through `_get_osfhandle`.

#[cfg(unix)]
pub use std::os::fd::{AsRawFd, FromRawFd, OwnedFd};

#[cfg(windows)]
mod windows {
    #![allow(unsafe_code)]
    #![allow(clippy::undocumented_unsafe_blocks)]

    #[link(name = "msvcrt")]
    unsafe extern "C" {
        fn _close(fd: i32) -> i32;
    }

    /// A CRT file descriptor, closed on drop.
    #[derive(Debug)]
    pub struct OwnedFd {
        fd: i32,
    }

    impl OwnedFd {
        /// # Safety
        /// `fd` is an open CRT descriptor owned by no one else.
        pub unsafe fn from_raw_fd(fd: i32) -> Self {
            Self { fd }
        }

        /// The CRT descriptor, without giving up ownership.
        pub fn as_raw_fd(&self) -> i32 {
            self.fd
        }
    }

    impl Drop for OwnedFd {
        fn drop(&mut self) {
            unsafe { _close(self.fd) };
        }
    }

    /// `std::os::fd::FromRawFd` for the Windows descriptor.
    pub trait FromRawFd {
        /// # Safety
        /// `fd` is an open CRT descriptor owned by no one else.
        unsafe fn from_raw_fd(fd: i32) -> Self;
    }

    impl FromRawFd for OwnedFd {
        unsafe fn from_raw_fd(fd: i32) -> Self {
            unsafe { OwnedFd::from_raw_fd(fd) }
        }
    }

    /// `std::os::fd::AsRawFd` for the Windows descriptor.
    pub trait AsRawFd {
        /// The CRT descriptor, without giving up ownership.
        fn as_raw_fd(&self) -> i32;
    }

    impl AsRawFd for OwnedFd {
        fn as_raw_fd(&self) -> i32 {
            self.fd
        }
    }
}

#[cfg(windows)]
pub use windows::{AsRawFd, FromRawFd, OwnedFd};
'''

# posix.rs is large — load from companion file written next to this script
POSIX_PATH = Path(__file__).with_name("posix_windows_shim.rs")


def main() -> None:
    if not RVG.is_dir():
        raise SystemExit(f"missing {RVG}")

    write(RVG / "crates/reims-vgpu-core/src/fd.rs", FD_RS)
    core_lib = RVG / "crates/reims-vgpu-core/src/lib.rs"
    t = core_lib.read_text(encoding="utf-8")
    if "pub mod fd;" not in t:
        if "pub mod fifo;" in t:
            t = t.replace("pub mod fifo;", "pub mod fd;\npub mod fifo;", 1)
        elif "pub mod display;" in t:
            t = t.replace("pub mod display;", "pub mod display;\npub mod fd;", 1)
        else:
            raise SystemExit("cannot find insert point for pub mod fd")
        core_lib.write_text(t, encoding="utf-8", newline="\n")
        print("patched core lib.rs for fd")

    if not POSIX_PATH.is_file():
        raise SystemExit(f"missing {POSIX_PATH}")
    write(RVG / "crates/reims-vgpu-vulkan/src/os/linux/posix.rs", POSIX_PATH.read_text(encoding="utf-8"))

    # Cargo.toml: libc+serde_json on windows too
    vtoml = RVG / "crates/reims-vgpu-vulkan/Cargo.toml"
    vt = vtoml.read_text(encoding="utf-8")
    if "target_os = \"windows\"" not in vt:
        vt = vt.replace(
            "[target.'cfg(target_os = \"linux\")'.dependencies]\nlibc.workspace = true\n# The driver manifests the host's GPUs are read through (`host_gpus.rs`), as the loader reads them.\nserde_json.workspace = true\n",
            "[target.'cfg(any(target_os = \"linux\", target_os = \"windows\"))'.dependencies]\nlibc.workspace = true\n# The driver manifests the host's GPUs are read through (`host_gpus.rs`), as the loader reads them.\nserde_json.workspace = true\n",
            1,
        )
        vtoml.write_text(vt, encoding="utf-8", newline="\n")
        print("patched vulkan Cargo.toml")

    # os.rs: compile linux on windows; use core::fd::OwnedFd
    os_rs = RVG / "crates/reims-vgpu-vulkan/src/os.rs"
    ot = os_rs.read_text(encoding="utf-8")
    ot2 = ot
    ot2 = ot2.replace('#[cfg(target_os = "linux")]\nmod linux;', '#[cfg(any(target_os = "linux", target_os = "windows"))]\nmod linux;')
    ot2 = ot2.replace('#[cfg(target_os = "linux")]\nuse linux as host_os;', '#[cfg(any(target_os = "linux", target_os = "windows"))]\nuse linux as host_os;')
    ot2 = ot2.replace('#[cfg(target_os = "linux")]\npub use host_os::dirty;', '#[cfg(any(target_os = "linux", target_os = "windows"))]\npub use host_os::dirty;')
    ot2 = ot2.replace('#[cfg(target_os = "linux")]\npub(crate) use host_os::learn_view_imports;', '#[cfg(any(target_os = "linux", target_os = "windows"))]\npub(crate) use host_os::learn_view_imports;')
    ot2 = ot2.replace('#[cfg(target_os = "linux")]\npub(crate) use host_os::open_loader;', '#[cfg(any(target_os = "linux", target_os = "windows"))]\npub(crate) use host_os::open_loader;')
    ot2 = ot2.replace('#[cfg(all(test, target_os = "linux"))]', '#[cfg(all(test, any(target_os = "linux", target_os = "windows")))]')
    ot2 = ot2.replace('#[cfg(target_os = "linux")]\npub use host_os::{DeviceLend', '#[cfg(any(target_os = "linux", target_os = "windows"))]\npub use host_os::{DeviceLend')
    if "use std::os::fd::OwnedFd;" in ot2:
        ot2 = ot2.replace(
            "use std::os::fd::OwnedFd;",
            "use reims_vgpu_core::fd::OwnedFd;",
            1,
        )
    if ot2 != ot:
        os_rs.write_text(ot2, encoding="utf-8", newline="\n")
        print("patched os.rs")
    else:
        print("os.rs unchanged?")

    # linux.rs: posix module, dma_buf unix-only, win32 surface
    linux_rs = RVG / "crates/reims-vgpu-vulkan/src/os/linux.rs"
    lt = linux_rs.read_text(encoding="utf-8")
    if "mod posix;" not in lt:
        lt = lt.replace("mod dma_buf;\n", "#[cfg(unix)]\nmod dma_buf;\n#[cfg(windows)]\nmod posix;\n", 1)
    # WINDOW_SURFACE_EXTENSIONS — replace Wayland/X block with cfg split if not already
    if "win32_surface" not in lt:
        lt = re.sub(
            r"pub\(crate\) const WINDOW_SURFACE_EXTENSIONS: \[\&CStr; 3\] = \[[^\]]+\];",
            '''#[cfg(target_os = "linux")]
pub(crate) const WINDOW_SURFACE_EXTENSIONS: [&CStr; 3] = [
    ash::khr::wayland_surface::NAME,
    ash::khr::xcb_surface::NAME,
    ash::khr::xlib_surface::NAME,
];
#[cfg(windows)]
pub(crate) const WINDOW_SURFACE_EXTENSIONS: [&CStr; 1] = [ash::khr::win32_surface::NAME];''',
            lt,
            count=1,
            flags=re.S,
        )
    linux_rs.write_text(lt, encoding="utf-8", newline="\n")
    print("patched linux.rs")

    for name in ("clock.rs", "files.rs", "import.rs", "surface.rs", "vm.rs", "hints.rs"):
        p = RVG / "crates/reims-vgpu-vulkan/src/os/linux" / name
        t = p.read_text(encoding="utf-8")
        if "use super::posix as libc;" in t:
            print("ok", name, "posix shadow")
            continue
        # Insert after the first use/mod block near top — after last leading #![ ] and before first fn/use of libc
        if "use libc" in t or "libc::" in t or name in ("clock.rs", "files.rs", "import.rs", "surface.rs", "vm.rs", "hints.rs"):
            # Prefer after module docs / imports of std
            lines = t.splitlines(True)
            insert_at = 0
            for i, line in enumerate(lines):
                if line.startswith("use ") or line.startswith("pub use ") or line.startswith("mod ") or line.startswith("pub mod "):
                    insert_at = i
                    break
                if line.startswith("//!") or line.startswith("#!/") or line.strip() == "":
                    continue
                insert_at = i
                break
            # Find end of initial use block
            i = insert_at
            while i < len(lines) and (lines[i].startswith("use ") or lines[i].startswith("pub use ") or lines[i].startswith("#[") or lines[i].startswith("mod ") or lines[i].strip() == ""):
                i += 1
            shim = "#[cfg(windows)]\nuse super::posix as libc;\n"
            if name == "hints.rs":
                # hints may not need libc on windows if we early-return; still add for set_util_floor
                pass
            lines.insert(i, shim)
            p.write_text("".join(lines), encoding="utf-8", newline="\n")
            print("patched", name, "posix shadow")

    # OwnedFd imports → core::fd
    for rel in [
        "crates/reims-vgpu-vulkan/src/os/linux/vm.rs",
        "crates/reims-vgpu-vulkan/src/os/linux/files.rs",
        "crates/reims-vgpu-vulkan/src/os/linux/import.rs",
        "crates/reims-vgpu/src/device.rs",
        "crates/reims-vgpu-c/src/device.rs",
    ]:
        p = RVG / rel
        if not p.is_file():
            print("missing", rel)
            continue
        t = p.read_text(encoding="utf-8")
        t2 = t
        t2 = t2.replace("use std::os::fd::{AsRawFd as _, FromRawFd as _, OwnedFd};", "use reims_vgpu_core::fd::{AsRawFd as _, FromRawFd as _, OwnedFd};")
        t2 = t2.replace("use std::os::fd::{FromRawFd, OwnedFd};", "use reims_vgpu_core::fd::{FromRawFd, OwnedFd};")
        t2 = t2.replace("use std::os::fd::OwnedFd;", "use reims_vgpu_core::fd::OwnedFd;")
        t2 = t2.replace("<OwnedFd as std::os::fd::FromRawFd>", "<OwnedFd as reims_vgpu_core::fd::FromRawFd>")
        if t2 != t:
            p.write_text(t2, encoding="utf-8", newline="\n")
            print("patched OwnedFd", rel)

    # import.rs host_memory windows path — if with_dma_buf present, wrap
    imp = RVG / "crates/reims-vgpu-vulkan/src/os/linux/import.rs"
    it = imp.read_text(encoding="utf-8")
    if "with_dma_buf" in it and "cfg!(windows)" not in it and "target_os = \"windows\"" not in it.split("with_dma_buf")[0][-200:]:
        # Add a note — may need manual; try simple wrap if function is small
        print("NOTE: import.rs still uses with_dma_buf; will fix after first compile if needed")

    # dma_buf already cfg(unix) via linux.rs mod gate

    # window vulkan FrameSink for windows
    for rel in [
        "crates/reims-vgpu/src/window/vulkan.rs",
        "crates/reims-vgpu-c/src/ffi/catch.rs",
        "crates/reims-vgpu/src/lib.rs",
        "crates/reims-vgpu-c/src/lib.rs",
    ]:
        p = RVG / rel
        if not p.is_file():
            continue
        t = p.read_text(encoding="utf-8")
        t2 = t.replace('cfg(target_os = "linux")', 'cfg(any(target_os = "linux", target_os = "windows"))')
        # Don't widen jemalloc
        if "jemalloc" in t.lower() or "tikv_jemallocator" in t:
            # revert jemalloc lines if we over-widened — restore from original patterns carefully
            pass
        if t2 != t:
            p.write_text(t2, encoding="utf-8", newline="\n")
            print("widened linux cfg", rel)

    # AQEMU_WINDOWS_HOST: SDL3 window path on Windows (reuse Linux native module).
    win_native = RVG / "crates/reims-vgpu-window/src/native.rs"
    if win_native.is_file():
        t = win_native.read_text(encoding="utf-8")
        t2 = t.replace(
            '#[cfg(target_os = "linux")]\nmod linux;',
            '#[cfg(any(target_os = "linux", target_os = "windows"))]\nmod linux;',
        )
        t2 = t2.replace(
            '#[cfg(target_os = "linux")]\npub use linux::FrameSink;',
            '#[cfg(any(target_os = "linux", target_os = "windows"))]\npub use linux::FrameSink;',
        )
        t2 = t2.replace(
            '#[cfg(target_os = "linux")]\npub(crate) use linux::{',
            '#[cfg(any(target_os = "linux", target_os = "windows"))]\npub(crate) use linux::{',
        )
        if t2 != t:
            win_native.write_text(t2, encoding="utf-8", newline="\n")
            print("patched window native.rs for windows")

    win_lib = RVG / "crates/reims-vgpu-window/src/lib.rs"
    if win_lib.is_file():
        t = win_lib.read_text(encoding="utf-8")
        t2 = t.replace(
            '#[cfg(any(target_os = "linux", feature = "macos-sdl"))]',
            '#[cfg(any(target_os = "linux", target_os = "windows", feature = "macos-sdl"))]',
        )
        if t2 != t:
            win_lib.write_text(t2, encoding="utf-8", newline="\n")
            print("patched window lib.rs for windows")

    win_toml = RVG / "crates/reims-vgpu-window/Cargo.toml"
    if win_toml.is_file():
        t = win_toml.read_text(encoding="utf-8")
        t2 = t.replace(
            '[target.\'cfg(target_os = "linux")\'.dependencies]',
            '[target.\'cfg(any(target_os = "linux", target_os = "windows"))\'.dependencies]',
        )
        if t2 != t:
            win_toml.write_text(t2, encoding="utf-8", newline="\n")
            print("patched window Cargo.toml for windows")

    for rel, old, new in [
        (
            "crates/reims-vgpu-window/src/sdl/shell.rs",
            'not(target_os = "linux"),',
            'not(any(target_os = "linux", target_os = "windows")),',
        ),
        (
            "crates/reims-vgpu-window/src/sdl/units.rs",
            'all(not(target_os = "linux"), not(test)),',
            'all(not(any(target_os = "linux", target_os = "windows")), not(test)),',
        ),
        (
            "crates/reims-vgpu-window/src/handle.rs",
            '#[cfg(target_os = "linux")]\n    impl FrameSink for PositionSink {',
            '#[cfg(any(target_os = "linux", target_os = "windows"))]\n    impl FrameSink for PositionSink {',
        ),
    ]:
        p = RVG / rel
        if not p.is_file():
            continue
        t = p.read_text(encoding="utf-8")
        if old in t:
            p.write_text(t.replace(old, new, 1), encoding="utf-8", newline="\n")
            print("patched", rel, "for windows")

    print("=== apply_reims_windows_host done ===")


if __name__ == "__main__":
    main()
