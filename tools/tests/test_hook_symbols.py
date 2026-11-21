"""The hooks expand to the same guest symbols for v0.0.0.7 as before the addresses moved into
nfsc/src/addresses_v7.h (commit 788e127), and to none of the v7 symbols for other editions.

Only the hook macros are expanded: each hook file is preprocessed with clang++ -E after its
#include lines are removed, with REX_EXTERN / REX_HOOK_RAW replaced by markers and a stand-in
nfsc_edition.h, so neither the SDK nor a build directory is needed.

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
BASELINE = "788e127"
HOOK_FILES = ["framerate.cpp", "main_menu.cpp", "pad.cpp", "widescreen.cpp"]
SYMBOL = re.compile(r"\b(?:__imp__)?sub_[0-9A-F]{8}\b")

PRELUDE = """\
#define REX_EXTERN(x) NFSC_TEST_EXTERN x;
#define REX_HOOK_RAW(x) void x(PPCContext& ctx, uint8_t* base)
"""


def edition_header(edition):
    return f"#pragma once\n#define NFSC_GAME_{edition.upper()} 1\n"


def expand(text, edition, include_addresses):
    """The guest symbols in a hook file's text after macro expansion."""
    body = "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("#include"))
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        (tmp / "nfsc_edition.h").write_text(edition_header(edition))
        tu = PRELUDE + ('#include "addresses.h"\n' if include_addresses else "") + body
        (tmp / "tu.cpp").write_text(tu)
        result = subprocess.run(
            ["clang++", "-std=c++23", "-E", "-P", f"-I{tmp}", f"-I{SRC}", str(tmp / "tu.cpp")],
            capture_output=True, text=True)
    if result.returncode != 0:
        raise AssertionError(f"clang++ -E failed:\n{result.stderr}")
    return set(SYMBOL.findall(result.stdout))


def baseline_text(name):
    return subprocess.run(["git", "-C", str(ROOT), "show", f"{BASELINE}:nfsc/src/{name}"],
                          capture_output=True, text=True, check=True).stdout


@unittest.skipUnless(shutil.which("clang++"), "clang++ not in PATH")
class HookSymbolsTest(unittest.TestCase):
    def test_v7_symbols_unchanged(self):
        for name in HOOK_FILES:
            with self.subTest(name):
                before = expand(baseline_text(name), "v7", include_addresses=False)
                self.assertTrue(before, "baseline has no symbols")
                now = expand((SRC / name).read_text(), "v7", include_addresses=True)
                self.assertEqual(now, before)

    def test_v11_has_no_v7_symbols(self):
        v7 = set()
        for name in HOOK_FILES:
            v7 |= expand(baseline_text(name), "v7", include_addresses=False)
        for name in HOOK_FILES:
            with self.subTest(name):
                now = expand((SRC / name).read_text(), "v11", include_addresses=True)
                self.assertFalse(now & v7, "v7 addresses in the v11 build")


if __name__ == "__main__":
    unittest.main()
