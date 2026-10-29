#!/usr/bin/env python3
"""
Lädt die ausgewählten Bilder (auswahl-<sammlung>.json) von Wikimedia Commons, rechnet sie für das
Graustufen-Display um (Papierton weg, Kontrast, Ränder beschneiden, auf die Bildfläche einpassen) und
schreibt die Liste <sammlung>.json für das Plugin. Vorhandene Bilder werden nicht neu geladen.

    pip install pillow
    python3 zeichnen.py --sammlungen sammlungen.json --auswahl auswahl/ --ziel ausgabe/ --basis-url URL --minuten 50
"""
import argparse
import io
import json
import os
import sys
import time
import urllib.request

from PIL import Image, ImageFilter, ImageOps

UA = "cyd-weather-trmnl-bildarchiv/1.0 (https://github.com/HOAI-droid/cyd-weather)"


def lade(url):
    for versuch in range(4):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=90) as r:
                return r.read()
        except Exception as ex:
            print(f"  Laden fehlgeschlagen ({ex}), neuer Versuch", file=sys.stderr)
            time.sleep(6 * (versuch + 1))
    raise RuntimeError("Bild nicht ladbar")


def umrechnen(daten, w, h):
    im = Image.open(io.BytesIO(daten))
    im = ImageOps.exif_transpose(im).convert("L")
    im = ImageOps.autocontrast(im, cutoff=(1, 2))
    # Ränder (Papier, Scanrand) wegschneiden: Bereich mit Druck finden
    maske = im.point(lambda v: 255 if v < 200 else 0).filter(ImageFilter.MaxFilter(5))
    box = maske.getbbox()
    if box:
        x0, y0, x1, y1 = box
        if (x1 - x0) > im.width * .5 and (y1 - y0) > im.height * .5:
            r = 8
            im = im.crop((max(0, x0 - r), max(0, y0 - r), min(im.width, x1 + r), min(im.height, y1 + r)))
    im.thumbnail((w, h), Image.LANCZOS)
    im = im.filter(ImageFilter.UnsharpMask(radius=1.2, percent=60, threshold=2))
    # Papier ganz weiß, Druck kräftig: leichte Gradationskurve
    return im.point(lambda v: 255 if v > 232 else int(v * 0.96))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sammlungen", required=True)
    ap.add_argument("--auswahl", required=True)
    ap.add_argument("--ziel", required=True)
    ap.add_argument("--basis-url", default="")
    ap.add_argument("--minuten", type=float, default=50)
    ap.add_argument("--nur", default="")
    ap.add_argument("--ausgabe", default=os.environ.get("GITHUB_OUTPUT", ""))
    a = ap.parse_args()
    sam = json.load(open(a.sammlungen, encoding="utf-8"))
    os.makedirs(a.ziel, exist_ok=True)
    start, neu_gesamt, fehlend_gesamt = time.time(), 0, 0
    for name, cfg in sam.items():
        if a.nur and name not in a.nur.split(","):
            continue
        pfad = os.path.join(a.auswahl, f"auswahl-{name}.json")
        if not os.path.exists(pfad):
            continue
        auswahl = json.load(open(pfad, encoding="utf-8"))
        w, h = cfg.get("bildflaeche", [752, 404])
        for e in auswahl:
            datei = os.path.join(a.ziel, f"{name}-{e['tag']:03d}.png")
            if os.path.exists(datei):
                continue
            if (time.time() - start) / 60 > a.minuten:
                break
            try:
                bild = umrechnen(lade(e["quelle"]), w, h)
                bild.save(datei, optimize=True)
                neu_gesamt += 1
                print(f"{name} {e['tag']:3d}: {e['titel'][:60]} ({bild.width}×{bild.height})", flush=True)
            except Exception as ex:
                print(f"{name} {e['tag']:3d}: übersprungen ({ex})", file=sys.stderr, flush=True)
            time.sleep(.5)
        liste = []
        for e in auswahl:
            datei = f"{name}-{e['tag']:03d}.png"
            p = os.path.join(a.ziel, datei)
            if os.path.exists(p):
                with Image.open(p) as im:
                    bw, bh = im.size
                x = {k: v for k, v in e.items() if k not in ("quelle",)}
                x.update(bild=a.basis_url + datei, w=bw, h=bh)
                liste.append(x)
            else:
                fehlend_gesamt += 1
        with open(os.path.join(a.ziel, f"{name}.json"), "w", encoding="utf-8") as f:
            json.dump({"sammlung": name, "anzahl": len(liste), "bilder": liste,
                       "quelle": "Wikimedia Commons, gemeinfrei", "stand": time.strftime("%Y-%m-%d")}, f, ensure_ascii=False, indent=0)
        print(f"== {name}: {len(liste)} von {len(auswahl)} fertig")
    if a.ausgabe:
        with open(a.ausgabe, "a") as f:
            f.write(f"neu={neu_gesamt}\nfehlend={fehlend_gesamt}\n")


if __name__ == "__main__":
    main()
