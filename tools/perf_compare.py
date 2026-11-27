#!/usr/bin/env python3
"""Run the render benchmark with WinUI3Style and Fusion and enforce a ratio ceiling.

The comparison is self-normalizing: both styles are measured in the same process
type on the same machine, so runner speed cancels out. This is a budget ceiling
for order-of-magnitude regressions, not a tight baseline gate; see
docs/METHODOLOGY.md for why benchmarks never gate snapshot correctness.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

SAMPLE_LINE = re.compile(r"^(?P<name>[a-z0-9-]+) p50=(?P<p50>[0-9.]+) ms p95=(?P<p95>[0-9.]+) ms samples=(?P<samples>\d+)$")

# Scenes that stay gated. Item-view scenes depend heavily on model size and
# delegate work; the chrome scene is the button-family paint path that the
# style is expected to keep within reach of Fusion.
DEFAULT_GATED_SCENES = ("dense-chrome", "rich-surface-icons")


def parse_report(output: str) -> dict[str, float]:
    """Extract scene name -> p50 (ms) from render benchmark output."""
    samples: dict[str, float] = {}
    for line in output.splitlines():
        match = SAMPLE_LINE.match(line.strip())
        if match:
            samples[match["name"]] = float(match["p50"])
    return samples


def ratios(candidate: dict[str, float], baseline: dict[str, float]) -> dict[str, float]:
    """WinUI3Style/Fusion p50 ratio per scene present in both reports."""
    return {
        name: candidate[name] / baseline[name]
        for name in sorted(candidate.keys() & baseline.keys())
    }


def evaluate(scene_ratios: dict[str, float], gated: tuple[str, ...],
             max_ratio: float) -> list[str]:
    """Return human-readable failures for gated scenes above the ceiling."""
    failures = []
    for name in gated:
        ratio = scene_ratios.get(name)
        if ratio is None:
            failures.append(f"scene '{name}' missing from one benchmark run")
        elif ratio > max_ratio:
            failures.append(
                f"scene '{name}' p50 ratio {ratio:.2f} exceeds ceiling {max_ratio:.2f}")
    return failures


def run_benchmark(binary: Path, style: str, rows: int, iterations: int,
                  env: dict[str, str]) -> dict[str, float]:
    completed = subprocess.run(
        [str(binary), f"--qstyle={style}", f"--rows={rows}",
         f"--iterations={iterations}"],
        capture_output=True, text=True, check=True, env=env, timeout=900)
    report = parse_report(completed.stdout + completed.stderr)
    if not report:
        raise RuntimeError(f"no 'scene p50=' lines parsed from {style} run")
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True,
                        help="path to winui3style_render_benchmark")
    parser.add_argument("--rows", type=int, default=10000)
    parser.add_argument("--iterations", type=int, default=40)
    parser.add_argument("--max-ratio", type=float, default=1.5,
                        help="fail when a gated scene's WinUI3Style p50 exceeds "
                             "this multiple of Fusion's p50")
    parser.add_argument("--scenes", nargs="*", default=list(DEFAULT_GATED_SCENES))
    parser.add_argument("--report-only", action="store_true",
                        help="print the comparison without failing")
    options = parser.parse_args()

    env = dict(os.environ)
    env.setdefault("QT_QPA_PLATFORM", "offscreen")

    candidate = run_benchmark(options.binary, "winui3", options.rows,
                              options.iterations, env)
    baseline = run_benchmark(options.binary, "fusion", options.rows,
                             options.iterations, env)

    print("| Scene | WinUI3Style p50 | Fusion p50 | Ratio |")
    print("|---|---:|---:|---:|")
    scene_ratios = ratios(candidate, baseline)
    for name, ratio in scene_ratios.items():
        print(f"| {name} | {candidate[name]:.3f} ms | {baseline[name]:.3f} ms"
              f" | {ratio:.2f}x |")

    if options.report_only:
        return 0
    failures = evaluate(scene_ratios, tuple(options.scenes), options.max_ratio)
    for failure in failures:
        print(f"::error::{failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
