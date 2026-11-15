from PIL import Image, ImageDraw, ImageFont
import os

SRC = r"C:/Users/julie/AppData/Local/Temp/issue64"
AFTER = r"D:/Codez/WinUI3Style/tmp/after_native.png"
OUT = r"D:/Codez/WinUI3Style/docs/validation/issue64/issue64-toggle-pill-before-after.png"

BG = (30, 30, 30)
FG = (225, 225, 225)
SUB = (170, 170, 170)
ACCENT = (140, 170, 255)

def font(sz, bold=False):
    name = "arialbd.ttf" if bold else "arial.ttf"
    try:
        return ImageFont.truetype("C:/Windows/Fonts/" + name, sz)
    except Exception:
        return ImageFont.load_default()

def bbox(px, w, h, pred):
    xs = []; ys = []
    for y in range(h):
        for x in range(w):
            if pred(px[x, y]):
                xs.append(x); ys.append(y)
    return (min(xs), min(ys), max(xs) + 1, max(ys) + 1)

def is_track(c):
    r, g, b = c[:3]
    return r > 110 and b > 140 and (b - r) > 20

def is_knob(c):
    r, g, b = c[:3]
    return r < 25 and g < 25 and b < 25

def load(path):
    im = Image.open(path).convert("RGB")
    return im, im.load(), im.size

def crop_scale(im, px, size, pred, margin, target_h):
    x0, y0, x1, y1 = bbox(px, size[0], size[1], pred)
    x0 = max(0, x0 - margin); y0 = max(0, y0 - margin)
    x1 = min(size[0], x1 + margin); y1 = min(size[1], y1 + margin)
    c = im.crop((x0, y0, x1, y1))
    w = round(c.width * target_h / c.height)
    return c.resize((w, target_h), Image.NEAREST)

panels = [
    ("WINUI 3 REFERENCE", os.path.join(SRC, "reference.png"), "Fixed radius-7 stadium (pill)"),
    ("BEFORE  (#64)", os.path.join(SRC, "ours.png"), "Ellipse inscribed in 17x14 (pointy ends)"),
    ("AFTER  (FIXED)", AFTER, "Fixed radius-7 stadium (pill)"),
]

H = 130
rows = []
for _, path, _ in panels:
    im, px, size = load(path)
    track = crop_scale(im, px, size, is_track, 3, H)
    knob = crop_scale(im, px, size, is_knob, 5, H)
    rows.append((track, knob))

margin = 28
gap = 36
colw = max(max(t.width, k.width) for t, k in rows) + 24
W = margin * 2 + colw * 3 + gap * 2
title_h = 30
head_h = 30
cap_h = 26
rowlabel_h = 24
rowgap = 18
H_total = margin + title_h + head_h + (H + cap_h) * 2 + rowlabel_h + rowgap + margin

canvas = Image.new("RGB", (W, H_total), BG)
d = ImageDraw.Draw(canvas)
d.text((margin, margin), "Issue 64 - Pressed ToggleSwitch knob shape - Dark - DPR 1",
       font=font(17, True), fill=FG)

y_head = margin + title_h
y_row1 = y_head + head_h
y_row2_label = y_row1 + H + cap_h + rowgap
y_row2 = y_row2_label + rowlabel_h

def col_x(i):
    return margin + i * (colw + gap)

for i, (head, _, cap) in enumerate(panels):
    cx = col_x(i)
    color = ACCENT if i == 2 else SUB
    d.text((cx, y_head), head, font=font(15, True), fill=color)

d.text((margin, y_row2_label), "Knob zoom (pressed, 17x14 box)", font=font(14, True), fill=SUB)

for i, (track, knob) in enumerate(rows):
    cx = col_x(i)
    tx = cx + (colw - track.width) // 2
    canvas.paste(track, (tx, y_row1))
    kx = cx + (colw - knob.width) // 2
    canvas.paste(knob, (kx, y_row2))
    cap = panels[i][2]
    d.text((cx, y_row1 + H + 6), cap, font=font(13), fill=SUB)

os.makedirs(os.path.dirname(OUT), exist_ok=True)
canvas.save(OUT)
print("saved", OUT, canvas.size)