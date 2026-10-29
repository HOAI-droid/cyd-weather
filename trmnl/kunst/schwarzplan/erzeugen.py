#!/usr/bin/env python3
"""
Erzeugt Schwarzpläne (nur Gebäude, schwarz auf weiß) für das TRMNL-Display aus OpenStreetMap.

Alle Orte im selben Maßstab, je ein 1-Bit-PNG mit 800 × 480 Pixeln. Das Plugin zeigt davon den
Ausschnitt innerhalb des Rands; die Pixel bleiben dabei 1:1, nichts wird skaliert.

    pip install pillow
    python3 erzeugen.py --orte orte.json --ziel ausgabe/ --basis-url https://.../releases/download/TAG/

Gebäude: © OpenStreetMap-Mitwirkende, ODbL. Abfrage über die Overpass-API.
"""
import argparse
import json
import math
import os
import sys
import time
import urllib.parse
import urllib.request

from PIL import Image, ImageDraw

BREITE, HOEHE = 800, 480
M_PRO_PX = 2600 / 752          # 2,6 km über die Bildbreite innerhalb des mittleren Rands
UEBERABTASTUNG = 3
SERVER = ["https://overpass-api.de/api/interpreter", "https://overpass.kumi.systems/api/interpreter",
          "https://overpass.private.coffee/api/interpreter"]


def overpass(lat, lon, halb_x, halb_y):
    dlat = halb_y / 110574.0
    dlon = halb_x / (111320.0 * math.cos(math.radians(lat)))
    s, n, w, e = lat - dlat, lat + dlat, lon - dlon, lon + dlon
    q = (f'[out:json][timeout:180];(way["building"]({s},{w},{n},{e});'
         f'relation["building"]["type"="multipolygon"]({s},{w},{n},{e}););out geom;')
    letzter = None
    for versuch in range(6):
        url = SERVER[versuch % len(SERVER)]
        try:
            req = urllib.request.Request(url, data=urllib.parse.urlencode({"data": q}).encode(),
                                         headers={"User-Agent": "cyd-weather-trmnl-schwarzplan/1.0"})
            with urllib.request.urlopen(req, timeout=240) as r:
                return json.loads(r.read().decode("utf-8"))
        except Exception as ex:  # 429, 504, Zeitüberschreitung: kurz warten, anderen Server versuchen
            letzter = ex
            print(f"  {url}: {ex}, neuer Versuch", file=sys.stderr)
            time.sleep(10 * (versuch + 1))
    raise RuntimeError(f"Overpass nicht erreichbar: {letzter}")


def zeichne(daten, lat0, lon0):
    k = UEBERABTASTUNG
    img = Image.new("L", (BREITE * k, HOEHE * k), 255)
    d = ImageDraw.Draw(img)
    fx = 111320.0 * math.cos(math.radians(lat0)) / M_PRO_PX * k
    fy = 110574.0 / M_PRO_PX * k

    def pt(g):
        return (BREITE * k / 2 + (g["lon"] - lon0) * fx, HOEHE * k / 2 - (g["lat"] - lat0) * fy)

    anzahl = 0
    for el in daten.get("elements", []):
        if el["type"] == "way" and el.get("geometry"):
            p = [pt(g) for g in el["geometry"]]
            if len(p) >= 3:
                d.polygon(p, fill=0)
                anzahl += 1
        elif el["type"] == "relation":
            aussen = [m for m in el.get("members", []) if m.get("role") == "outer" and m.get("geometry")]
            innen = [m for m in el.get("members", []) if m.get("role") == "inner" and m.get("geometry")]
            for m in aussen:
                p = [pt(g) for g in m["geometry"]]
                if len(p) >= 3:
                    d.polygon(p, fill=0)
                    anzahl += 1
            for m in innen:
                p = [pt(g) for g in m["geometry"]]
                if len(p) >= 3:
                    d.polygon(p, fill=255)
    klein = img.resize((BREITE, HOEHE), Image.LANCZOS)
    return klein.point(lambda v: 0 if v < 128 else 255).convert("1"), anzahl


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--orte", required=True)
    ap.add_argument("--ziel", required=True)
    ap.add_argument("--basis-url", default="", help="Adresse, unter der die Bilder später liegen")
    ap.add_argument("--nur", default="", help="nur diese Orte (Kürzel, mit Komma getrennt)")
    a = ap.parse_args()
    orte = json.load(open(a.orte, encoding="utf-8"))
    os.makedirs(a.ziel, exist_ok=True)
    nur = set(filter(None, a.nur.split(",")))
    halb_x, halb_y = BREITE / 2 * M_PRO_PX + 30, HOEHE / 2 * M_PRO_PX + 30
    fertig = []
    for o in orte:
        datei = f"schwarzplan-{o['id']}.png"
        pfad = os.path.join(a.ziel, datei)
        if not nur or o["id"] in nur:
            print(f"{o['n']} …", flush=True)
            try:
                daten = overpass(o["lat"], o["lon"], halb_x, halb_y)
                bild, n = zeichne(daten, o["lat"], o["lon"])
                if n < 20:
                    raise RuntimeError(f"nur {n} Gebäude gefunden")
                bild.save(pfad, optimize=True)
                print(f"  {n} Gebäude, {os.path.getsize(pfad) // 1024} KB")
            except Exception as ex:
                print(f"  übersprungen: {ex}", file=sys.stderr)
            time.sleep(3)
        if os.path.exists(pfad):
            fertig.append({"id": o["id"], "n": o["n"], "sub": o.get("sub", ""), "bild": a.basis_url + datei})
    liste = {"m_pro_px": round(M_PRO_PX, 4), "breite": BREITE, "hoehe": HOEHE, "anzahl": len(fertig), "orte": fertig,
             "quelle": "Gebäude © OpenStreetMap-Mitwirkende (ODbL)", "stand": time.strftime("%Y-%m-%d")}
    with open(os.path.join(a.ziel, "schwarzplan.json"), "w", encoding="utf-8") as f:
        json.dump(liste, f, ensure_ascii=False, indent=1)
    print(f"{len(fertig)} von {len(orte)} Orten fertig")
    if not fertig:
        sys.exit(1)


if __name__ == "__main__":
    main()
