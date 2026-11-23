#!/usr/bin/env python3
"""Draw the FUNKY-TRAIN icon as a 960x540 PNG using only the Python stdlib.

Run from the repository root:  python tools/make_icon.py
Writes firmware/funky-train/icon.png
"""

import math
import struct
import zlib
from pathlib import Path

W, H = 960, 540
SS = 2
SW, SH = W * SS, H * SS

NIGHT = (16, 12, 20)
PANEL = (29, 20, 40)
PANEL2 = (42, 27, 57)
ORANGE = (255, 159, 28)
GOLD = (242, 193, 78)
CREAM = (255, 240, 208)
AVO = (143, 164, 75)
VIOLET = (185, 135, 255)
BLUE = (70, 182, 255)
DARK = (9, 6, 13)


def blank():
    px = bytearray(SW * SH * 3)
    for y in range(SH):
        t = y / SH
        base = (NIGHT[0] + int(18 * t), NIGHT[1] + int(8 * t), NIGHT[2] + int(26 * t))
        o = y * SW * 3
        px[o:o + SW * 3] = bytes(base) * SW
    return px


def put(px, x, y, color):
    if 0 <= x < SW and 0 <= y < SH:
        o = (y * SW + x) * 3
        px[o:o + 3] = bytes(color)


def rect(px, x0, y0, x1, y1, color):
    x0, y0 = int(x0 * SS), int(y0 * SS)
    x1, y1 = int(x1 * SS), int(y1 * SS)
    x0, x1 = max(0, x0), min(SW, x1)
    y0, y1 = max(0, y0), min(SH, y1)
    row = bytes(color) * (x1 - x0)
    for y in range(y0, y1):
        o = (y * SW + x0) * 3
        px[o:o + len(row)] = row


def circle(px, cx, cy, r, color, inner=None):
    ellipse(px, cx, cy, r, r, color, inner=inner)


def ellipse(px, cx, cy, rx, ry, color, inner=None):
    cx, cy, rx, ry = cx * SS, cy * SS, rx * SS, ry * SS
    irx = iry = 0
    if inner is not None:
        irx, iry, inner = inner[0] * SS, inner[1] * SS, inner[2]
    reach = max(rx, ry) + 2
    for y in range(max(0, int(cy - reach)), min(SH, int(cy + reach + 1))):
        dy = y - cy
        for x in range(max(0, int(cx - reach)), min(SW, int(cx + reach + 1))):
            dx = x - cx
            if (dx / rx) ** 2 + (dy / ry) ** 2 <= 1.0:
                col = color
                if inner is not None and irx > 0 and iry > 0:
                    if (dx / irx) ** 2 + (dy / iry) ** 2 < 1.0:
                        col = inner
                put(px, x, y, col)


def ring(px, cx, cy, r, thick, color):
    cx, cy, r, thick = cx * SS, cy * SS, r * SS, thick * SS
    lo, hi = (r - thick) ** 2, (r + thick) ** 2
    for y in range(max(0, int(cy - r - thick)), min(SH, int(cy + r + thick + 1))):
        dy = y - cy
        for x in range(max(0, int(cx - r - thick)), min(SW, int(cx + r + thick + 1))):
            d = (x - cx) ** 2 + dy * dy
            if lo <= d <= hi:
                put(px, x, y, color)


def line(px, x0, y0, x1, y1, width, color):
    x0, y0, x1, y1 = x0 * SS, y0 * SS, x1 * SS, y1 * SS
    w = width * SS
    dx, dy = x1 - x0, y1 - y0
    ln2 = dx * dx + dy * dy
    for y in range(max(0, int(min(y0, y1) - w - 2)), min(SH, int(max(y0, y1) + w + 3))):
        for x in range(max(0, int(min(x0, x1) - w - 2)), min(SW, int(max(x0, x1) + w + 3))):
            t = 0.0 if ln2 == 0 else max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / ln2))
            qx, qy = x0 + t * dx, y0 + t * dy
            if (x - qx) ** 2 + (y - qy) ** 2 <= w * w:
                put(px, x, y, color)


def round_rect(px, x0, y0, x1, y1, r, color):
    cx0, cy0 = (x0 + r) * SS, (y0 + r) * SS
    cx1, cy1 = (x1 - r) * SS, (y1 - r) * SS
    r2 = (r * SS) ** 2
    for y in range(max(0, int(y0 * SS)), min(SH, int(y1 * SS) + 1)):
        for x in range(max(0, int(x0 * SS)), min(SW, int(x1 * SS) + 1)):
            dx = 0.0
            dy = 0.0
            if x < cx0: dx = x - cx0
            elif x > cx1: dx = x - cx1
            if y < cy0: dy = y - cy0
            elif y > cy1: dy = y - cy1
            if dx * dx + dy * dy <= r2:
                put(px, x, y, color)


def downsample(px):
    out = bytearray(W * H * 3)
    n = SS * SS
    for y in range(H):
        for x in range(W):
            r = g = b = 0
            for yy in range(SS):
                row = (y * SS + yy) * SW
                for xx in range(SS):
                    o = (row + x * SS + xx) * 3
                    r += px[o]
                    g += px[o + 1]
                    b += px[o + 2]
            o = (y * W + x) * 3
            out[o] = r // n
            out[o + 1] = g // n
            out[o + 2] = b // n
    return out


def png(rows):
    raw = b''.join(b'\x00' + bytes(row) for row in rows)

    def chunk(tag, data):
        return (struct.pack('>I', len(data)) + tag + data +
                struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff))

    return (b'\x89PNG\r\n\x1a\n' +
            chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))


def main():
    px = blank()

    # 80s stripe bars, like the firmware header.
    for y, h, col in ((0, 18, ORANGE), (H - 22, 22, AVO), (68, 5, PANEL2), (H - 74, 5, PANEL2)):
        rect(px, 0, y, W, y + h, col)

    # Neon sunset: one strong motif, with dark slats cutting the disc.
    ring(px, 470, 238, 196, 4, (77, 47, 105))
    ring(px, 470, 238, 174, 2, (61, 36, 86))
    circle(px, 470, 238, 148, GOLD, inner=(0.80, 0.80, ORANGE))
    for y, h in ((226, 9), (250, 11), (277, 13)):
        rect(px, 310, y, 630, y + h, NIGHT)

    # Speed lines: the train is moving, even as a tile.
    for y, x0, x1, col in (
        (128, 54, 238, VIOLET), (166, 24, 194, BLUE), (204, 88, 262, ORANGE),
        (332, 28, 214, BLUE), (368, 64, 246, VIOLET), (404, 18, 168, AVO)
    ):
        rect(px, x0, y, x1, y + 4, col)
    for y, x0, x1 in ((146, 706, 908), (190, 750, 936), (348, 722, 920), (390, 774, 944)):
        rect(px, x0, y, x1, y + 4, (72, 48, 98))

    # Rails and sleepers.
    line(px, 60, 452, 900, 452, 5, BLUE)
    line(px, 96, 474, 866, 474, 3, (45, 116, 155))
    for x in range(90, 890, 92):
        line(px, x, 448, x - 16, 482, 3, (52, 39, 70))

    # The train: violet body, cream windows, orange nose and gold lamp.
    round_rect(px, 252, 292, 706, 436, 26, DARK)
    round_rect(px, 260, 300, 698, 428, 22, VIOLET)
    rect(px, 260, 406, 698, 412, DARK)
    rect(px, 646, 316, 686, 402, ORANGE)
    circle(px, 666, 358, 15, GOLD)
    for x in (300, 372, 444, 516, 588):
        round_rect(px, x, 324, x + 52, 382, 8, CREAM)
        rect(px, x + 4, 372, x + 48, 378, (180, 156, 196))
    circle(px, 324, 432, 21, DARK, inner=(0.45, 0.45, ORANGE))
    circle(px, 456, 432, 21, DARK, inner=(0.45, 0.45, ORANGE))
    circle(px, 588, 432, 21, DARK, inner=(0.45, 0.45, ORANGE))
    rect(px, 272, 284, 462, 294, PANEL2)
    rect(px, 480, 284, 636, 294, PANEL2)
    circle(px, 404, 278, 7, GOLD)

    small = downsample(px)
    rows = [small[y * W * 3:(y + 1) * W * 3] for y in range(H)]
    out = Path(__file__).resolve().parent.parent / 'firmware' / 'funky-train' / 'icon.png'
    out.write_bytes(png(rows))
    print(f'wrote {out} ({out.stat().st_size} bytes)')


if __name__ == '__main__':
    main()
