"""The hooks expand to the same guest symbols, in the same order, for v0.0.0.7 as before the
addresses moved into nfsc/src/addresses_v7.h (BASELINE, commit 788e127), and to none of the v7
symbols for other editions. Each NFSC_FN_* of addresses_v7.h is also checked against
V7_FUNCTIONS.

When a v0.0.0.7 hook is added or changed on purpose, update V7_FUNCTIONS and move BASELINE to
the commit that makes the change.

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

# The v0.0.0.7 functions, as found for the hooks before there were other editions.
V7_FUNCTIONS = {
    "PRESENT": "sub_826E3860",
    "PAD_GET_STATE": "sub_82941FE8",
    "RENDER_SIZE": "sub_8250B0D0",
    "SET_RESOLUTION_MODE": "sub_82500168",
    "VIEW_LAYOUT": "sub_824FFAB0",
    "RT_DESCRIPTOR": "sub_82505998",
    "MAIN_MENU_SETUP": "sub_82642DB0",
    "MENU_ADD_OPTION": "sub_82313658",
    "ALLOC": "sub_821A3320",
    "OPTION_CTOR": "sub_8230EC78",
    "OPTION_REACT": "sub_8230EE18",
    "STRING_LOOKUP": "sub_8231F7B8",
}
SYMBOL = re.compile(r"\b(?:__imp__)?sub_[0-9A-F]{8}\b")

PRELUDE = """\
#define REX_EXTERN(x) NFSC_TEST_EXTERN x;
#define REX_HOOK_RAW(x) void x(PPCContext& ctx, uint8_t* base)
"""


def edition_header(edition):
    return f"#pragma once\n#define NFSC_GAME_{edition.upper()} 1\n"


def preprocess(text, edition):
    """clang++ -E of text, with nfsc/src and a stand-in nfsc_edition.h on the include path."""
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        (tmp / "nfsc_edition.h").write_text(edition_header(edition))
        (tmp / "tu.cpp").write_text(text)
        result = subprocess.run(
            ["clang++", "-std=c++23", "-E", "-P", f"-I{tmp}", f"-I{SRC}", str(tmp / "tu.cpp")],
            capture_output=True, text=True)
    if result.returncode != 0:
        raise AssertionError(f"clang++ -E failed:\n{result.stderr}")
    return result.stdout


def expand(text, edition, include_addresses):
    """The guest symbols in a hook file's text after macro expansion, in order."""
    body = "\n".join(line for line in text.splitlines() if not line.lstrip().startswith("#include"))
    tu = PRELUDE + ('#include "addresses.h"\n' if include_addresses else "") + body
    return SYMBOL.findall(preprocess(tu, edition))


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

    def test_v7_functions(self):
        lines = "".join(f"NFSC_TEST {name} NFSC_FN_{name}\n" for name in V7_FUNCTIONS)
        out = preprocess('#include "addresses.h"\n' + lines, "v7")
        found = dict(re.findall(r"^NFSC_TEST (\w+) (\w+)$", out, re.MULTILINE))
        self.assertEqual(found, V7_FUNCTIONS)

    def test_v11_has_no_v7_symbols(self):
        v7 = set()
        for name in HOOK_FILES:
            v7 |= set(expand(baseline_text(name), "v7", include_addresses=False))
        for name in HOOK_FILES:
            with self.subTest(name):
                now = set(expand((SRC / name).read_text(), "v11", include_addresses=True))
                self.assertFalse(now & v7, "v7 addresses in the v11 build")


if __name__ == "__main__":
    unittest.main()
