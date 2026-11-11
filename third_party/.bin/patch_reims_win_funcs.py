#!/usr/bin/env python3
from pathlib import Path

root = Path(r"C:\Users\chron\CURSOR-PROJECTS\AQEMU\aqemu\third_party\reims-vgpu")

# hints
p = root / "crates/reims-vgpu-vulkan/src/os/linux/hints.rs"
t = p.read_text(encoding="utf-8")
old = """fn set_util_floor(floor: u32) -> std::io::Result<()> {
    let request = util_floor_request(floor);
    // SAFETY: the calling thread's attributes, from a live `sched_attr` of the size it names.
    let rc = unsafe { libc::syscall(libc::SYS_sched_setattr, 0, std::ptr::from_ref(&request), 0) };
    if rc == 0 {
        Ok(())
    } else {
        Err(std::io::Error::last_os_error())
    }
}"""
new = """fn set_util_floor(floor: u32) -> std::io::Result<()> {
    #[cfg(windows)]
    {
        let _ = floor;
        return Err(std::io::Error::new(
            std::io::ErrorKind::Unsupported,
            "sched_setattr is Linux-only",
        ));
    }
    #[cfg(unix)]
    {
        let request = util_floor_request(floor);
        // SAFETY: the calling thread's attributes, from a live `sched_attr` of the size it names.
        let rc = unsafe { libc::syscall(libc::SYS_sched_setattr, 0, std::ptr::from_ref(&request), 0) };
        if rc == 0 {
            Ok(())
        } else {
            Err(std::io::Error::last_os_error())
        }
    }
}"""
if old in t:
    p.write_text(t.replace(old, new, 1), encoding="utf-8", newline="\n")
    print("patched hints")
else:
    print("hints already patched or shape changed")

# import host_memory_noting
p = root / "crates/reims-vgpu-vulkan/src/os/linux/import.rs"
t = p.read_text(encoding="utf-8")
if "AQEMU_WINDOWS_HOST: no dma-buf" in t:
    print("import already split")
else:
    marker = "unsafe fn host_memory_noting("
    idx = t.find(marker)
    if idx < 0:
        raise SystemExit("host_memory_noting not found")
    end = t.find("\nthread_local!", idx)
    if end < 0:
        raise SystemExit("end not found")
    body = t[idx:end]
    win = """/// AQEMU_WINDOWS_HOST: no dma-buf; the runtime copies guest pages.
#[cfg(windows)]
unsafe fn host_memory_noting(
    at: NonNull<u8>,
    logged: bool,
    asked: Option<Arc<std::sync::atomic::AtomicBool>>,
) -> rt::HostMemory {
    let _ = asked;
    static GATE: std::sync::LazyLock<rt::ImportGate> =
        std::sync::LazyLock::new(|| Arc::new(import_gate));
    static RUNS: std::sync::LazyLock<rt::RunResolver> =
        std::sync::LazyLock::new(|| Arc::new(super::host_ram::runs));
    // SAFETY: the caller's.
    let memory = unsafe { rt::HostMemory::new(at) }.with_import_gate(Arc::clone(&GATE));
    let memory = match imports_need_files() {
        true => memory.with_runs(Arc::clone(&RUNS)),
        false => memory,
    };
    let dirty = logged
        .then(|| super::dirty::ImportDirty::new(shown_at(at).as_ptr().addr().to_u64()))
        .flatten();
    match dirty {
        Some(dirty) => memory.with_dirty_pages(Arc::new(dirty)),
        None => memory,
    }
}

#[cfg(unix)]
""" + body
    p.write_text(t[:idx] + win + t[end:], encoding="utf-8", newline="\n")
    print("patched import host_memory_noting")

# device.rs
p = root / "crates/reims-vgpu-c/src/device.rs"
t = p.read_text(encoding="utf-8")
old = """fn host_page_size() -> NonZeroU64 {
    // SAFETY: a query of a constant; it reads and writes no memory of the caller's.
    let page = unsafe { libc::sysconf(libc::_SC_PAGESIZE) };
    u64::try_from(page)
        .ok()
        .and_then(NonZeroU64::new)
        .unwrap_or(NonZeroU64::MIN)
}

/// The size of the file `fd` names (`fstat`).
fn file_size(fd: i32) -> std::io::Result<u64> {
    let mut st = MaybeUninit::<libc::stat>::zeroed();
    // SAFETY: `st` is a writable `struct stat`; an `fd` that names no open file is an error the
    // call returns.
    if unsafe { libc::fstat(fd, st.as_mut_ptr()) } != 0 {
        return Err(std::io::Error::last_os_error());
    }
    // SAFETY: `fstat` succeeded, so it filled `st`; it was zeroed before.
    let st = unsafe { st.assume_init() };
    Ok(u64::try_from(st.st_size).unwrap_or(0))
}

/// A duplicate of `fd`, close-on-exec (`fcntl(F_DUPFD_CLOEXEC)`), which the map holds.
fn duplicate(fd: i32) -> std::io::Result<OwnedFd> {
    // SAFETY: `fcntl` reads and writes no memory of the caller's; an `fd` that names no open
    // file is an error the call returns.
    let new = unsafe { libc::fcntl(fd, libc::F_DUPFD_CLOEXEC, 0) };
    if new < 0 {
        return Err(std::io::Error::last_os_error());
    }
    // SAFETY: `new` is a descriptor this call opened, owned by nothing else.
    Ok(unsafe { OwnedFd::from_raw_fd(new) })
}"""
new = """#[cfg(unix)]
fn host_page_size() -> NonZeroU64 {
    // SAFETY: a query of a constant; it reads and writes no memory of the caller's.
    let page = unsafe { libc::sysconf(libc::_SC_PAGESIZE) };
    u64::try_from(page)
        .ok()
        .and_then(NonZeroU64::new)
        .unwrap_or(NonZeroU64::MIN)
}

#[cfg(windows)]
fn host_page_size() -> NonZeroU64 {
    #[repr(C)]
    struct SystemInfo {
        oem_id: u32,
        page_size: u32,
        _rest: [u8; 64],
    }
    #[link(name = "kernel32")]
    unsafe extern "system" {
        fn GetSystemInfo(info: *mut SystemInfo);
    }
    let mut info = SystemInfo {
        oem_id: 0,
        page_size: 0,
        _rest: [0; 64],
    };
    unsafe { GetSystemInfo(&mut info) };
    NonZeroU64::new(u64::from(info.page_size)).unwrap_or(NonZeroU64::MIN)
}

/// The size of the file `fd` names (`fstat`).
#[cfg(unix)]
fn file_size(fd: i32) -> std::io::Result<u64> {
    let mut st = MaybeUninit::<libc::stat>::zeroed();
    // SAFETY: `st` is a writable `struct stat`; an `fd` that names no open file is an error the
    // call returns.
    if unsafe { libc::fstat(fd, st.as_mut_ptr()) } != 0 {
        return Err(std::io::Error::last_os_error());
    }
    // SAFETY: `fstat` succeeded, so it filled `st`; it was zeroed before.
    let st = unsafe { st.assume_init() };
    Ok(u64::try_from(st.st_size).unwrap_or(0))
}

#[cfg(windows)]
fn file_size(fd: i32) -> std::io::Result<u64> {
    #[link(name = "msvcrt")]
    unsafe extern "C" {
        fn _get_osfhandle(fd: i32) -> isize;
    }
    #[link(name = "kernel32")]
    unsafe extern "system" {
        fn GetFileSizeEx(file: isize, size: *mut i64) -> i32;
    }
    let handle = unsafe { _get_osfhandle(fd) };
    if handle == -1 {
        return Err(std::io::Error::last_os_error());
    }
    let mut size = 0i64;
    if unsafe { GetFileSizeEx(handle, &mut size) } == 0 {
        return Err(std::io::Error::last_os_error());
    }
    Ok(u64::try_from(size).unwrap_or(0))
}

/// A duplicate of `fd`, close-on-exec (`fcntl(F_DUPFD_CLOEXEC)`), which the map holds.
#[cfg(unix)]
fn duplicate(fd: i32) -> std::io::Result<OwnedFd> {
    // SAFETY: `fcntl` reads and writes no memory of the caller's; an `fd` that names no open
    // file is an error the call returns.
    let new = unsafe { libc::fcntl(fd, libc::F_DUPFD_CLOEXEC, 0) };
    if new < 0 {
        return Err(std::io::Error::last_os_error());
    }
    // SAFETY: `new` is a descriptor this call opened, owned by nothing else.
    Ok(unsafe { OwnedFd::from_raw_fd(new) })
}

#[cfg(windows)]
fn duplicate(fd: i32) -> std::io::Result<OwnedFd> {
    #[link(name = "msvcrt")]
    unsafe extern "C" {
        fn _dup(fd: i32) -> i32;
    }
    let new = unsafe { _dup(fd) };
    if new < 0 {
        return Err(std::io::Error::last_os_error());
    }
    Ok(unsafe { OwnedFd::from_raw_fd(new) })
}"""
if old in t:
    p.write_text(t.replace(old, new, 1), encoding="utf-8", newline="\n")
    print("patched c device.rs")
else:
    print("device.rs shape changed")
    if "cfg(windows)]\nfn host_page_size" in t:
        print("  already has windows host_page_size")
