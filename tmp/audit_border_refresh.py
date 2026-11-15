import sys
from pathlib import Path
import numpy as np
from PIL import Image

baseline = Path(sys.argv[1])
candidate = Path(sys.argv[2])

def manifest(root):
    return {p.relative_to(root).as_posix(): p for p in root.rglob("*.png")}

base = manifest(baseline)
cand = manifest(candidate)
if base.keys() != cand.keys():
    print("MANIFEST MISMATCH", sorted(base.keys() ^ cand.keys()))
    sys.exit(1)

def erode(m):
    out = m.copy()
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            out &= np.roll(np.roll(m, dy, 0), dx, 1)
    out[0, :] = out[-1, :] = out[:, 0] = out[:, -1] = False
    return out

changed_files = 0
total_pixels = 0
violations = []
for rel in sorted(base):
    a = np.asarray(Image.open(base[rel]).convert("RGBA"), dtype=np.int16)
    b = np.asarray(Image.open(cand[rel]).convert("RGBA"), dtype=np.int16)
    if a.shape != b.shape:
        print("SIZE MISMATCH", rel)
        violations.append(rel)
        continue
    mask = np.any(a != b, axis=-1)
    n = int(mask.sum())
    if n == 0:
        continue
    changed_files += 1
    total_pixels += n
    ys, xs = np.nonzero(mask)
    thick = int(erode(mask).sum())
    print(f"{rel}: changed={n} bbox=({xs.min()},{ys.min()})-({xs.max()},{ys.max()}) thick3={thick}")
    if thick:
        violations.append(rel)

print(f"SUMMARY changed_files={changed_files} changed_pixels={total_pixels} identical={len(base)-changed_files}")
if violations:
    print("THICK-BAND VIOLATIONS:", violations)
    sys.exit(2)
print("AUDIT OK: all deltas are <=2px thin bands (border rings)")