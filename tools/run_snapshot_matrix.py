#!/usr/bin/env python3
"""Run two fresh gallery capture processes and validate their image matrix."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageChops


def files(directory: Path) -> set[str]:
    return {
        path.relative_to(directory).as_posix()
        for path in directory.rglob("*")
        if path.is_file()
    }


def png_manifest(directory: Path) -> dict[str, dict[str, object]]:
    manifest: dict[str, dict[str, object]] = {}
    for relative in sorted(files(directory)):
        path = directory / relative
        with Image.open(path) as image:
            manifest[relative] = {
                "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                "size": list(image.size),
                "mode": image.mode,
            }
    return manifest


def compare_images(left: Path, right: Path, name: str) -> None:
    with Image.open(left) as left_image, Image.open(right) as right_image:
        left_rgba = left_image.convert("RGBA")
        right_rgba = right_image.convert("RGBA")
        if left_rgba.size != right_rgba.size:
            raise AssertionError(
                f"{name}: dimension mismatch {left_rgba.size} != {right_rgba.size}"
            )
        delta = ImageChops.difference(left_rgba, right_rgba)
        bbox = delta.getbbox()
        if bbox is None:
            return
        pixels = delta.get_flattened_data()
        differing = sum(pixel != (0, 0, 0, 0) for pixel in pixels)
        maximum = max((max(pixel) for pixel in pixels), default=0)
        raise AssertionError(
            f"{name}: pixel mismatch bbox={bbox} differing_pixels={differing} "
            f"max_delta={maximum}"
        )


def run_capture(gallery: Path, output: Path, environment: dict[str, str]) -> None:
    command = [str(gallery), "--capture-dir", str(output)]
    completed = subprocess.run(
        command,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=180,
    )
    if completed.returncode:
        raise AssertionError(
            f"gallery capture failed with {completed.returncode}:\n{completed.stdout}"
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--gallery", type=Path, required=True)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    arguments = parser.parse_args()

    if not arguments.gallery.is_file():
        print(f"ERROR: gallery executable not found: {arguments.gallery}")
        return 2

    environment = os.environ.copy()
    environment.update(
        {
            "QT_QPA_PLATFORM": "offscreen",
            "WINUI3STYLE_DISABLE_ANIMATIONS": "1",
            "QT_ACCESSIBILITY": "0",
        }
    )
    if os.name == "nt":
        # Qt's offscreen platform uses the basic font database and otherwise
        # searches Qt's obsolete lib/fonts directory. Use the system fonts so
        # snapshots validate real Segoe text and Fluent glyphs, not tofu boxes.
        environment.setdefault(
            "QT_QPA_FONTDIR",
            str(Path(os.environ.get("SystemRoot", r"C:\Windows")) / "Fonts"),
        )

    with tempfile.TemporaryDirectory(prefix="winui3style-snapshot-") as temporary:
        root = Path(temporary)
        first = root / "first"
        second = root / "second"
        run_capture(arguments.gallery, first, environment)
        run_capture(arguments.gallery, second, environment)

        first_files = files(first)
        second_files = files(second)
        if first_files != second_files:
            print(f"ERROR: snapshot manifest differs: only_first={sorted(first_files - second_files)} "
                  f"only_second={sorted(second_files - first_files)}")
            return 1
        first_manifest = png_manifest(first)
        second_manifest = png_manifest(second)
        if not first_manifest:
            print("ERROR: gallery produced no captures")
            return 1
        for relative in sorted(first_files):
            compare_images(first / relative, second / relative, relative)

        if not arguments.baseline.is_dir():
            print(f"ERROR: approved baseline directory is missing: {arguments.baseline}")
            return 1
        baseline_manifest = png_manifest(arguments.baseline)
        if baseline_manifest.keys() != first_manifest.keys():
            print(
                "ERROR: approved baseline manifest differs: "
                f"only_baseline={sorted(baseline_manifest.keys() - first_manifest.keys())} "
                f"only_candidate={sorted(first_manifest.keys() - baseline_manifest.keys())}"
            )
            return 1
        for relative in sorted(first_files):
            compare_images(first / relative, arguments.baseline / relative, relative)
        print(f"approved baseline: {arguments.baseline}")

        report = {
            "files": first_manifest,
            "deterministic": True,
            "baseline_checked": True,
        }
        if arguments.report:
            arguments.report.parent.mkdir(parents=True, exist_ok=True)
            arguments.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(f"deterministic snapshot matrix: {len(first_files)} files, pixel-exact")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (AssertionError, OSError, ValueError) as error:
        print(f"ERROR: {error}")
        raise SystemExit(1)
