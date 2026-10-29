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


def dunkle_raender_weg(im):
    """Schwarze oder dunkle Ränder (Buch fotografiert vor dunklem Grund) abschneiden."""
    klein = im.resize((200, max(1, int(200 * im.height / im.width))))
    w, h = klein.size
    px = klein.load()
    spalte = lambda x: sum(px[x, y] for y in range(h)) / h
    zeile = lambda y: sum(px[x, y] for x in range(w)) / w
    l, r, o, u = 0, w - 1, 0, h - 1
    while l < w // 4 and spalte(l) < 90: l += 1
    while r > w * 3 // 4 and spalte(r) < 90: r -= 1
    while o < h // 4 and zeile(o) < 90: o += 1
    while u > h * 3 // 4 and zeile(u) < 90: u -= 1
    if (l, r, o, u) == (0, w - 1, 0, h - 1):
        return im
    f = im.width / w
    rand = 2 if l or r < w - 1 or o or u < h - 1 else 0
    return im.crop((int((l + rand) * f), int((o + rand) * f), int((r + 1 - rand) * f), int((u + 1 - rand) * f)))


def umrechnen(daten, w, h):
    im = Image.open(io.BytesIO(daten))
    im = ImageOps.exif_transpose(im).convert("L")
    im = ImageOps.autocontrast(im, cutoff=(1, 2))
    im = dunkle_raender_weg(im)
    # Papierton (oft grau oder gelblich) auf Weiß ziehen: der helle Großteil der Fläche ist Papier
    hist, n, summe = im.histogram(), im.width * im.height, 0
    papier = 255
    for v in range(255, -1, -1):
        summe += hist[v]
        if summe > n * .30:
            papier = max(v, 120)
            break
    im = im.point(lambda v: min(255, int(v * 255 / papier)))
    # Ränder (Papier, Scanrand) wegschneiden: Zeilen und Spalten mit nennenswert Druck suchen,
    # einzelne Flecken auf dem Papier zählen nicht
    klein = im.resize((300, max(1, int(300 * im.height / im.width))))
    kw, kh = klein.size
    px = klein.load()
    dunkel = lambda v: v < 150
    spalten = [sum(dunkel(px[x, y]) for y in range(kh)) / kh for x in range(kw)]
    zeilen = [sum(dunkel(px[x, y]) for x in range(kw)) / kw for y in range(kh)]
    xs = [i for i, v in enumerate(spalten) if v > .06]
    ys = [i for i, v in enumerate(zeilen) if v > .06]
    if xs and ys:
        x0, x1, y0, y1 = xs[0], xs[-1] + 1, ys[0], ys[-1] + 1
        if (x1 - x0) > kw * .45 and (y1 - y0) > kh * .45:
            f, r = im.width / kw, 1
            im = im.crop((int(max(0, x0 - r) * f), int(max(0, y0 - r) * f), int(min(kw, x1 + r) * f), int(min(kh, y1 + r) * f)))
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
