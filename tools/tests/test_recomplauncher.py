"""Tests for recomplauncher.json and the release names of tools/package-release.py.
Run: python -m unittest discover -s tools/tests -v"""
import importlib.util
import json
import pathlib
import re
import tempfile
import unittest
from unittest import mock

TOOLS = pathlib.Path(__file__).resolve().parents[1]
ROOT = TOOLS.parent

spec = importlib.util.spec_from_file_location("package_release", TOOLS / "package-release.py")
package_release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package_release)


class ManifestTest(unittest.TestCase):
    def setUp(self):
        self.manifest = json.loads((ROOT / "recomplauncher.json").read_text(encoding="utf-8"))

    def test_checks_the_title_id_of_the_editions(self):
        editions = (ROOT / "nfsc" / "cmake" / "editions.cmake").read_text(encoding="utf-8")
        title_id = re.search(r"set\(NFSC_TITLE_ID (\w+)\)", editions).group(1)
        (game,) = [i for i in self.manifest["inputs"] if i["id"] == "game"]
        self.assertEqual(game["check"]["ids"], [title_id])

    def test_starts_nfsc_with_the_copy_and_the_data_folder(self):
        start = self.manifest["start"]
        self.assertEqual(start["program"], ["nfsc.exe"])
        # Names main.cpp maps to title.game_root and app.user_root.
        main = (ROOT / "nfsc" / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('{"game_data_root", "title.game_root"}', main)
        self.assertIn('{"user_data_root", "app.user_root"}', main)
        self.assertEqual(start["args"],
                         ["--game_data_root={input:game}", "--user_data_root={data}"])

    def test_saves_and_cache_are_in_the_data_folder(self):
        # With app.user_root set, game_path.cpp puts the cache in <user_root>/cache, which the
        # launcher leaves out of backups.
        self.assertEqual(self.manifest["saves"], ["{data}"])
        clear = [a["clear"] for a in self.manifest["actions"] if "clear" in a]
        self.assertEqual(clear, ["saves/cache"])

    def test_names_this_repository(self):
        self.assertEqual(self.manifest["repo"], "Cioscos/nfs-carbon-ce-recomp")


class RegionTest(unittest.TestCase):
    def region_for(self, define):
        with tempfile.TemporaryDirectory() as tmp:
            tree = pathlib.Path(tmp)
            (tree / "nfsc_generated").mkdir()
            (tree / "nfsc_generated" / "nfsc_edition.h").write_text(
                f"#pragma once\n\n#define {define} 1\n", encoding="utf-8")
            with mock.patch.object(package_release, "BUILD_TREE", tree):
                return package_release.built_region()

    def test_regions(self):
        self.assertEqual(self.region_for("NFSC_GAME_V7"), "USA")
        self.assertEqual(self.region_for("NFSC_GAME_V11"), "Europe")

    def test_unknown_edition_stops(self):
        with self.assertRaises(SystemExit):
            self.region_for("NFSC_GAME_V99")

    def test_every_edition_has_a_region(self):
        editions = (ROOT / "nfsc" / "cmake" / "editions.cmake").read_text(encoding="utf-8")
        names = re.search(r"set\(NFSC_EDITIONS ([\w ]+)\)", editions).group(1).split()
        self.assertEqual(sorted(n.upper() for n in names),
                         sorted(package_release.EDITION_REGIONS))

    def test_manifest_region_is_known(self):
        self.assertIn(package_release.LAUNCHER_MANIFEST_REGION,
                      package_release.EDITION_REGIONS.values())


if __name__ == "__main__":
    unittest.main()
