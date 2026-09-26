"""Erzeugt ein Beispiel-Radarbild (Karte + Regen) im Speicherformat der Firmware:
320x240 Bytes, obere 4 Bit Kartenhelligkeit, untere 4 Bit Regenstufe.
Die echte Firmware lädt dafür Kartenkacheln (CARTO/OSM) und Radarkacheln (RainViewer)."""
import math, pathlib, sys
from PIL import Image, ImageDraw, ImageFont

W, H = 320, 240
img = Image.new("L", (W, H), 34)  # Land
d = ImageDraw.Draw(img)
# Flüsse und Seen
d.line([(-10, 150), (40, 142), (80, 158), (120, 144), (170, 118), (205, 130), (260, 158), (330, 148)], fill=10, width=4)
d.line([(92, 250), (104, 210), (130, 190), (122, 158)], fill=10, width=3)
for x, y, rx, ry in [(118, 150, 15, 8), (206, 176, 10, 6), (92, 106, 8, 4), (232, 92, 12, 5), (60, 190, 9, 5)]:
    d.ellipse([x - rx, y - ry, x + rx, y + ry], fill=10)
# Autobahnring und Ausfallstraßen
d.ellipse([160 - 72, 120 - 56, 160 + 72, 120 + 56], outline=66, width=1)
for a in [8, 62, 128, 196, 246, 300, 340]:
    r = math.radians(a)
    d.line([(160 + 14 * math.cos(r), 120 + 14 * math.sin(r)), (160 + 260 * math.cos(r), 120 + 260 * math.sin(r))], fill=56, width=1)
# Ortsnamen wie auf den Kartenkacheln
font = ImageFont.truetype(str(pathlib.Path(__file__).parent.parent / "tools" / "Outfit.ttf"), 10)
for name, x, y in [("Potsdam", 104, 160), ("Oranienburg", 146, 44), ("Bernau", 214, 60), ("Nauen", 58, 90),
                   ("Strausberg", 256, 112), ("Zossen", 168, 214), ("Berlin", 178, 104)]:
    d.text((x, y), name, fill=150, font=font, anchor="mm")

cells = [(70, 58, 52, 30, .95, -25), (36, 114, 32, 22, .6, 10), (118, 32, 28, 16, .5, -10),
         (256, 200, 42, 22, .72, -15), (296, 164, 24, 16, .45, 0)]
out = bytearray(W * H)
px = img.load()
for y in range(H):
    for x in range(W):
        f = 0
        for cx, cy, rx, ry, i, rot in cells:
            a = -math.radians(rot)
            dx, dy = x - cx, y - cy
            u = dx * math.cos(a) - dy * math.sin(a)
            v = dx * math.sin(a) + dy * math.cos(a)
            f = max(f, i * math.exp(-(u * u / (rx * rx) + v * v / (ry * ry))))
        rain = 0 if f < .12 else 1 if f < .3 else 2 if f < .5 else 3 if f < .68 else 4 if f < .85 else 5
        m = min(15, px[x, y] * 15 // 160)
        out[y * W + x] = (m << 4) | rain
sys.stdout.buffer.write(bytes(out))
