#!/usr/bin/env python3
"""Mechanism tests for strict RGB/alpha snapshot comparison."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from PIL import Image

from compare_images import compare
from run_snapshot_matrix import compare_images


class ImageComparisonTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="winui3style-image-test-")
        self.addCleanup(self.temporary.cleanup)
        self.reference = Path(self.temporary.name) / "reference.png"
        self.candidate = Path(self.temporary.name) / "candidate.png"

    def assert_difference(self, expected: Image.Image, actual: Image.Image,
                          bbox: tuple[int, int, int, int], count: int) -> None:
        expected.save(self.reference)
        actual.save(self.candidate)
        metrics = compare(self.reference, self.candidate, None)
        with self.subTest(consumer="metrics"):
            self.assertEqual(metrics.bbox, bbox)
            self.assertEqual(metrics.differing_pixels, count)
            self.assertEqual(metrics.max_delta, 1)
            self.assertGreater(metrics.rms, 0)
        with self.subTest(consumer="snapshot"):
            with self.assertRaisesRegex(AssertionError, "pixel mismatch") as failure:
                compare_images(self.reference, self.candidate, "fixture.png")
            self.assertIn(f"bbox={bbox}", str(failure.exception))
            self.assertIn(f"differing_pixels={count}", str(failure.exception))
            self.assertIn("max_delta=1", str(failure.exception))

    def test_rgb_difference_with_unchanged_alpha(self) -> None:
        for alpha in (0, 73, 255):
            for channel in range(3):
                with self.subTest(alpha=alpha, channel=channel):
                    expected = Image.new("RGBA", (4, 3), (10, 20, 30, alpha))
                    actual = expected.copy()
                    pixel = list(actual.getpixel((2, 1)))
                    pixel[channel] += 1
                    actual.putpixel((2, 1), tuple(pixel))
                    self.assert_difference(expected, actual, (2, 1, 3, 2), 1)

    def test_bounds_include_rgb_and_alpha_changes(self) -> None:
        expected = Image.new("RGBA", (4, 3), (10, 20, 30, 255))
        actual = expected.copy()
        actual.putpixel((0, 0), (11, 20, 30, 255))
        actual.putpixel((3, 2), (10, 20, 30, 254))
        self.assert_difference(expected, actual, (0, 0, 4, 3), 2)

    def test_alpha_only_difference_is_rejected(self) -> None:
        expected = Image.new("RGBA", (4, 3), (10, 20, 30, 255))
        actual = expected.copy()
        actual.putpixel((2, 1), (10, 20, 30, 254))
        self.assert_difference(expected, actual, (2, 1, 3, 2), 1)

    def test_identical_images_pass(self) -> None:
        expected = Image.new("RGBA", (4, 3), (10, 20, 30, 255))
        expected.save(self.reference)
        expected.save(self.candidate)
        metrics = compare(self.reference, self.candidate, None)
        self.assertIsNone(metrics.bbox)
        self.assertEqual(metrics.differing_pixels, 0)
        self.assertEqual(metrics.max_delta, 0)
        self.assertEqual(metrics.rms, 0)
        compare_images(self.reference, self.candidate, "fixture.png")

    def test_dimension_mismatch_is_rejected(self) -> None:
        Image.new("RGBA", (4, 3)).save(self.reference)
        Image.new("RGBA", (3, 4)).save(self.candidate)
        with self.assertRaisesRegex(ValueError, "dimension mismatch"):
            compare(self.reference, self.candidate, None)
        with self.assertRaisesRegex(AssertionError, "dimension mismatch"):
            compare_images(self.reference, self.candidate, "fixture.png")


if __name__ == "__main__":
    unittest.main()
