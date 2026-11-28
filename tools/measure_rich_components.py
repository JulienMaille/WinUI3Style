"""Measure baseline, candidate and Fusion in balanced order on one runner."""
import argparse
import csv
import itertools
import os
import re
import statistics
import subprocess
from collections import defaultdict
from pathlib import Path

REPORT = re.compile(r"^([\w-]+) p50=([\d.]+) ms p95=([\d.]+) ms", re.MULTILINE)
FRAME = re.compile(r"^frame ([\w-]+) ([0-9a-f]{64}) (\d+x\d+)$", re.MULTILINE)
SCENES = ("rich-surface-icons", "rich-table", "rich-toolbar", "rich-tabs")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("--rounds", type=int, default=12)
    args = parser.parse_args()
    variants = {
        "main": (args.before, "winui3"),
        "prototype": (args.after, "winui3"),
        "fusion": (args.before, "fusion"),
    }
    orders = tuple(itertools.permutations(variants))
    env = {**os.environ, "QT_QPA_PLATFORM": "offscreen", "QT_SCALE_FACTOR": "1",
           "WINUI3STYLE_DISABLE_ANIMATIONS": "1"}
    values = defaultdict(list)
    frames = defaultdict(set)
    with Path("windows-rich-measurements.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(("round", "variant", "scene", "p50_ms", "p95_ms", "sha256", "size"))
        for index in range(args.rounds):
            for key in orders[index % len(orders)]:
                executable, style = variants[key]
                result = subprocess.run(
                    [str(executable.resolve()), "--rich-probe", f"--qstyle={style}", "--iterations=40"],
                    env=env, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180,
                )
                output = result.stdout + result.stderr
                if result.returncode:
                    raise RuntimeError(f"{key} exited with {result.returncode}:\n{output}")
                reports = {name: (float(p50), float(p95)) for name, p50, p95 in REPORT.findall(output)}
                hashes = {name: (digest, size) for name, digest, size in FRAME.findall(output)}
                if any(scene not in reports or scene not in hashes for scene in SCENES):
                    raise RuntimeError(f"Incomplete report from {key}:\n{output}")
                for scene in SCENES:
                    p50, p95 = reports[scene]
                    digest, size = hashes[scene]
                    values[key, scene].append(p50)
                    frames[key, scene].add((digest, size))
                    writer.writerow((index + 1, key, scene, p50, p95, digest, size))
            stream.flush()
            print(f"Round {index + 1}/{args.rounds} complete", flush=True)
    print("| Scene | main p50 | Prototype p50 | Fusion p50 | Saved | main/Fusion | Prototype/Fusion | Pixels match |")
    print("|---|---:|---:|---:|---:|---:|---:|---|")
    for scene in SCENES:
        before, after, fusion = (statistics.median(values[key, scene]) for key in variants)
        same = len(frames["main", scene]) == 1 and frames["main", scene] == frames["prototype", scene]
        print(f"| {scene} | {before:.3f} ms | {after:.3f} ms | {fusion:.3f} ms | "
              f"{100*(1-after/before):.1f}% | {before/fusion:.3f} | {after/fusion:.3f} | {same} |")

if __name__ == "__main__":
    main()
