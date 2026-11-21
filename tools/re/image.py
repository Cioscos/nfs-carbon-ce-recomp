"""Helpers for a dumped guest image (nfsc.dump_image): the PE image as loaded at 0x82000000."""
import struct

BASE = 0x82000000
IMAGE_SCN_MEM_EXECUTE = 0x20000000


def code_range(data, base=BASE):
    """(start, end) guest addresses spanning the executable sections of the PE header."""
    if data[:2] != b"MZ":
        raise ValueError("not a PE image (no MZ header)")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE image (no PE signature)")
    count, optional_size = struct.unpack_from("<H", data, pe + 6)[0], \
        struct.unpack_from("<H", data, pe + 20)[0]
    table = pe + 24 + optional_size
    ranges = []
    for i in range(count):
        entry = table + i * 40
        size, va = struct.unpack_from("<II", data, entry + 8)
        flags = struct.unpack_from("<I", data, entry + 36)[0]
        if flags & IMAGE_SCN_MEM_EXECUTE:
            ranges.append((base + va, base + va + size))
    if not ranges:
        raise ValueError("no executable section")
    return min(r[0] for r in ranges), max(r[1] for r in ranges)

