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
import subprocess
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
    for versuch in range(5):
        url = SERVER[(versuch + int(abs(lat * 1000))) % len(SERVER)]
        try:
            req = urllib.request.Request(url, data=urllib.parse.urlencode({"data": q}).encode(),
                                         headers={"User-Agent": "cyd-weather-trmnl-schwarzplan/1.0"})
            with urllib.request.urlopen(req, timeout=150) as r:
                return json.loads(r.read().decode("utf-8"))
        except Exception as ex:  # 429, 504, Zeitüberschreitung: kurz warten, anderen Server versuchen
            letzter = ex
            print(f"  {url}: {ex}, neuer Versuch", file=sys.stderr)
            time.sleep(8 * (versuch + 1))
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
    ap.add_argument("--ziel", required=True, help="Ordner mit den Bildern; vorhandene werden nicht neu gezeichnet")
    ap.add_argument("--basis-url", default="", help="Adresse, unter der die Bilder später liegen")
    ap.add_argument("--nur", default="", help="nur diese Orte neu zeichnen (Kürzel mit Komma)")
    ap.add_argument("--minuten", type=float, default=50, help="Zeitbudget; danach keine neuen Orte mehr anfangen")
    ap.add_argument("--ausgabe", default=os.environ.get("GITHUB_OUTPUT", ""), help="Datei für fertig/fehlend (GitHub Actions)")
    ap.add_argument("--teil", type=int, default=0, help="nur jeden n-ten Ort (für parallele Läufe): dieser Teil")
    ap.add_argument("--teile", type=int, default=1, help="Anzahl paralleler Teile")
    ap.add_argument("--hochladen", default=os.environ.get("RELEASE_TAG", ""), help="jedes fertige Bild sofort in dieses Release laden")
    a = ap.parse_args()
    orte = json.load(open(a.orte, encoding="utf-8"))
    os.makedirs(a.ziel, exist_ok=True)
    nur = set(filter(None, a.nur.split(",")))
    halb_x, halb_y = BREITE / 2 * M_PRO_PX + 30, HOEHE / 2 * M_PRO_PX + 30
    fehlpfad = os.path.join(a.ziel, f"fehlgeschlagen-{a.teil}.json" if a.teile > 1 else "fehlgeschlagen.json")
    fehl = {}
    for f in os.listdir(a.ziel):  # Fehlschläge aller Teile zusammenführen
        if f.startswith("fehlgeschlagen") and f.endswith(".json"):
            try:
                for k, v in json.load(open(os.path.join(a.ziel, f), encoding="utf-8")).items():
                    fehl[k] = max(fehl.get(k, 0), v)
            except (OSError, ValueError):
                pass
    start, neu = time.time(), 0
    for i, o in enumerate(orte):
        if i % a.teile != a.teil:
            continue
        pfad = os.path.join(a.ziel, f"schwarzplan-{o['id']}.png")
        if nur:
            if o["id"] not in nur:
                continue
        elif os.path.exists(pfad) or fehl.get(o["id"], 0) >= 2:
            continue
        if (time.time() - start) / 60 > a.minuten:
            break
        print(f"Tag {o.get('tag', '?')}: {o['n']} · {o.get('sub', '')} …", flush=True)
        try:
            daten = overpass(o["lat"], o["lon"], halb_x, halb_y)
            bild, n = zeichne(daten, o["lat"], o["lon"])
            if n < 20:
                raise RuntimeError(f"nur {n} Gebäude gefunden")
            bild.save(pfad, optimize=True)
            neu += 1
            if a.hochladen:
                subprocess.run(["gh", "release", "upload", a.hochladen, pfad, "--clobber"], check=False)
            fehl.pop(o["id"], None)
            print(f"  {n} Gebäude, {os.path.getsize(pfad) // 1024} KB", flush=True)
        except Exception as ex:
            fehl[o["id"]] = fehl.get(o["id"], 0) + 1
            print(f"  übersprungen ({fehl[o['id']]}. Fehlschlag): {ex}", file=sys.stderr, flush=True)
        time.sleep(2)
    with open(fehlpfad, "w", encoding="utf-8") as f:
        json.dump(fehl, f, ensure_ascii=False, indent=1)
    fertig = []
    for o in orte:
        datei = f"schwarzplan-{o['id']}.png"
        if os.path.exists(os.path.join(a.ziel, datei)):
            fertig.append({"tag": o.get("tag", len(fertig) + 1), "id": o["id"], "n": o["n"], "sub": o.get("sub", ""),
                           "bild": a.basis_url + datei})
    fehlend = sum(1 for o in orte if not os.path.exists(os.path.join(a.ziel, f"schwarzplan-{o['id']}.png"))
                  and fehl.get(o["id"], 0) < 2)
    liste = {"m_pro_px": round(M_PRO_PX, 4), "breite": BREITE, "hoehe": HOEHE, "anzahl": len(fertig), "orte": fertig,
             "quelle": "Gebäude © OpenStreetMap-Mitwirkende (ODbL)", "stand": time.strftime("%Y-%m-%d")}
    with open(os.path.join(a.ziel, "schwarzplan.json"), "w", encoding="utf-8") as f:
        json.dump(liste, f, ensure_ascii=False, indent=1)
    print(f"Diese Runde neu: {neu} · fertig: {len(fertig)} von {len(orte)} · noch offen: {fehlend}")
    if a.ausgabe:
        with open(a.ausgabe, "a") as f:
            f.write(f"neu={neu}\nfehlend={fehlend}\nfertig={len(fertig)}\n")
    if not fertig:
        sys.exit(1)


if __name__ == "__main__":
    main()
