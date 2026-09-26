#!/usr/bin/env python3
"""Erzeugt kantengeglättete TFT_eSPI-Smooth-Fonts (.vlw-Format) als C-Header.

Aufruf:  pip install pillow && python3 tools/make_fonts.py
Die Header landen in src/fonts/ und werden direkt in die Firmware kompiliert
(kein Dateisystem-Upload nötig).
"""
import pathlib
import struct
from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parent.parent
TTF = ROOT / "tools" / "Outfit.ttf"
OUT = ROOT / "src" / "fonts"

TEXT = [chr(c) for c in range(0x20, 0x7F)] + list("ÄÖÜäöüß°·–…é")
DIGITS = list(" 0123456789-°.:")

# name, pixelgröße, gewicht, zeichensatz
FONTS = [
    ("font_s", 13, 500, TEXT),   # Beschriftungen
    ("font_b", 16, 600, TEXT),   # Werte, Titel
    ("font_m", 20, 600, TEXT),   # Wetterlage
    ("font_c", 28, 600, TEXT),   # Uhr
    ("font_h", 64, 300, DIGITS), # große Temperatur
]


def glyph(font, ch):
    l, t, r, b = font.getbbox(ch, anchor="ls")
    adv = round(font.getlength(ch))
    w, h = max(0, r - l), max(0, b - t)
    if w == 0 or h == 0:
        return dict(w=0, h=0, dy=0, dx=0, adv=adv, bmp=b"")
    img = Image.new("L", (w + 4, h + 4), 0)
    ImageDraw.Draw(img).text((2 - l, 2 - t), ch, font=font, fill=255, anchor="ls")
    bmp = img.crop((2, 2, 2 + w, 2 + h)).tobytes()
    return dict(w=w, h=h, dy=-t, dx=l, adv=adv, bmp=bmp)


def build(name, size, weight, chars):
    font = ImageFont.truetype(str(TTF), size)
    try:
        font.set_variation_by_axes([weight])
    except Exception as e:  # pragma: no cover
        print("Warnung: variable Achse nicht gesetzt:", e)
    glyphs = [(ord(c), glyph(font, c)) for c in chars]
    asc = -font.getbbox("d", anchor="ls")[1] if "d" in chars else -font.getbbox("0", anchor="ls")[1]
    desc = font.getbbox("p", anchor="ls")[3] if "p" in chars else 1
    data = struct.pack(">6i", len(glyphs), 11, size, 0, asc, desc)
    for code, g in glyphs:
        data += struct.pack(">7i", code, g["h"], g["w"], g["adv"], g["dy"], g["dx"], 0)
    for _, g in glyphs:
        data += g["bmp"]
    lines = [f"// Automatisch erzeugt von tools/make_fonts.py – Outfit {weight}, {size}px (SIL OFL)",
             "#pragma once", "#include <pgmspace.h>", "",
             f"const uint8_t {name}[] PROGMEM = {{"]
    for i in range(0, len(data), 24):
        lines.append("  " + ",".join(f"0x{b:02X}" for b in data[i:i + 24]) + ",")
    lines.append("};\n")
    (OUT / f"{name}.h").write_text("\n".join(lines))
    print(f"{name}: {len(glyphs)} Zeichen, {len(data)} Bytes")


if __name__ == "__main__":
    OUT.mkdir(parents=True, exist_ok=True)
    for f in FONTS:
        build(*f)
