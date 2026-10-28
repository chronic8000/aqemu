#!/usr/bin/env python3
"""Cross-build and ad-hoc sign extras/Inferno/aqemu-register for iOS 14 arm64."""
import hashlib
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "aqemu-register.c")
OUT = os.path.join(HERE, "aqemu-register")
TBD = os.path.join(HERE, "libSystem.B.tbd")
PS_PATH = "/tmp/ios-ro/root/bin/ps"

PAGE = 4096
CSMAGIC_REQUIREMENTSET = 0xFADE0C01
CSMAGIC_CODEDIRECTORY = 0xFADE0C02
CSMAGIC_EMBEDDED_SIGNATURE = 0xFADE0CC0
CSMAGIC_EMBEDDED_ENTITLEMENTS = 0xFADE7171
CSMAGIC_BLOBWRAPPER = 0xFADE0B01
CSSLOT_CODEDIRECTORY = 0
CSSLOT_REQUIREMENTS = 2
CSSLOT_ENTITLEMENTS = 5
CSSLOT_SIGNATURESLOT = 0x10000
LC_SEGMENT_64 = 0x19
LC_CODE_SIGNATURE = 0x1D
LC_DYLD_INFO_ONLY = 0x80000022
LC_DYLD_CHAINED_FIXUPS = 0x80000034

ENTITLEMENTS_XML = b"""<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
\t<key>platform-application</key>
\t<true/>
\t<key>com.apple.private.skip-library-validation</key>
\t<true/>
</dict>
</plist>
"""
REQUIREMENTS = struct.pack(">III", CSMAGIC_REQUIREMENTSET, 12, 0)
EMPTY_SIG = struct.pack(">II", CSMAGIC_BLOBWRAPPER, 8)


def run(cmd):
    print("RUN", " ".join(cmd))
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.stdout:
        print(r.stdout)
    if r.returncode != 0:
        print(r.stderr)
        sys.exit(r.returncode)
    if r.stderr.strip():
        print(r.stderr)


def u32be(buf, off):
    return struct.unpack_from(">I", buf, off)[0]


def walk_cmds(data):
    ncmds, sizeofcmds = struct.unpack_from("<II", data, 16)
    off = 32
    cmds = []
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<II", data, off)
        cmds.append((cmd, cmdsize, off))
        off += cmdsize
    return cmds, sizeofcmds


def page_hashes(data, code_limit):
    out = []
    off = 0
    while off < code_limit:
        chunk = data[off:min(off + PAGE, code_limit)]
        out.append(hashlib.sha256(chunk).digest())
        off += PAGE
    return out


def parse_cd_hashes(blob, cd_off):
    magic, length, version, flags, hash_off, ident_off, n_special, n_code, code_limit = struct.unpack_from(
        ">IIIIIIIII", blob, cd_off
    )
    if magic != CSMAGIC_CODEDIRECTORY:
        raise SystemExit("not a code directory")
    hash_size = blob[cd_off + 36]
    base = cd_off + hash_off - n_special * hash_size
    hashes = []
    for i in range(n_special + n_code):
        start = base + i * hash_size
        hashes.append(blob[start:start + hash_size])
    return {
        "version": version,
        "flags": flags,
        "n_special": n_special,
        "n_code": n_code,
        "code_limit": code_limit,
        "hashes": hashes,
    }


def verify_stock_ps():
    if not os.path.isfile(PS_PATH):
        print("skip stock signature check; ps not mounted")
        return
    data = open(PS_PATH, "rb").read()
    cmds, _ = walk_cmds(data)
    sig = None
    for cmd, cmdsize, off in cmds:
        if cmd == LC_CODE_SIGNATURE:
            sig = struct.unpack_from("<II", data, off + 8)
    if not sig:
        raise SystemExit("ps has no code signature")
    blob = data[sig[0]:sig[0] + sig[1]]
    count = u32be(blob, 8)
    slots = {}
    for i in range(count):
        typ, off = struct.unpack_from(">II", blob, 12 + i * 8)
        slots[typ] = off
    cd = parse_cd_hashes(blob, slots[CSSLOT_CODEDIRECTORY])
    req = blob[slots[CSSLOT_REQUIREMENTS]:slots[CSSLOT_REQUIREMENTS] + u32be(blob, slots[CSSLOT_REQUIREMENTS] + 4)]
    ent = blob[slots[CSSLOT_ENTITLEMENTS]:slots[CSSLOT_ENTITLEMENTS] + u32be(blob, slots[CSSLOT_ENTITLEMENTS] + 4)]
    expect_req = hashlib.sha256(req).digest()
    expect_ent = hashlib.sha256(ent).digest()
    got_req = cd["hashes"][cd["n_special"] - 2]
    got_ent = cd["hashes"][0]
    if got_req != expect_req or got_ent != expect_ent:
        raise SystemExit("special-slot layout does not match this iOS image")
    code = page_hashes(data, cd["code_limit"])
    if code[0] != cd["hashes"][cd["n_special"]]:
        padded = hashlib.sha256(data[:PAGE].ljust(PAGE, b"\x00")).digest()
        if padded == cd["hashes"][cd["n_special"]]:
            raise SystemExit("stock binary uses zero-padded pages; signer must match")
        raise SystemExit("code slot 0 does not match raw page hash")
    if len(code) != cd["n_code"]:
        raise SystemExit("code slot count mismatch")
    for i, h in enumerate(code):
        if h != cd["hashes"][cd["n_special"] + i]:
            raise SystemExit("code slot %d mismatch" % i)
    print("stock ps signature algorithm matches")


def sign(data, ident):
    data = bytearray(data)
    cmds, sizeofcmds = walk_cmds(data)
    if any(cmd == LC_CODE_SIGNATURE for cmd, _, _ in cmds):
        raise SystemExit("binary already signed")
    if any(cmd == LC_DYLD_CHAINED_FIXUPS for cmd, _, _ in cmds):
        raise SystemExit("binary uses chained fixups, which iOS 14 dyld rejects")
    if not any(cmd == LC_DYLD_INFO_ONLY for cmd, _, _ in cmds):
        raise SystemExit("binary has no LC_DYLD_INFO_ONLY")
    link = None
    text_size = None
    for cmd, cmdsize, off in cmds:
        if cmd != LC_SEGMENT_64:
            continue
        name = data[off + 8:off + 24].split(b"\x00", 1)[0]
        if name == b"__LINKEDIT":
            link = off
        if name == b"__TEXT":
            text_size = struct.unpack_from("<Q", data, off + 48)[0]  # filesize
    if link is None or text_size is None:
        raise SystemExit("missing __TEXT or __LINKEDIT")
    fileoff, filesize = struct.unpack_from("<QQ", data, link + 40)
    code_limit = fileoff + filesize
    if code_limit != len(data):
        # Hash only through the linkedit the loader described.
        if code_limit > len(data):
            data.extend(b"\x00" * (code_limit - len(data)))
    header_end = 32 + sizeofcmds
    if header_end + 16 > PAGE:
        raise SystemExit("no room for LC_CODE_SIGNATURE")

    ent = struct.pack(">II", CSMAGIC_EMBEDDED_ENTITLEMENTS, 8 + len(ENTITLEMENTS_XML)) + ENTITLEMENTS_XML
    req = REQUIREMENTS
    empty = EMPTY_SIG

    ident_b = ident.encode("ascii") + b"\x00"
    n_special = 5
    n_code = (code_limit + PAGE - 1) // PAGE
    # CodeDirectory version 0x20400 fixed header is 88 bytes, then ident.
    cd_prefix_len = 88 + len(ident_b)
    hash_off = cd_prefix_len + n_special * 32
    cd_len = hash_off + n_code * 32
    cd = bytearray(cd_len)
    struct.pack_into(
        ">IIIIIIIII",
        cd,
        0,
        CSMAGIC_CODEDIRECTORY,
        cd_len,
        0x20400,
        0x2,  # CS_ADHOC
        hash_off,
        88,
        n_special,
        n_code,
        code_limit,
    )
    cd[36] = 32  # hashSize
    cd[37] = 2   # sha256
    cd[38] = 0   # platform
    cd[39] = 12  # log2 page size, matching this image's own binaries
    # spare2, scatterOffset, teamOffset and spare3 at offsets 40..55 stay zero.
    struct.pack_into(">Q", cd, 56, 0)          # codeLimit64
    struct.pack_into(">Q", cd, 64, 0)          # execSegBase
    struct.pack_into(">Q", cd, 72, text_size)  # execSegLimit
    struct.pack_into(">Q", cd, 80, 1)          # CS_EXECSEG_MAIN_BINARY
    cd[88:88 + len(ident_b)] = ident_b

    # Superblob index is fixed before hashing because the signature lives past codeLimit.
    # Hashes cover [0:code_limit), which includes the new load command but not the blob.
    super_count = 4
    super_header = 12 + super_count * 8
    # Offsets filled after we know blob sizes. CD is first.
    cd_off = super_header
    req_off = cd_off + len(cd)
    ent_off = req_off + len(req)
    sig_off = ent_off + len(ent)
    super_len = sig_off + len(empty)
    datasize = (super_len + 15) & ~15

    dataoff = code_limit
    struct.pack_into("<IIII", data, header_end, LC_CODE_SIGNATURE, 16, dataoff, datasize)
    magic, cputype, cpusub, filetype, ncmds, old_sizeof, flags, reserved = struct.unpack_from("<IIIIIIII", data, 0)
    struct.pack_into(
        "<IIIIIIII",
        data,
        0,
        magic,
        cputype,
        cpusub,
        filetype,
        ncmds + 1,
        old_sizeof + 16,
        flags,
        reserved,
    )
    new_filesize = filesize + datasize
    struct.pack_into("<Q", data, link + 48, new_filesize)
    vmsize = (new_filesize + 0x3FFF) & ~0x3FFF
    struct.pack_into("<Q", data, link + 32, vmsize)

    code = page_hashes(data, code_limit)
    if len(code) != n_code:
        raise SystemExit("internal slot count")
    specials = [b"\x00" * 32 for _ in range(n_special)]
    specials[0] = hashlib.sha256(ent).digest()  # slot -5
    specials[3] = hashlib.sha256(req).digest()  # slot -2
    all_hashes = b"".join(specials + code)
    cd[cd_prefix_len:cd_prefix_len + len(all_hashes)] = all_hashes

    superblob = bytearray(datasize)
    struct.pack_into(">III", superblob, 0, CSMAGIC_EMBEDDED_SIGNATURE, super_len, super_count)
    index = [
        (CSSLOT_CODEDIRECTORY, cd_off),
        (CSSLOT_REQUIREMENTS, req_off),
        (CSSLOT_ENTITLEMENTS, ent_off),
        (CSSLOT_SIGNATURESLOT, sig_off),
    ]
    for i, (typ, off) in enumerate(index):
        struct.pack_into(">II", superblob, 12 + i * 8, typ, off)
    superblob[cd_off:cd_off + len(cd)] = cd
    superblob[req_off:req_off + len(req)] = req
    superblob[ent_off:ent_off + len(ent)] = ent
    superblob[sig_off:sig_off + len(empty)] = empty
    data.extend(superblob)
    return bytes(data)


def self_check(data):
    cmds, _ = walk_cmds(data)
    sig = None
    for cmd, cmdsize, off in cmds:
        if cmd == LC_CODE_SIGNATURE:
            sig = struct.unpack_from("<II", data, off + 8)
        if cmd == LC_DYLD_CHAINED_FIXUPS:
            raise SystemExit("chained fixups survived signing")
    if not any(cmd == LC_DYLD_INFO_ONLY for cmd, _, _ in cmds) or not sig:
        raise SystemExit("signed binary is missing dyld info or a signature")
    blob = data[sig[0]:sig[0] + sig[1]]
    count = u32be(blob, 8)
    slots = {}
    for i in range(count):
        typ, off = struct.unpack_from(">II", blob, 12 + i * 8)
        slots[typ] = off
    cd = parse_cd_hashes(blob, slots[CSSLOT_CODEDIRECTORY])
    code = page_hashes(data, cd["code_limit"])
    for i, digest in enumerate(code):
        if digest != cd["hashes"][cd["n_special"] + i]:
            raise SystemExit("self-check failed on code slot %d" % i)
    ent = struct.pack(">II", CSMAGIC_EMBEDDED_ENTITLEMENTS, 8 + len(ENTITLEMENTS_XML)) + ENTITLEMENTS_XML
    if cd["hashes"][0] != hashlib.sha256(ent).digest():
        raise SystemExit("self-check failed on entitlement hash")
    if cd["hashes"][3] != hashlib.sha256(REQUIREMENTS).digest():
        raise SystemExit("self-check failed on requirements hash")
    print("self-check ok slots", cd["n_code"], "codeLimit", cd["code_limit"])


def main():
    verify_stock_ps()
    common = ["clang", "-target", "arm64-apple-ios14.0", "-fuse-ld=lld"]
    run(common + [
        "-nostdlib", "-fno-stack-protector", "-fno-builtin", "-Os",
        "-o", OUT + ".unsigned", SRC, TBD,
        "-Wl,-e,_main",
        "-Wl,-undefined,dynamic_lookup",
        "-Wl,-platform_version,ios,14.0,14.0",
        "-Wl,-no_fixup_chains",
    ])
    unsigned = open(OUT + ".unsigned", "rb").read()
    signed = sign(unsigned, "com.aqemu.register")
    self_check(signed)
    open(OUT, "wb").write(signed)
    os.remove(OUT + ".unsigned")
    print("wrote", OUT, "bytes", len(signed))


if __name__ == "__main__":
    main()
