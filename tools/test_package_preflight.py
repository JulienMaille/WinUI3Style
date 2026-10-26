"""Exercise the package source guard against real temporary Git repositories."""

import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


POWERSHELL = shutil.which("pwsh") or shutil.which("powershell")
PACKAGE_SCRIPT = Path(__file__).with_name("package_poc.ps1")
CTEST_POLICY = Path(__file__).with_name("package_ctest_policy.ps1")


class PackageSourceGuardTests(unittest.TestCase):
    def setUp(self):
        if not POWERSHELL or not shutil.which("git"):
            self.skipTest("PowerShell and Git required")
        self.temporary = tempfile.TemporaryDirectory(prefix="winui3-package-test-")
        self.addCleanup(self.temporary.cleanup)
        root = Path(self.temporary.name)
        self.repo = root / "source with spaces"
        tools = self.repo / "tools"
        tools.mkdir(parents=True)
        shutil.copyfile(PACKAGE_SCRIPT, tools / PACKAGE_SCRIPT.name)
        shutil.copyfile(CTEST_POLICY, tools / CTEST_POLICY.name)
        (self.repo / "CMakeLists.txt").write_text("# Fixture: stop after source guard\n")
        (self.repo / ".gitignore").write_text("ignored-output/\n")
        self.build = root / "build"
        self.qt = root / "Qt"
        self.sources = root / "Qt sources"
        for base, relative in (
            (self.build, "CMakeCache.txt"),
            (self.build, "demo/Release/winui3style_gallery.exe"),
            (self.build, "src/Release/winui3style.dll"),
            (self.build, "plugins/styles/qwinui3style.dll"),
            (self.qt, "bin/windeployqt.exe"),
            (self.qt, "plugins/styles/qmodernwindowsstyle.dll"),
            (self.sources, "qtbase/LICENSES/LGPL-3.0-only.txt"),
        ):
            path = base / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        self.git("init", "--quiet")
        self.git("add", ".")
        self.git(
            "-c", "user.name=Package test", "-c", "user.email=package-test@example.invalid",
            "-c", "commit.gpgsign=false", "commit", "--quiet", "-m", "Fixture",
        )

    def git(self, *arguments):
        return subprocess.run(
            ["git", "-C", str(self.repo), *arguments], check=True,
            capture_output=True, text=True, errors="replace",
        )

    def package_error(self):
        result = subprocess.run(
            [
                POWERSHELL, "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
                "-File", str(self.repo / "tools" / PACKAGE_SCRIPT.name),
                "-BuildDir", str(self.build), "-QtRoot", str(self.qt),
                "-QtSourceRoot", str(self.sources),
            ], capture_output=True, text=True, errors="replace", timeout=30,
        )
        self.assertNotEqual(result.returncode, 0)
        return result.stdout + result.stderr

    def ctest_arguments(self, exclude_native=False):
        switch = " -ExcludeNativeTests" if exclude_native else ""
        command = (
            f". '{CTEST_POLICY.as_posix()}'; "
            f"(Get-PackageCTestArguments -BuildPath 'fixture'{switch}) -join '|'"
        )
        result = subprocess.run(
            [POWERSHELL, "-NoProfile", "-NonInteractive", "-Command", command],
            check=True, capture_output=True, text=True, errors="replace", timeout=30,
        )
        return result.stdout.strip().split("|")

    def test_package_ctest_defaults_to_full_suite(self):
        self.assertEqual(
            self.ctest_arguments(),
            ["--test-dir", "fixture", "-C", "Release", "--output-on-failure", "--no-tests=error"],
        )

    def test_package_ctest_switch_excludes_only_native_label(self):
        self.assertEqual(
            self.ctest_arguments(exclude_native=True),
            ["--test-dir", "fixture", "-C", "Release", "--output-on-failure", "--no-tests=error", "-LE", "native"],
        )

    def test_package_deploys_windows_comparison_style_and_vc_runtime_setup(self):
        source = PACKAGE_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("--include-plugins qmodernwindowsstyle", source)
        self.assertIn("--compiler-runtime", source)
        self.assertNotIn("--no-compiler-runtime", source)
        self.assertIn("plugins/styles/qmodernwindowsstyle.dll", source)
        self.assertIn("Join-Path $stage 'vc_redist.x64.exe'", source)
        self.assertIn("autoInstalled = $false", source)

    def test_package_startup_smoke_redirects_all_handles_and_checks_real_exit(self):
        source = PACKAGE_SCRIPT.read_text(encoding="utf-8")
        for handle in ("StandardInput", "StandardOutput", "StandardError"):
            self.assertIn(f"$smokeStartInfo.Redirect{handle} = $true", source)
        self.assertIn("$smoke.StandardInput.Close()", source)
        self.assertIn("$smoke.WaitForExit(30000)", source)
        self.assertIn("$smoke.ExitCode -ne 0", source)
        self.assertIn("$smokeOutput.Contains('--capture-dir')", source)

    def test_clean_source_reaches_next_precondition(self):
        self.assertIn("Cannot identify project version", self.package_error())

    def test_unstaged_source_is_rejected(self):
        (self.repo / "CMakeLists.txt").write_text("# Changed\n")
        self.assertRegex(self.package_error(), "Commit.*before packaging")

    def test_staged_source_is_rejected(self):
        (self.repo / "CMakeLists.txt").write_text("# Changed\n")
        self.git("add", "CMakeLists.txt")
        self.assertRegex(self.package_error(), "Commit.*before packaging")

    def test_untracked_source_is_rejected(self):
        (self.repo / "missing-from-source-archive.py").write_text("# New source\n")
        self.assertRegex(self.package_error(), "Commit.*before packaging")

    def test_ignored_build_output_is_allowed(self):
        ignored = self.repo / "ignored-output"
        ignored.mkdir()
        (ignored / "local-build.log").write_text("ignored\n")
        self.assertIn("Cannot identify project version", self.package_error())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--powershell", help="PowerShell executable selected by CMake")
    options, remaining = parser.parse_known_args()
    if options.powershell:
        POWERSHELL = options.powershell
    unittest.main(argv=[__file__, *remaining])
