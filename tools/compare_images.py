#!/usr/bin/env python3
"""Compare two raster captures without hiding geometry or pixel differences."""

from __future__ import annotations

import argparse
import math
from pathlib import Path
from dataclasses import dataclass

from PIL import Image, ImageChops


@dataclass(frozen=True)
class Metrics:
    rms: float
    differing_pixels: int
    max_delta: int
    bbox: tuple[int, int, int, int] | None
    size: tuple[int, int]


def compare(reference: Path, candidate: Path, diff: Path | None) -> Metrics:
    with Image.open(reference) as reference_image:
        expected = reference_image.convert("RGBA")
    with Image.open(candidate) as candidate_image:
        actual = candidate_image.convert("RGBA")
    if expected.size != actual.size:
        raise ValueError(f"dimension mismatch: expected {expected.size}, got {actual.size}")
    delta = ImageChops.difference(expected, actual)
    histogram = delta.histogram()
    squares = sum((value % 256) ** 2 * count for value, count in enumerate(histogram))
    rms = math.sqrt(squares / (expected.width * expected.height * 4))
    pixels = delta.get_flattened_data()
    differing_pixels = sum(
        1 for pixel in pixels if pixel != (0, 0, 0, 0)
    )
    max_delta = max((max(pixel) for pixel in pixels), default=0)
    if diff:
        diff.parent.mkdir(parents=True, exist_ok=True)
        delta.save(diff)
    return Metrics(rms, differing_pixels, max_delta, delta.getbbox(), expected.size)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("reference", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--diff", type=Path)
    parser.add_argument("--max-rms", type=float, default=12.0)
    parser.add_argument("--max-different-pixels", type=int)
    arguments = parser.parse_args()
    try:
        metrics = compare(arguments.reference, arguments.candidate, arguments.diff)
    except ValueError as error:
        print(f"ERROR: {error}")
        return 2
    print(
        f"size={metrics.size[0]}x{metrics.size[1]} "
        f"rms={metrics.rms:.3f} differing_pixels={metrics.differing_pixels} "
        f"max_delta={metrics.max_delta} bbox={metrics.bbox}"
    )
    failed = metrics.rms > arguments.max_rms
    if (arguments.max_different_pixels is not None
            and metrics.differing_pixels > arguments.max_different_pixels):
        failed = True
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
