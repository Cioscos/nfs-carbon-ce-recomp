"""Tests for the edition selection in CMake: nfsc/cmake/edition_select.cmake (shared functions)
and nfsc/cmake/select_functions.cmake (the build step that copies the edition's
functions.toml before codegen reads it). They run `cmake -P`, so no build directory is needed.

Run: python -m unittest discover -s tools/tests -v
"""
import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

TESTS = pathlib.Path(__file__).resolve().parent
ROOT = TESTS.parents[1]
CMAKE_DIR = ROOT / "nfsc" / "cmake"
sys.path.insert(0, str(TESTS))

from test_xexid import make_header  # noqa: E402

V11_DIGEST = bytes.fromhex("8bcc2f4cdf7888f85ee82c012ed14d570f6e0231")


def cmake_path(path):
    return pathlib.Path(path).as_posix()


def run_cmake(args, cwd):
    return subprocess.run(["cmake", *args], capture_output=True, text=True, cwd=cwd)


def run_driver(body, tmp):
    """Runs a cmake -P script that has editions.cmake and edition_select.cmake included."""
    driver = pathlib.Path(tmp) / "driver.cmake"
    driver.write_text(body.format(cmake_dir=cmake_path(CMAKE_DIR)))
    return run_cmake(["-P", str(driver)], tmp)


@unittest.skipUnless(shutil.which("cmake"), "cmake not in PATH")
class CheckEditionsTest(unittest.TestCase):
    def check(self, overrides):
        with tempfile.TemporaryDirectory() as tmp:
            return run_driver(
                'include("{cmake_dir}/editions.cmake")\n' + overrides +
                'include("{cmake_dir}/edition_select.cmake")\nnfsc_check_editions()\n', tmp)

    def test_real_table_is_valid(self):
        result = self.check("")
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_uppercase_digest_is_rejected(self):
        result = self.check("set(NFSC_EDITION_v11_DIGEST 8BCC2F4CDF7888F85EE82C012ED14D570F6E0231)\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("NFSC_EDITION_v11_DIGEST", result.stderr)

    def test_short_version_is_rejected(self):
        result = self.check("set(NFSC_EDITION_v7_VERSION 7)\n")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("NFSC_EDITION_v7_VERSION", result.stderr)


@unittest.skipUnless(shutil.which("cmake"), "cmake not in PATH")
class ChooseEditionTest(unittest.TestCase):
    def test_missing_xex_message(self):
        with tempfile.TemporaryDirectory() as tmp:
            result = run_driver(
                'include("{cmake_dir}/editions.cmake")\n'
                'include("{cmake_dir}/edition_select.cmake")\n'
                'nfsc_choose_edition(auto "" "/repo/game/default.xex" edition)\n', tmp)
        self.assertNotEqual(result.returncode, 0)
        message = " ".join(result.stderr.split())
        self.assertIn("From the repository root", message)
        self.assertIn('python tools/xdvdfs.py extract "path/to/your/disc.iso" game default.xex',
                      message)
        self.assertIn("to configure without it", message)
        self.assertNotIn("check only the host code", message)


@unittest.skipUnless(shutil.which("cmake"), "cmake not in PATH")
class SelectFunctionsTest(unittest.TestCase):
    """select_functions.cmake on a small tree: nfsc/config/<edition>/ and game/default.xex."""

    def make_tree(self, tmp, xex=True):
        tmp = pathlib.Path(tmp)
        for edition in ("v7", "v11", "selected"):
            (tmp / "nfsc" / "config" / edition).mkdir(parents=True)
        (tmp / "nfsc/config/v7/functions.toml").write_text("# v7\n")
        (tmp / "nfsc/config/v11/functions.toml").write_text("# v11\n")
        # What the previous edition's configure left there.
        (tmp / "nfsc/config/selected/functions.toml").write_text("# v7\n")
        if xex:
            (tmp / "game").mkdir()
            (tmp / "game/default.xex").write_bytes(make_header(0x454107EC, 0xB, V11_DIGEST))
        return tmp

    def select(self, tree, version="auto"):
        return run_cmake([
            f"-DNFSC_SOURCE_DIR={cmake_path(tree / 'nfsc')}",
            f"-DNFSC_XEX={cmake_path(tree / 'game/default.xex')}",
            f"-DNFSC_XEXID={cmake_path(ROOT / 'tools/xexid.py')}",
            f"-DPython3_EXECUTABLE={cmake_path(sys.executable)}",
            f"-DNFSC_GAME_VERSION={version}",
            f"-DNFSC_DEPFILE={cmake_path(tree / 'select.d')}",
            "-P", str(CMAKE_DIR / "select_functions.cmake")], tree)

    def test_copies_the_toml_of_the_xex_edition(self):
        with tempfile.TemporaryDirectory() as tmp:
            tree = self.make_tree(tmp)
            result = self.select(tree)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual((tree / "nfsc/config/selected/functions.toml").read_text(), "# v11\n")
            depfile = (tree / "select.d").read_text()
            self.assertIn(cmake_path(tree / "nfsc/config/selected/functions.toml") + ":", depfile)
            self.assertIn(cmake_path(tree / "game/default.xex"), depfile)

    def test_leaves_an_up_to_date_copy_alone(self):
        with tempfile.TemporaryDirectory() as tmp:
            tree = self.make_tree(tmp)
            selected = tree / "nfsc/config/selected/functions.toml"
            self.assertEqual(self.select(tree).returncode, 0)
            before = selected.stat().st_mtime_ns
            self.assertEqual(self.select(tree).returncode, 0)
            self.assertEqual(selected.stat().st_mtime_ns, before)

    def test_forced_version_must_match_the_xex(self):
        with tempfile.TemporaryDirectory() as tmp:
            tree = self.make_tree(tmp)
            result = self.select(tree, version="v7")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("NFSC_GAME_VERSION", result.stderr)

    def test_missing_xex_stops(self):
        with tempfile.TemporaryDirectory() as tmp:
            tree = self.make_tree(tmp, xex=False)
            result = self.select(tree, version="v11")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("xdvdfs.py extract", result.stderr)


if __name__ == "__main__":
    unittest.main()
