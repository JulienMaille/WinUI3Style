#!/usr/bin/env python3
"""Mechanism tests for the render benchmark report parser and ratio ceiling."""

from __future__ import annotations

import unittest

from perf_compare import evaluate, parse_report, ratios


SAMPLE_OUTPUT = """\
style=WinUI3::Style
tokens-uncached ns/call=220.3 checksum=1069555253000000
tokens-cached ns/call=83.1 checksum=1069555253000000
button-accent-paint p50=1350.0 ns p95=1400.0 ns samples=40
list-icons p50=0.838 ms p95=0.977 ms samples=40
table-icons p50=1.802 ms p95=2.070 ms samples=40
dense-chrome p50=1.104 ms p95=1.198 ms samples=40
This plugin does not support propagateSizeHints()
"""


class ParseReportTest(unittest.TestCase):
    def test_extracts_scene_p50_lines_only(self):
        report = parse_report(SAMPLE_OUTPUT)
        self.assertEqual(report, {
            "list-icons": 0.838,
            "table-icons": 1.802,
            "dense-chrome": 1.104,
        })

    def test_empty_output_raises_keyerror_free_dict(self):
        self.assertEqual(parse_report(""), {})

    def test_ns_lines_do_not_leak_into_ms_report(self):
        self.assertNotIn("button-accent-paint", parse_report(SAMPLE_OUTPUT))


class RatiosTest(unittest.TestCase):
    def test_ratio_uses_common_scenes(self):
        candidate = {"dense-chrome": 1.4, "rich-surface-icons": 2.4, "extra": 1.0}
        baseline = {"dense-chrome": 1.0, "rich-surface-icons": 3.0}
        ratios_seen = ratios(candidate, baseline)
        self.assertEqual(sorted(ratios_seen), ["dense-chrome", "rich-surface-icons"])
        self.assertAlmostEqual(ratios_seen["dense-chrome"], 1.4)
        self.assertAlmostEqual(ratios_seen["rich-surface-icons"], 0.8)


class EvaluateTest(unittest.TestCase):
    def test_fails_missing_scene(self):
        failures = evaluate({"dense-chrome": 1.0}, ("dense-chrome", "rich-surface-icons"),
                            2.0)
        self.assertEqual(len(failures), 1)
        self.assertIn("missing", failures[0])

    def test_fails_above_ceiling(self):
        failures = evaluate({"dense-chrome": 2.5}, ("dense-chrome",), 2.0)
        self.assertEqual(len(failures), 1)
        self.assertIn("exceeds ceiling 2.00", failures[0])

    def test_passes_at_and_below_ceiling(self):
        self.assertEqual(evaluate({"dense-chrome": 2.0}, ("dense-chrome",), 2.0), [])
        self.assertEqual(evaluate({"dense-chrome": 1.2}, ("dense-chrome",), 2.0), [])


if __name__ == "__main__":
    unittest.main()
