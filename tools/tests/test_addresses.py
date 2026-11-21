"""nfsc/src/addresses.h refuses feature flags whose hooks depend on a feature that is off:
WIDESCREEN needs PRESENT (the present hook updates the 16:9 pillarbox) and PAD needs MAIN_MENU
(the main menu hook ends the intro skip). The real edition headers pass, and each defines
every feature flag: the language select screen is on for v0.0.0.11 only.

addresses.h is preprocessed with clang++ -E next to a stand-in addresses_v11.h.

Run: python -m unittest discover -s tools/tests -v
"""
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "nfsc" / "src"
FEATURES = ("PRESENT", "PAD", "WIDESCREEN", "MAIN_MENU", "LANGUAGE_SELECT")
# The flags of the real headers.
EDITION_FEATURES = {
    "v7": {"PRESENT": "1", "PAD": "1", "WIDESCREEN": "1", "MAIN_MENU": "1",
           "LANGUAGE_SELECT": "0"},
    "v11": {"PRESENT": "1", "PAD": "1", "WIDESCREEN": "1", "MAIN_MENU": "1",
            "LANGUAGE_SELECT": "1"},
}


def preprocess(edition, addresses_v11=None, extra=""):
    """clang++ -E of addresses.h for edition, then of extra; with addresses_v11, that text
    stands in for the real header."""
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        (tmp / "nfsc_edition.h").write_text(f"#pragma once\n#define NFSC_GAME_{edition.upper()} 1\n")
        if addresses_v11 is None:
            header = SRC / "addresses.h"
        else:
            header = tmp / "addresses.h"
            shutil.copy(SRC / "addresses.h", header)
            (tmp / "addresses_v11.h").write_text(addresses_v11)
        (tmp / "tu.cpp").write_text(f'#include "{header.as_posix()}"\n{extra}')
        return subprocess.run(
            ["clang++", "-std=c++23", "-E", "-P", f"-I{tmp}", f"-I{SRC}", str(tmp / "tu.cpp")],
            capture_output=True, text=True)


def flags(**on):
    return "#pragma once\n" + "".join(
        f"#define NFSC_FEATURE_{name} {1 if on.get(name.lower()) else 0}\n" for name in FEATURES)


@unittest.skipUnless(shutil.which("clang++"), "clang++ not in PATH")
class FeatureFlagsTest(unittest.TestCase):
    def test_real_headers(self):
        for edition in ("v7", "v11"):
            with self.subTest(edition):
                result = preprocess(edition)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_real_headers_define_every_flag(self):
        extra = "".join(f"NFSC_TEST {name} NFSC_FEATURE_{name}\n" for name in FEATURES)
        for edition, expected in EDITION_FEATURES.items():
            with self.subTest(edition):
                result = preprocess(edition, extra=extra)
                self.assertEqual(result.returncode, 0, result.stderr)
                found = dict(re.findall(r"^NFSC_TEST (\w+) (\w+)$", result.stdout, re.MULTILINE))
                self.assertEqual(found, expected)

    def test_all_off_and_all_on(self):
        for header in (flags(), flags(present=1, pad=1, widescreen=1, main_menu=1,
                                      language_select=1)):
            result = preprocess("v11", header)
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_widescreen_needs_present(self):
        result = preprocess("v11", flags(widescreen=1))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("NFSC_FEATURE_WIDESCREEN needs NFSC_FEATURE_PRESENT", result.stderr)

    def test_pad_needs_main_menu(self):
        result = preprocess("v11", flags(pad=1))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("NFSC_FEATURE_PAD needs NFSC_FEATURE_MAIN_MENU", result.stderr)


if __name__ == "__main__":
    unittest.main()
