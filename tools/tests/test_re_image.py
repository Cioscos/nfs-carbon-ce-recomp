"""Tests for tools/re/image.py. Run: python -m unittest discover -s tools/tests -v"""
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "re"))

import image  # noqa: E402

PCH_V7 = """\
#pragma once
#define REX_IMAGE_BASE 0x82000000ull
#define REX_IMAGE_SIZE 0x10E0000ull
#define REX_CODE_BASE 0x82110000ull
#define REX_CODE_SIZE 0xA25A64ull
"""


class CodeRangeTest(unittest.TestCase):
    def write_pch(self, tmp, text, name="nfsc_pch.h"):
        (pathlib.Path(tmp) / name).write_text(text)
        (pathlib.Path(tmp) / "nfsc_init.cpp").write_text("// init\n")
        return pathlib.Path(tmp)

    def test_v7_code_range(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = self.write_pch(tmp, PCH_V7)
            self.assertEqual(image.code_range(gen), (0x82110000, 0x82B35A64))

    def test_accepts_init_cpp_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = self.write_pch(tmp, PCH_V7)
            self.assertEqual(image.code_range(gen / "nfsc_init.cpp"), (0x82110000, 0x82B35A64))

    def test_missing_pch(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError):
                image.code_range(pathlib.Path(tmp))

    def test_missing_define(self):
        with tempfile.TemporaryDirectory() as tmp:
            gen = self.write_pch(tmp, "#define REX_CODE_BASE 0x82110000ull\n")
            with self.assertRaises(ValueError):
                image.code_range(gen)


if __name__ == "__main__":
    unittest.main()
