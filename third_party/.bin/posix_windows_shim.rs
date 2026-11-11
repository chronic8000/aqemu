//! Linux `libc` calls the host VM uses, implemented with Win32.
//!
//! The Linux modules are the Vulkan host on Windows (`super` is compiled for both). They call
//! `mmap`, `memfd_create` and `clock_gettime` through a `libc` name; this module is imported as
//! `libc` on Windows so those calls resolve here. dma-buf ioctls are not implemented: that module
//! is Unix-only, and scheduler hints fail closed.

#![allow(non_camel_case_types)]
#![allow(clippy::missing_safety_doc)]

use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};

pub use libc::{c_char, c_int, c_long, c_ulong, c_void};

pub type off_t = i64;
pub type clockid_t = i32;

#[derive(Clone, Copy)]
pub struct timespec {
    pub tv_sec: i64,
    pub tv_nsec: i64,
}

#[derive(Clone, Copy)]
pub struct rlimit {
    pub rlim_cur: u64,
    pub rlim_max: u64,
}

#[derive(Clone, Copy)]
pub struct iovec {
    pub iov_base: *mut c_void,
    pub iov_len: usize,
}

pub const PROT_NONE: c_int = 0;
pub const PROT_READ: c_int = 1;
pub const PROT_WRITE: c_int = 2;
pub const PROT_EXEC: c_int = 4;

pub const MAP_SHARED: c_int = 0x01;
pub const MAP_PRIVATE: c_int = 0x02;
pub const MAP_FIXED: c_int = 0x10;
pub const MAP_ANONYMOUS: c_int = 0x20;
pub const MAP_NORESERVE: c_int = 0x4000;

pub const MAP_FAILED: *mut c_void = !0usize as *mut c_void;

pub const MFD_CLOEXEC: c_int = 0x0001;
pub const MFD_ALLOW_SEALING: c_int = 0x0002;
pub const O_CLOEXEC: c_int = 0x80000;

pub const F_ADD_SEALS: c_int = 1033;
pub const F_SEAL_GROW: c_int = 0x0004;
pub const F_SEAL_SHRINK: c_int = 0x0002;

pub const RLIMIT_NOFILE: c_int = 7;
pub const RLIMIT_AS: c_int = 9;

pub const ENOMEM: c_int = 12;
pub const EACCES: c_int = 13;
pub const EPERM: c_int = 1;
pub const EINVAL: c_int = 22;

pub const CLOCK_REALTIME: clockid_t = 0;
pub const CLOCK_BOOTTIME: clockid_t = 7;

pub const _SC_PAGESIZE: c_int = 30;
pub const _SC_PHYS_PAGES: c_int = 85;

const MEM_COMMIT: u32 = 0x1000;
const MEM_RESERVE: u32 = 0x2000;
const MEM_RELEASE: u32 = 0x8000;
const PAGE_NOACCESS: u32 = 0x01;
const PAGE_READONLY: u32 = 0x02;
const PAGE_READWRITE: u32 = 0x04;
const PAGE_EXECUTE_READ: u32 = 0x20;
const PAGE_EXECUTE_READWRITE: u32 = 0x40;
const FILE_MAP_READ: u32 = 0x0004;
const FILE_MAP_WRITE: u32 = 0x0002;
const GENERIC_READ: u32 = 0x8000_0000;
const GENERIC_WRITE: u32 = 0x4000_0000;
const FILE_SHARE_READ: u32 = 0x1;
const FILE_SHARE_WRITE: u32 = 0x2;
const OPEN_EXISTING: u32 = 3;
const FILE_ATTRIBUTE_TEMPORARY: u32 = 0x100;
const FILE_FLAG_DELETE_ON_CLOSE: u32 = 0x0400_0000;
const FILE_BEGIN: u32 = 0;
const INVALID_HANDLE: isize = -1;

#[repr(C)]
struct SystemInfo {
    oem_id: u32,
    page_size: u32,
    _rest: [u8; 64],
}

#[repr(C)]
struct MemoryStatusEx {
    length: u32,
    memory_load: u32,
    total_phys: u64,
    _rest: [u8; 48],
}

#[repr(C)]
struct MemoryBasicInformation {
    base_address: *mut c_void,
    allocation_base: *mut c_void,
    allocation_protect: u32,
    region_size: usize,
    state: u32,
    protect: u32,
    type_: u32,
}

#[link(name = "kernel32")]
unsafe extern "system" {
    fn SetLastError(code: u32);
    fn VirtualAlloc(addr: *mut c_void, size: usize, typ: u32, protect: u32) -> *mut c_void;
    fn VirtualFree(addr: *mut c_void, size: usize, typ: u32) -> i32;
    fn VirtualProtect(addr: *mut c_void, size: usize, protect: u32, old: *mut u32) -> i32;
    fn VirtualQuery(addr: *const c_void, info: *mut MemoryBasicInformation, len: usize) -> usize;
    fn GetSystemInfo(info: *mut SystemInfo);
    fn GlobalMemoryStatusEx(status: *mut MemoryStatusEx) -> i32;
    fn QueryPerformanceCounter(count: *mut i64) -> i32;
    fn QueryPerformanceFrequency(freq: *mut i64) -> i32;
    fn GetSystemTimePreciseAsFileTime(time: *mut u64);
    fn CreateFileW(
        name: *const u16,
        access: u32,
        share: u32,
        security: *mut c_void,
        disposition: u32,
        flags: u32,
        template: isize,
    ) -> isize;
    fn CreateFileMappingW(
        file: isize,
        security: *mut c_void,
        protect: u32,
        max_high: u32,
        max_low: u32,
        name: *const u16,
    ) -> isize;
    fn MapViewOfFileEx(
        mapping: isize,
        access: u32,
        offset_high: u32,
        offset_low: u32,
        bytes: usize,
        base: *mut c_void,
    ) -> *mut c_void;
    fn UnmapViewOfFile(base: *const c_void) -> i32;
    fn CloseHandle(handle: isize) -> i32;
    fn SetFilePointerEx(file: isize, distance: i64, new_offset: *mut i64, method: u32) -> i32;
    fn SetEndOfFile(file: isize) -> i32;
    fn GetTempPathW(len: u32, buffer: *mut u16) -> u32;
    fn GetTempFileNameW(path: *const u16, prefix: *const u16, unique: u32, out: *mut u16) -> u32;
}

#[link(name = "msvcrt")]
unsafe extern "C" {
    fn _open_osfhandle(handle: isize, flags: i32) -> i32;
    fn _get_osfhandle(fd: i32) -> isize;
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum Kind {
    Alloc,
    View,
}

static MAPS: LazyLock<Mutex<HashMap<usize, Kind>>> = LazyLock::new(|| Mutex::new(HashMap::new()));

fn maps() -> std::sync::MutexGuard<'static, HashMap<usize, Kind>> {
    MAPS.lock().unwrap_or_else(|poison| poison.into_inner())
}

fn fail(errno: c_int) -> *mut c_void {
    unsafe { SetLastError(errno as u32) };
    MAP_FAILED
}

fn page_protect(prot: c_int) -> u32 {
    let read = prot & PROT_READ != 0;
    let write = prot & PROT_WRITE != 0;
    let exec = prot & PROT_EXEC != 0;
    match (exec, write, read) {
        (true, true, _) => PAGE_EXECUTE_READWRITE,
        (true, false, true) => PAGE_EXECUTE_READ,
        (false, true, _) => PAGE_READWRITE,
        (false, false, true) => PAGE_READONLY,
        _ => PAGE_NOACCESS,
    }
}

fn release(addr: *mut c_void, kind: Kind) {
    unsafe {
        match kind {
            Kind::Alloc => {
                VirtualFree(addr, 0, MEM_RELEASE);
            }
            Kind::View => {
                UnmapViewOfFile(addr);
            }
        }
    }
}

/// # Safety
/// `addr` is a mapping this process made, or a null hint. `length` is the mapping's size.
pub unsafe fn mmap(
    addr: *mut c_void,
    length: usize,
    prot: c_int,
    flags: c_int,
    fd: c_int,
    offset: off_t,
) -> *mut c_void {
    if length == 0 {
        return fail(EINVAL);
    }
    let fixed = flags & MAP_FIXED != 0;
    if fixed {
        if let Some(kind) = maps().remove(&(addr as usize)) {
            release(addr, kind);
        }
    }
    let hint = if fixed { addr } else { std::ptr::null_mut() };
    if flags & MAP_ANONYMOUS != 0 || fd < 0 {
        let mapped = unsafe {
            VirtualAlloc(hint, length, MEM_RESERVE | MEM_COMMIT, page_protect(prot))
        };
        if mapped.is_null() {
            return fail(ENOMEM);
        }
        maps().insert(mapped as usize, Kind::Alloc);
        return mapped;
    }
    let handle = unsafe { _get_osfhandle(fd) };
    if handle == INVALID_HANDLE {
        return fail(EINVAL);
    }
    let mapping = unsafe {
        CreateFileMappingW(
            handle,
            std::ptr::null_mut(),
            PAGE_READWRITE,
            0,
            0,
            std::ptr::null(),
        )
    };
    if mapping == 0 || mapping == INVALID_HANDLE {
        return fail(ENOMEM);
    }
    let access = if prot & PROT_WRITE != 0 {
        FILE_MAP_READ | FILE_MAP_WRITE
    } else {
        FILE_MAP_READ
    };
    let off = offset as u64;
    let view = unsafe {
        MapViewOfFileEx(
            mapping,
            access,
            (off >> 32) as u32,
            off as u32,
            length,
            hint,
        )
    };
    unsafe { CloseHandle(mapping) };
    if view.is_null() {
        return fail(ENOMEM);
    }
    maps().insert(view as usize, Kind::View);
    view
}

/// # Safety
/// `addr` is the start of a mapping from [`mmap`] and `length` is that mapping's size.
pub unsafe fn munmap(addr: *mut c_void, _length: usize) -> c_int {
    let kind = maps().remove(&(addr as usize));
    match kind {
        Some(kind) => {
            release(addr, kind);
            0
        }
        None => {
            let unmapped = unsafe { UnmapViewOfFile(addr) };
            if unmapped != 0 {
                return 0;
            }
            let freed = unsafe { VirtualFree(addr, 0, MEM_RELEASE) };
            if freed != 0 {
                return 0;
            }
            unsafe { SetLastError(EINVAL as u32) };
            -1
        }
    }
}

/// # Safety
/// `addr` and `length` name a mapping this process may change the protection of.
pub unsafe fn mprotect(addr: *mut c_void, length: usize, prot: c_int) -> c_int {
    let mut old = 0u32;
    let ok = unsafe { VirtualProtect(addr, length, page_protect(prot), &mut old) };
    if ok == 0 {
        -1
    } else {
        0
    }
}

/// # Safety
/// `name` is a NUL-terminated string, or null. The returned descriptor is owned by the caller.
pub unsafe fn memfd_create(_name: *const c_char, _flags: c_int) -> c_int {
    let mut dir = [0u16; 261];
    let n = unsafe { GetTempPathW(dir.len() as u32, dir.as_mut_ptr()) };
    if n == 0 || n as usize >= dir.len() {
        unsafe { SetLastError(ENOMEM as u32) };
        return -1;
    }
    let prefix = [b'r' as u16, b'v' as u16, b'g' as u16, 0];
    let mut file = [0u16; 261];
    if unsafe { GetTempFileNameW(dir.as_ptr(), prefix.as_ptr(), 0, file.as_mut_ptr()) } == 0 {
        return -1;
    }
    let handle = unsafe {
        CreateFileW(
            file.as_ptr(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            std::ptr::null_mut(),
            OPEN_EXISTING,
            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
            0,
        )
    };
    if handle == INVALID_HANDLE {
        return -1;
    }
    let fd = unsafe { _open_osfhandle(handle, 0) };
    if fd < 0 {
        unsafe { CloseHandle(handle) };
        unsafe { SetLastError(ENOMEM as u32) };
    }
    fd
}

/// # Safety
/// `fd` is an open CRT descriptor.
pub unsafe fn ftruncate(fd: c_int, length: off_t) -> c_int {
    let handle = unsafe { _get_osfhandle(fd) };
    if handle == INVALID_HANDLE {
        unsafe { SetLastError(EINVAL as u32) };
        return -1;
    }
    if unsafe { SetFilePointerEx(handle, length, std::ptr::null_mut(), FILE_BEGIN) } == 0 {
        return -1;
    }
    if unsafe { SetEndOfFile(handle) } == 0 {
        return -1;
    }
    0
}

/// # Safety
/// `fd` is an open CRT descriptor. Seal commands are accepted and do nothing.
pub unsafe fn fcntl(fd: c_int, cmd: c_int, _arg: c_int) -> c_int {
    if cmd == F_ADD_SEALS {
        let handle = unsafe { _get_osfhandle(fd) };
        if handle == INVALID_HANDLE {
            unsafe { SetLastError(EINVAL as u32) };
            return -1;
        }
        return 0;
    }
    unsafe { SetLastError(EINVAL as u32) };
    -1
}

/// # Safety
/// `limit` is a writable `rlimit`.
pub unsafe fn getrlimit(resource: c_int, limit: *mut rlimit) -> c_int {
    if limit.is_null() {
        return -1;
    }
    let cap = if resource == RLIMIT_NOFILE {
        1 << 20
    } else {
        u64::MAX / 4
    };
    unsafe {
        (*limit).rlim_cur = cap;
        (*limit).rlim_max = cap;
    }
    0
}

/// # Safety
/// `limit` is a readable `rlimit`. The limit is not enforced.
pub unsafe fn setrlimit(_resource: c_int, _limit: *const rlimit) -> c_int {
    0
}

pub fn getpid() -> c_int {
    0
}

/// # Safety
/// The iovec pointers are valid for the lengths they name. An unreadable remote page returns -1.
pub unsafe fn process_vm_readv(
    _pid: c_int,
    local: *const iovec,
    _local_count: c_ulong,
    remote: *const iovec,
    remote_count: c_ulong,
    _flags: c_ulong,
) -> isize {
    if local.is_null() || remote.is_null() {
        unsafe { SetLastError(EACCES as u32) };
        return -1;
    }
    let local = unsafe { &*local };
    let remotes = unsafe { std::slice::from_raw_parts(remote, remote_count as usize) };
    let mut dst = local.iov_base.cast::<u8>();
    let mut written = 0isize;
    for region in remotes {
        if !readable(region.iov_base, region.iov_len) {
            unsafe { SetLastError(EACCES as u32) };
            return -1;
        }
        unsafe {
            std::ptr::copy_nonoverlapping(region.iov_base.cast::<u8>(), dst, region.iov_len);
            dst = dst.add(region.iov_len);
        }
        written += region.iov_len as isize;
    }
    written
}

fn readable(addr: *const c_void, len: usize) -> bool {
    if len == 0 {
        return true;
    }
    let mut left = len;
    let mut at = addr as usize;
    while left > 0 {
        let mut info = MemoryBasicInformation {
            base_address: std::ptr::null_mut(),
            allocation_base: std::ptr::null_mut(),
            allocation_protect: 0,
            region_size: 0,
            state: 0,
            protect: 0,
            type_: 0,
        };
        let queried = unsafe {
            VirtualQuery(
                at as *const c_void,
                &mut info,
                std::mem::size_of::<MemoryBasicInformation>(),
            )
        };
        if queried == 0 || info.state != MEM_COMMIT || info.region_size == 0 {
            return false;
        }
        let prot = info.protect & 0xff;
        let page_ok = matches!(prot, 0x02 | 0x04 | 0x08 | 0x20 | 0x40 | 0x80);
        if !page_ok || info.protect & 0x100 != 0 {
            return false;
        }
        let region_end = (info.base_address as usize).saturating_add(info.region_size);
        if region_end <= at {
            return false;
        }
        let span = region_end - at;
        if span >= left {
            return true;
        }
        left -= span;
        at += span;
    }
    true
}

/// # Safety
/// `out` is a writable `timespec`.
pub unsafe fn clock_gettime(clock: clockid_t, out: *mut timespec) -> c_int {
    if out.is_null() {
        return -1;
    }
    if clock == CLOCK_REALTIME {
        let mut ticks = 0u64;
        unsafe { GetSystemTimePreciseAsFileTime(&mut ticks) };
        let unix = ticks.saturating_sub(11_644_473_600_000_0000);
        unsafe {
            (*out).tv_sec = (unix / 10_000_000) as i64;
            (*out).tv_nsec = ((unix % 10_000_000) * 100) as i64;
        }
        return 0;
    }
    let mut counter = 0i64;
    let mut freq = 0i64;
    unsafe {
        QueryPerformanceCounter(&mut counter);
        QueryPerformanceFrequency(&mut freq);
    }
    if freq <= 0 {
        return -1;
    }
    let nanos = (counter as u128).saturating_mul(1_000_000_000) / freq as u128;
    unsafe {
        (*out).tv_sec = (nanos / 1_000_000_000) as i64;
        (*out).tv_nsec = (nanos % 1_000_000_000) as i64;
    }
    0
}

/// # Safety
/// A query. `_SC_PAGESIZE` and `_SC_PHYS_PAGES` are the only names that succeed.
pub unsafe fn sysconf(name: c_int) -> c_long {
    if name == _SC_PAGESIZE {
        let mut info = SystemInfo {
            oem_id: 0,
            page_size: 0,
            _rest: [0; 64],
        };
        unsafe { GetSystemInfo(&mut info) };
        return info.page_size as c_long;
    }
    if name == _SC_PHYS_PAGES {
        let mut status = MemoryStatusEx {
            length: std::mem::size_of::<MemoryStatusEx>() as u32,
            memory_load: 0,
            total_phys: 0,
            _rest: [0; 48],
        };
        if unsafe { GlobalMemoryStatusEx(&mut status) } == 0 || status.total_phys == 0 {
            return -1;
        }
        let mut info = SystemInfo {
            oem_id: 0,
            page_size: 0,
            _rest: [0; 64],
        };
        unsafe { GetSystemInfo(&mut info) };
        let page = u64::from(info.page_size.max(1));
        return (status.total_phys / page) as c_long;
    }
    -1
}


