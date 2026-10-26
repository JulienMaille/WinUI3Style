import io
import json
import tarfile
import tempfile
import unittest
from pathlib import Path

from extract_qt_notices import extract_notices


class QtNoticesTests(unittest.TestCase):
    def make_archive(self, root, license_file="COPYING", include_license=True):
        archive = root / "source.tar.xz"
        files = {
            "LICENSES/LGPL-3.0-only.txt": b"Qt license",
            "src/vendor/qt_attribution.json": json.dumps([
                {"LicenseFile": license_file, "Description": "line one\nline two"}
            ]).encode().replace(b"\\n", b"\n"),
            "src/vendor/code.cpp": b"unrelated source",
        }
        if include_license:
            files["src/vendor/COPYING"] = b"Vendor notice"
        with tarfile.open(archive, "w:xz") as output:
            for name, data in files.items():
                member = tarfile.TarInfo("qtbase-version/" + name)
                member.size = len(data)
                output.addfile(member, io.BytesIO(data))
        return archive

    def test_copies_every_referenced_notice_without_source_tree(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            output = root / "output"
            self.assertEqual(extract_notices(self.make_archive(root), output), 3)
            self.assertEqual((output / "LICENSES/LGPL-3.0-only.txt").read_bytes(), b"Qt license")
            self.assertEqual((output / "src/vendor/COPYING").read_bytes(), b"Vendor notice")
            self.assertTrue((output / "src/vendor/qt_attribution.json").is_file())
            self.assertFalse((output / "src/vendor/code.cpp").exists())

    def test_missing_referenced_notice_fails(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(ValueError, "Required notice not in archive"):
                extract_notices(self.make_archive(root, include_license=False), root / "output")

    def test_rejects_module_escape(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(ValueError, "escapes source module"):
                extract_notices(self.make_archive(root, "../../../escape"), root / "output")
            self.assertFalse((root / "escape").exists())


if __name__ == "__main__":
    unittest.main()
