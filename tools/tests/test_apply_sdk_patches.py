"""Tests for tools/apply-sdk-patches.sh on a small git repository that stands in for the SDK.

Run: python -m unittest discover -s tools/tests -v
"""
import os
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "apply-sdk-patches.sh"


def find_bash():
    """Git's bash on Windows (not the WSL one in System32), otherwise bash from PATH."""
    if os.name == "nt":
        exec_path = subprocess.run(["git", "--exec-path"], capture_output=True, text=True)
        if exec_path.returncode == 0:
            # .../Git/mingw64/libexec/git-core -> .../Git/bin/bash.exe
            bash = pathlib.Path(exec_path.stdout.strip()).parents[2] / "bin" / "bash.exe"
            if bash.is_file():
                return str(bash)
        return None
    return shutil.which("bash")


BASH = find_bash()

LINES = [f"line {i}\n" for i in range(1, 21)]


def git(repo, *args):
    result = subprocess.run(["git", "-C", str(repo), *args], capture_output=True, text=True)
    if result.returncode != 0:
        raise AssertionError(f"git {' '.join(args)} failed: {result.stderr}")
    return result.stdout


@unittest.skipUnless(BASH, "bash not found")
class ApplySdkPatchesTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        tmp = pathlib.Path(self._tmp.name)
        self.sdk = tmp / "sdk"
        self.patches = tmp / "patches"
        self.patches.mkdir()
        self.sdk.mkdir()
        git(self.sdk, "init", "-q")
        git(self.sdk, "config", "user.email", "test@example.invalid")
        git(self.sdk, "config", "user.name", "test")
        git(self.sdk, "config", "core.autocrlf", "false")
        for name in ("a.txt", "b.txt", "c.txt"):
            (self.sdk / name).write_text("".join(LINES), newline="\n")
        self.commit("sdk")
        # 0001 changes line 5 of a.txt; 0002 is made on top of it and changes line 7 of a.txt
        # (inside 0001's context) and b.txt, like our patches that touch the same file.
        self.write_patch("0001-first.patch", {"a.txt": {5: "first\n"}})
        self.edit("a.txt", {5: "first\n"})
        git(self.sdk, "add", "a.txt")  # so that the diff for 0002 holds only its own changes
        self.write_patch("0002-second.patch", {"a.txt": {7: "second\n"}, "b.txt": {3: "second\n"}})
        git(self.sdk, "reset", "-q", "--hard")

    def tearDown(self):
        self._tmp.cleanup()

    def commit(self, message):
        git(self.sdk, "add", "-A")
        git(self.sdk, "commit", "-q", "-m", message)

    def edit(self, name, changes):
        lines = (self.sdk / name).read_text().splitlines(keepends=True)
        for number, text in changes.items():
            lines[number - 1] = text
        (self.sdk / name).write_text("".join(lines), newline="\n")

    def write_patch(self, patch_name, files):
        """Writes a patch of `files` ({name: {line: text}}) against the index."""
        for name, changes in files.items():
            self.edit(name, changes)
        diff = git(self.sdk, "diff")
        (self.patches / patch_name).write_text(diff, newline="\n")
        for name in files:
            git(self.sdk, "checkout", "-q", "--", name)

    def run_script(self):
        bash = BASH
        assert bash is not None
        return subprocess.run([bash, str(SCRIPT), str(self.sdk), str(self.patches)],
                              capture_output=True, text=True)

    def line(self, name, number):
        return (self.sdk / name).read_text().splitlines()[number - 1]

    def test_a_clean_sdk_gets_every_patch(self):
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("applied 0001-first.patch", result.stdout)
        self.assertIn("applied 0002-second.patch", result.stdout)
        self.assertEqual(self.line("a.txt", 5), "first")
        self.assertEqual(self.line("a.txt", 7), "second")
        self.assertEqual(self.line("b.txt", 3), "second")

    def test_a_second_run_changes_nothing(self):
        self.assertEqual(self.run_script().returncode, 0)
        before = git(self.sdk, "diff")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("patches already applied (2)", result.stdout)
        self.assertEqual(git(self.sdk, "diff"), before)

    def test_a_patch_the_sdk_already_has_is_left_out(self):
        self.edit("a.txt", {5: "first\n"})
        self.commit("upstream takes 0001")
        result = self.run_script()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("already in the SDK: 0001-first.patch", result.stdout)
        self.assertIn("applied 0002-second.patch", result.stdout)
        self.assertEqual(self.line("a.txt", 7), "second")
        self.assertEqual(self.line("b.txt", 3), "second")

    def test_a_patch_that_does_not_apply_stops_before_any_change(self):
        self.edit("b.txt", {3: "upstream changed this line its own way\n"})
        self.commit("upstream changes b.txt")
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("0002-second.patch does not apply", result.stderr)
        self.assertEqual(git(self.sdk, "status", "--porcelain"), "")

    def test_a_local_change_stops_before_any_change(self):
        self.edit("c.txt", {1: "my own edit\n"})
        result = self.run_script()
        self.assertEqual(result.returncode, 1)
        self.assertIn("local changes", result.stderr)
        self.assertEqual(self.line("c.txt", 1), "my own edit")
        self.assertEqual(self.line("a.txt", 5), "line 5")


if __name__ == "__main__":
    unittest.main()
