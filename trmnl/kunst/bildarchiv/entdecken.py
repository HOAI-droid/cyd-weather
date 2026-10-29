#!/usr/bin/env python3
"""
Sucht auf Wikimedia Commons Bilder für die Bildarchive (Merian, Vögel, Karten) und schreibt
eine Kandidatenliste mit Bildangaben: kandidaten-<sammlung>.json

    python3 entdecken.py --sammlungen sammlungen.json --ziel ausgabe/ [--nur merian]

Nur Python-Standardbibliothek. Wikimedia verlangt eine aussagekräftige User-Agent-Kennung.
"""
import argparse
import html
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

API = "https://commons.wikimedia.org/w/api.php"
UA = "cyd-weather-trmnl-bildarchiv/1.0 (https://github.com/HOAI-droid/cyd-weather)"


def api(**p):
    p.update(format="json", formatversion="2")
    url = API + "?" + urllib.parse.urlencode(p)
    for versuch in range(5):
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=60) as r:
                return json.loads(r.read().decode("utf-8"))
        except Exception as ex:
            print(f"  API-Fehler {ex}, neuer Versuch", file=sys.stderr)
            time.sleep(5 * (versuch + 1))
    raise RuntimeError("Commons nicht erreichbar")


def text(v):
    v = re.sub(r"<[^>]+>", " ", v or "")
    return re.sub(r"\s+", " ", html.unescape(v)).strip()


def unterkategorien(kat, tiefe, gesehen):
    """Alle Kategorien bis zur Tiefe (Breitensuche), höchstens 400."""
    ergebnis, rand = [], [(kat, 0)]
    while rand and len(ergebnis) < 400:
        k, t = rand.pop(0)
        if k in gesehen:
            continue
        gesehen.add(k)
        ergebnis.append(k)
        if t >= tiefe:
            continue
        weiter = None
        while True:
            p = dict(action="query", list="categorymembers", cmtitle="Category:" + k, cmtype="subcat", cmlimit="500")
            if weiter:
                p["cmcontinue"] = weiter
            d = api(**p)
            for m in d.get("query", {}).get("categorymembers", []):
                rand.append((m["title"].split(":", 1)[1], t + 1))
            weiter = d.get("continue", {}).get("cmcontinue")
            if not weiter:
                break
    return ergebnis


def dateien(kat, mindestbreite):
    weiter = None
    while True:
        p = dict(action="query", generator="categorymembers", gcmtitle="Category:" + kat, gcmtype="file", gcmlimit="50",
                 prop="imageinfo", iiprop="url|size|mime|extmetadata", iiurlwidth="1600",
                 iiextmetadatafilter="ObjectName|ImageDescription|DateTimeOriginal|Artist|LicenseShortName|Categories")
        if weiter:
            p.update(weiter)
        d = api(**p)
        for s in d.get("query", {}).get("pages", []):
            ii = (s.get("imageinfo") or [{}])[0]
            if ii.get("mime") not in ("image/jpeg", "image/png", "image/tiff"):
                continue
            if (ii.get("width") or 0) < mindestbreite:
                continue
            em = ii.get("extmetadata", {})
            g = lambda k: text(em.get(k, {}).get("value", ""))
            yield {"datei": s["title"].split(":", 1)[1], "kategorie": kat, "breite": ii.get("width"), "hoehe": ii.get("height"),
                   "url": ii.get("thumburl") or ii.get("url"), "seite": ii.get("descriptionurl"),
                   "objekt": g("ObjectName"), "beschreibung": g("ImageDescription")[:400], "datum": g("DateTimeOriginal")[:60],
                   "kuenstler": g("Artist")[:120], "lizenz": g("LicenseShortName"), "kategorien": g("Categories")[:400]}
        weiter = d.get("continue")
        if not weiter:
            break
        weiter = {k: v for k, v in weiter.items() if k != "continue"}
        time.sleep(.3)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sammlungen", required=True)
    ap.add_argument("--ziel", required=True)
    ap.add_argument("--nur", default="")
    a = ap.parse_args()
    s = json.load(open(a.sammlungen, encoding="utf-8"))
    os.makedirs(a.ziel, exist_ok=True)
    for name, cfg in s.items():
        if a.nur and name not in a.nur.split(","):
            continue
        print(f"== {name}", flush=True)
        gesehen, alle, doppelt = set(), [], set()
        for wurzel in cfg["kategorien"]:
            kats = unterkategorien(wurzel, cfg.get("tiefe", 2), gesehen)
            n0 = len(alle)
            for k in kats:
                for f in dateien(k, cfg.get("mindestbreite", 1000)):
                    if f["datei"] in doppelt:
                        continue
                    doppelt.add(f["datei"])
                    f["wurzel"] = wurzel
                    alle.append(f)
                if len(alle) > 20000:
                    break
            print(f"  {wurzel}: {len(kats)} Kategorien, {len(alle) - n0} Bilder", flush=True)
        with open(os.path.join(a.ziel, f"kandidaten-{name}.json"), "w", encoding="utf-8") as f:
            json.dump(alle, f, ensure_ascii=False, indent=0)
        print(f"  zusammen {len(alle)} Bilder", flush=True)
        for x in alle[:: max(1, len(alle) // 15)][:15]:
            print("   ·", x["datei"], "|", x["objekt"][:60], "|", x["datum"][:20])


if __name__ == "__main__":
    main()
