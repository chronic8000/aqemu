#!/usr/bin/env python3
"""Write a 4096-byte-sector GPT + APFS-type partition on Inferno guest `root`.

Ramrod with CreateFilesystemPartitions=false rejects zeros (invalid GPT header)
and empty GPT (Storage with GPT format). An APFS partition without a Data volume
fails with status 75. AQEMU Power On runs format-seed-apfs.sh (mkapfs) so the
seed partition has a Data-role volume.
"""
from __future__ import annotations

import argparse
import os
import struct
import subprocess
import sys
import zlib

LBA = 4096
N_ENTRIES = 128
ENTRY_SIZE = 128
SEED_NAME = "AQEMU_SEED"
ROOT_BYTES = 8 * 1024 * 1024 * 1024
START_LBA = 256

APFS_TYPE = bytes.fromhex("EF57347C0000AA11AA1100306543ECAC")
DISK_GUID = bytes.fromhex("0100E0A05047415481004151454D5501")
PART_GUID = bytes.fromhex("0200E0A05047415481004151454D5502")


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def make_header(
    my_lba: int,
    alt_lba: int,
    first_usable: int,
    last_usable: int,
    part_lba: int,
    part_crc: int,
) -> bytes:
    h = bytearray(92)
    h[0:8] = b"EFI PART"
    struct.pack_into("<I", h, 8, 0x00010000)
    struct.pack_into("<I", h, 12, 92)
    struct.pack_into("<Q", h, 24, my_lba)
    struct.pack_into("<Q", h, 32, alt_lba)
    struct.pack_into("<Q", h, 40, first_usable)
    struct.pack_into("<Q", h, 48, last_usable)
    h[56:72] = DISK_GUID
    struct.pack_into("<Q", h, 72, part_lba)
    struct.pack_into("<I", h, 80, N_ENTRIES)
    struct.pack_into("<I", h, 84, ENTRY_SIZE)
    struct.pack_into("<I", h, 88, part_crc)
    struct.pack_into("<I", h, 16, crc32(h))
    return bytes(h)


def first_part_name(path: str) -> str:
    with open(path, "rb") as f:
        sz = os.path.getsize(path)
        for header_off, lba in ((LBA, LBA), (512, 512)):
            if header_off + 8 > sz:
                continue
            f.seek(header_off)
            if f.read(8) != b"EFI PART":
                continue
            f.seek(header_off + 72)
            entry_lba = struct.unpack("<Q", f.read(8))[0]
            f.seek(entry_lba * lba + 56)
            raw = f.read(72)
            return raw.decode("utf-16le").split("\x00")[0]
    return ""


def has_efi_part(path: str) -> bool:
    with open(path, "rb") as f:
        sz = os.path.getsize(path)
        for off in (512, LBA):
            if off + 8 <= sz:
                f.seek(off)
                if f.read(8) == b"EFI PART":
                    return True
    return False


def needs_seed(path: str) -> bool:
    if not os.path.exists(path) or not has_efi_part(path):
        return True
    return first_part_name(path) == SEED_NAME


def ensure_sparse_size(path: str, size: int) -> None:
    parent = os.path.dirname(path)
    if parent:
        os.makedirs(parent, exist_ok=True)
    if not os.path.exists(path):
        open(path, "wb").close()
        if os.name == "nt":
            subprocess.run(
                ["fsutil", "sparse", "setflag", path],
                check=False,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
    cur = os.path.getsize(path)
    if cur < size:
        with open(path, "r+b") as f:
            f.truncate(size)


def seed(path: str) -> None:
    ensure_sparse_size(path, ROOT_BYTES)
    if not needs_seed(path):
        print(f"Already has a GPT (not AQEMU_SEED), leaving as-is: {path}")
        return
    sz = os.path.getsize(path)
    if sz % LBA:
        raise SystemExit(f"Size {sz} is not a multiple of {LBA}: {path}")
    n_lba = sz // LBA
    array_bytes = N_ENTRIES * ENTRY_SIZE
    array_lbas = (array_bytes + LBA - 1) // LBA
    first_usable = 2 + array_lbas
    last_usable = n_lba - 1 - array_lbas - 1
    if last_usable < START_LBA:
        raise SystemExit(f"Image too small for GPT: {path}")

    entries = bytearray(array_bytes)
    entries[0:16] = APFS_TYPE
    entries[16:32] = PART_GUID
    struct.pack_into("<Q", entries, 32, START_LBA)
    struct.pack_into("<Q", entries, 40, last_usable)
    name = SEED_NAME.encode("utf-16le")
    entries[56 : 56 + len(name)] = name
    part_crc = crc32(entries)
    backup_hdr = n_lba - 1
    backup_arr = n_lba - 1 - array_lbas
    primary = make_header(1, backup_hdr, first_usable, last_usable, 2, part_crc)
    backup = make_header(
        backup_hdr, 1, first_usable, last_usable, backup_arr, part_crc
    )

    mbr = bytearray(LBA)
    mbr[448] = 0x02
    mbr[450] = 0xEE
    mbr[451:454] = b"\xff\xff\xff"
    struct.pack_into("<I", mbr, 454, 1)
    struct.pack_into("<I", mbr, 458, min(0xFFFFFFFF, n_lba - 1))
    mbr[510:512] = b"\x55\xaa"

    def write_lba(f, lba: int, data: bytes) -> None:
        f.seek(lba * LBA)
        if len(data) < LBA:
            f.write(data + bytes(LBA - len(data)))
        else:
            f.write(data)

    with open(path, "r+b") as f:
        write_lba(f, 0, mbr)
        write_lba(f, 1, primary)
        f.seek(2 * LBA)
        f.write(entries)
        f.seek(backup_arr * LBA)
        f.write(entries)
        write_lba(f, backup_hdr, backup)

    print(f"Wrote APFS-type GPT ({SEED_NAME}) on {path}")
    print("Power On AQEMU to run extras/Inferno/format-seed-apfs.sh (mkapfs Data volume).")


def root_from_vm_xml(vm_xml: str) -> str:
    xml = os.path.abspath(vm_xml)
    base = os.path.splitext(os.path.basename(xml))[0]
    return os.path.join(os.path.dirname(xml), f"{base}_inferno", "root")


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("root", nargs="?", help="Path to *_inferno/root")
    p.add_argument("--vm-xml", help="VM .aqemu path (uses <name>_inferno/root)")
    args = p.parse_args()
    if args.vm_xml:
        path = root_from_vm_xml(args.vm_xml)
    elif args.root:
        path = args.root
    else:
        p.print_help()
        return 2
    seed(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
