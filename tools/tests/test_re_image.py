"""Tests for tools/re/image.py. Run: python -m unittest discover -s tools/tests -v"""
import pathlib
import struct
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "re"))

import image  # noqa: E402

EXECUTE = 0x20000000


def make_pe(sections):
    """A PE header with the given (name, virtual address, virtual size, characteristics)."""
    data = bytearray(0x1000)
    data[0:2] = b"MZ"
    pe = 0x80
    struct.pack_into("<I", data, 0x3C, pe)
    data[pe:pe + 4] = b"PE\0\0"
    optional_size = 0xE0
    struct.pack_into("<HHIIIHH", data, pe + 4, 0x1F2, len(sections), 0, 0, 0, optional_size, 0)
    table = pe + 24 + optional_size
    for i, (name, va, size, flags) in enumerate(sections):
        entry = table + i * 40
        data[entry:entry + 8] = name.ljust(8, b"\0")
        struct.pack_into("<II", data, entry + 8, size, va)
        struct.pack_into("<I", data, entry + 36, flags)
    return bytes(data)


class CodeRangeTest(unittest.TestCase):
    def test_v7_text_section(self):
        pe = make_pe([(b".rdata", 0x1000, 0x10F000, 0x40000040),
                      (b".text", 0x110000, 0xA25A64, EXECUTE | 0x20)])
        self.assertEqual(image.code_range(pe), (0x82110000, 0x82B35A64))

    def test_spans_all_executable_sections(self):
        pe = make_pe([(b".text", 0x110000, 0x1000, EXECUTE),
                      (b".data", 0x200000, 0x1000, 0xC0000040),
                      (b".text2", 0x300000, 0x2000, EXECUTE)])
        self.assertEqual(image.code_range(pe), (0x82110000, 0x82302000))

    def test_rejects_non_pe(self):
        with self.assertRaises(ValueError):
            image.code_range(b"XEX2" + bytes(0x100))

    def test_rejects_no_code(self):
        with self.assertRaises(ValueError):
            image.code_range(make_pe([(b".data", 0x1000, 0x1000, 0xC0000040)]))


if __name__ == "__main__":
    unittest.main()
