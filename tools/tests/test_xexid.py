"""Tests for tools/xexid.py. Run: python -m unittest discover -s tools/tests -v"""
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest

TOOLS = pathlib.Path(__file__).resolve().parents[1]
ROOT = TOOLS.parent
sys.path.insert(0, str(TOOLS))

import xexid  # noqa: E402

EXEC_INFO = 0x200
SECURITY = 0x400


def make_header(title, version, digest, pages=1):
    """A minimal XEX2 header with an execution info and a security info."""
    data = bytearray(0x1000)
    data[0:4] = b"XEX2"
    struct.pack_into(">I", data, 0x10, SECURITY)
    struct.pack_into(">I", data, 0x14, 1)
    struct.pack_into(">II", data, 0x18, 0x00040006, EXEC_INFO)
    struct.pack_into(">I", data, EXEC_INFO + 4, version)
    struct.pack_into(">I", data, EXEC_INFO + 12, title)
    struct.pack_into(">I", data, SECURITY + 0x180, pages)
    data[SECURITY + 0x188:SECURITY + 0x188 + 20] = digest
    return bytes(data)


def run_cli(path):
    return subprocess.run([sys.executable, "-I", str(TOOLS / "xexid.py"), str(path)],
                          capture_output=True, text=True)


class XexIdTest(unittest.TestCase):
    def test_reads_identity(self):
        header = make_header(0x454107EC, 0xB, bytes(range(20)))
        self.assertEqual(xexid.read_identity(header), (0x454107EC, 0xB, bytes(range(20))))

    def test_rejects_non_xex(self):
        self.assertIsNone(xexid.read_identity(b"MZ" + bytes(100)))

    def test_rejects_zero_pages(self):
        self.assertIsNone(xexid.read_identity(make_header(0x454107EC, 0xB, bytes(20), pages=0)))

    def test_rejects_truncated_header(self):
        header = make_header(0x454107EC, 0xB, bytes(20))
        for size in (0x18, 0x1C, 0x20, EXEC_INFO + 8, SECURITY + 0x184, SECURITY + 0x190):
            self.assertIsNone(xexid.read_identity(header[:size]), size)

    def test_cli_output(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "default.xex"
            path.write_bytes(make_header(0x454107EC, 0xB, bytes(range(20))))
            result = run_cli(path)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout,
                         "title=454107EC version=0000000B digest=" + bytes(range(20)).hex() + "\n")

    def test_cli_fails_on_non_xex(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "default.xex"
            path.write_bytes(b"MZ" + bytes(100))
            result = run_cli(path)
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertNotEqual(result.stderr, "")

    def test_cli_fails_on_missing_file(self):
        result = run_cli(ROOT / "does-not-exist.xex")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")

    @unittest.skipUnless((ROOT / "game" / "default.xex").is_file(), "no game/default.xex")
    def test_real_v11(self):
        result = run_cli(ROOT / "game" / "default.xex")
        if "version=0000000B" not in result.stdout:
            self.skipTest("game/default.xex is not v0.0.0.11")
        self.assertEqual(result.stdout.strip(), "title=454107EC version=0000000B "
                         "digest=8bcc2f4cdf7888f85ee82c012ed14d570f6e0231")


if __name__ == "__main__":
    unittest.main()
