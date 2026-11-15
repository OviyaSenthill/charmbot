#!/usr/bin/env python3
"""Render a preview sheet of every Charmbot expression (approximates FaceApp.cpp).

Usage:  python3 tools/face_preview.py [out.png]
Needs:  pip install pillow

Keep the numbers here in sync with firmware/src/apps/FaceStyle.h when you
tweak the design, so you can iterate on your laptop before flashing.
"""
import math
import sys
from PIL import Image, ImageDraw, ImageFont

VISOR, EYE, EYE_GLOW = "#0B1530", "#3FE6F5", "#1A6A80"
ANGRY, ANGRY_GLOW = "#FF3B5C", "#7A1A2A"
LOVE, LOVE_GLOW = "#FF4D6D", "#7A2535"
SPR_X, SPR_Y, SPR_W, SPR_H = 40, 45, 240, 150
EYE_W, EYE_H, EYE_GAP, EYE_Y, MOUTH_Y, LINE_W, GLOW_PX = 36, 48, 100, 55, 112, 8, 5

EXPRESSIONS = [
    ("Neutral",   "oval", "oval", None,      None,         "cyan", False),
    ("Happy",     "oval", "oval", None,      "smile",      "cyan", False),
    ("Wink",      "oval", "arc",  None,      "small",      "cyan", False),
    ("Worried",   "oval", "oval", "worried", "frown",      "cyan", False),
    ("Angry",     "oval", "oval", "angry",   "frown",      "red",  False),
    ("Love",      "heart", "heart", None,    "small",      "pink", False),
    ("Surprised", "oval", "oval", None,      "o",          "cyan", True),
    ("Bored",     "line", "line", None,      "flat",       "cyan", False),
    ("Content",   "arc",  "arc",  None,      "small",      "cyan", False),
    ("Sad",       "oval", "oval", None,      "frown",      "cyan", False),
]


def circle(d, x, y, r, c):
    d.ellipse([x - r, y - r, x + r, y + r], fill=c)


def ellipse(d, x, y, rx, ry, c):
    d.ellipse([x - rx, y - ry, x + rx, y + ry], fill=c)


def thick_arc(d, cx, cy, r, a0, a1, w, c):
    for a in range(a0, a1 + 1, 4):
        rad = math.radians(a)
        circle(d, cx + r * math.cos(rad), cy + r * math.sin(rad), w / 2, c)


def thick_line(d, x0, y0, x1, y1, w, c):
    steps = max(abs(x1 - x0), abs(y1 - y0)) // 2 + 1
    for i in range(steps + 1):
        circle(d, x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps, w / 2, c)


def heart(d, cx, cy, r, c):
    circle(d, cx - r * 0.9, cy - r * 0.3, r, c)
    circle(d, cx + r * 0.9, cy - r * 0.3, r, c)
    d.polygon([(cx - r * 1.85, cy + r * 0.1), (cx + r * 1.85, cy + r * 0.1), (cx, cy + r * 1.9)], fill=c)


def eye(d, shape, cx, cy, big, col, glow):
    if shape == "oval":
        s = 1.2 if big else 1.0
        rx, ry = EYE_W / 2 * s, EYE_H / 2 * s
        ellipse(d, cx, cy, rx + GLOW_PX, ry + GLOW_PX, glow)
        ellipse(d, cx, cy, rx, ry, col)
    elif shape == "arc":
        thick_arc(d, cx, cy + 8, 16, 210, 330, LINE_W, col)
    elif shape == "line":
        thick_line(d, cx - 18, cy + 4, cx + 18, cy + 4, LINE_W, col)
    elif shape == "heart":
        heart(d, cx, cy - 6, 15, glow)
        heart(d, cx, cy - 6, 11, col)


def render(expr):
    name, le, re, brow, mouth, pal, big = expr
    col, glow = {"cyan": (EYE, EYE_GLOW), "red": (ANGRY, ANGRY_GLOW), "pink": (LOVE, LOVE_GLOW)}[pal]
    img = Image.new("RGB", (320, 240), VISOR)
    d = ImageDraw.Draw(img)
    ox, oy = SPR_X, SPR_Y
    lx, rx, ey = ox + SPR_W // 2 - EYE_GAP // 2, ox + SPR_W // 2 + EYE_GAP // 2, oy + EYE_Y
    eye(d, le, lx, ey, big, col, glow)
    eye(d, re, rx, ey, big, col, glow)
    by = ey - EYE_H // 2 - 12
    if brow == "worried":
        thick_line(d, lx - 16, by + 4, lx + 14, by - 6, LINE_W - 2, col)
        thick_line(d, rx - 14, by - 6, rx + 16, by + 4, LINE_W - 2, col)
    elif brow == "angry":
        thick_line(d, lx - 16, by - 6, lx + 14, by + 6, LINE_W - 2, col)
        thick_line(d, rx - 14, by + 6, rx + 16, by - 6, LINE_W - 2, col)
    mx, my = ox + SPR_W // 2, oy + MOUTH_Y
    if mouth == "smile":
        thick_arc(d, mx, my - 12, 20, 30, 150, LINE_W, col)
    elif mouth == "small":
        thick_arc(d, mx, my - 8, 12, 30, 150, LINE_W - 2, col)
    elif mouth == "frown":
        thick_arc(d, mx, my + 14, 16, 210, 330, LINE_W - 2, col)
    elif mouth == "o":
        ellipse(d, mx, my, 11, 13, col)
        ellipse(d, mx, my, 5, 7, VISOR)
    elif mouth == "flat":
        thick_line(d, mx - 16, my, mx + 16, my, LINE_W - 2, col)
    return name, img


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "docs/face-preview.png"
    pad, label = 16, 26
    cols, rows = 5, 2
    sheet = Image.new("RGB", (cols * (320 + pad) + pad, rows * (240 + pad + label) + pad), "#E8EEF5")
    d = ImageDraw.Draw(sheet)
    try:
        font = ImageFont.truetype("DejaVuSans-Bold.ttf", 18)
    except OSError:
        font = ImageFont.load_default()
    for i, expr in enumerate(EXPRESSIONS):
        name, img = render(expr)
        x = pad + (i % cols) * (320 + pad)
        y = pad + (i // cols) * (240 + pad + label)
        d.text((x, y), f"{i}. {name}", fill="#1A2238", font=font)
        sheet.paste(img, (x, y + label))
    sheet.save(out)
    print(f"wrote {out}")


if __name__ == "__main__":
    main()
