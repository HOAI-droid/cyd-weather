#!/usr/bin/env python3
"""
Ergänzt die Vogelauswahl um deutschen Namen, Familie und einen kurzen Text (Wikidata + deutsche Wikipedia).
Einträge, deren lateinischer Name sich nicht finden lässt (z. B. Lesefehler im Scan), fallen weg.
Danach wird auf --anzahl gekürzt und die Tage 1..n werden neu vergeben.

    python3 texte.py --ein auswahl/auswahl-voegel.json --aus ausgabe/auswahl-voegel.json --anzahl 120
"""
import argparse
import hashlib
import json
import re
import sys
import time
import urllib.parse
import urllib.request

UA = "cyd-weather-trmnl-bildarchiv/1.0 (https://github.com/HOAI-droid/cyd-weather)"
SPARQL = "https://query.wikidata.org/sparql"


def hole(url, daten=None):
    for versuch in range(5):
        try:
            req = urllib.request.Request(url, data=daten, headers={"User-Agent": UA, "Accept": "application/json"})
            with urllib.request.urlopen(req, timeout=90) as r:
                return json.loads(r.read().decode("utf-8"))
        except urllib.error.HTTPError as ex:
            if ex.code == 404:
                return None
            print(f"  Fehler {ex}, neuer Versuch", file=sys.stderr)
        except Exception as ex:
            print(f"  Fehler {ex}, neuer Versuch", file=sys.stderr)
        time.sleep(5 * (versuch + 1))
    return None


def wikidata(namen):
    """Lateinischer Name (auch alte Synonyme) -> Wikidata-Eintrag mit deutschem Namen, Familie und Artikel."""
    werte = " ".join('"%s"' % n.replace('"', "") for n in namen)
    q = """
    SELECT ?name ?art ?artLabel ?famLabel ?famLat ?artikel ?gueltig WHERE {
      VALUES ?name { %s }
      { ?art wdt:P225 ?name . BIND(?name AS ?gueltig) }
      UNION { ?art wdt:P1420 ?syn . ?syn wdt:P225 ?name . ?art wdt:P225 ?gueltig . }
      ?art wdt:P105 wd:Q7432 .
      OPTIONAL { ?art wdt:P171+ ?fam . ?fam wdt:P105 wd:Q35409 . ?fam wdt:P225 ?famLat . }
      OPTIONAL { ?artikel schema:about ?art ; schema:isPartOf <https://de.wikipedia.org/> . }
      SERVICE wikibase:label { bd:serviceParam wikibase:language "de". }
    }""" % werte
    d = hole(SPARQL + "?" + urllib.parse.urlencode({"query": q, "format": "json"}))
    erg = {}
    for b in (d or {}).get("results", {}).get("bindings", []):
        g = lambda k: b.get(k, {}).get("value", "")
        n = g("name")
        if n in erg and erg[n]["artikel"]:
            continue
        erg[n] = {"id": g("art").rsplit("/", 1)[-1], "de": g("artLabel"), "fam": g("famLabel"), "famlat": g("famLat"),
                  "artikel": g("artikel"), "gueltig": g("gueltig")}
    return erg


def kurztext(artikel, grenze=330):
    if not artikel:
        return ""
    titel = urllib.parse.unquote(artikel.rsplit("/", 1)[-1])
    d = hole("https://de.wikipedia.org/api/rest_v1/page/summary/" + urllib.parse.quote(titel, safe=""))
    t = re.sub(r"\s+", " ", (d or {}).get("extract", "")).strip()
    t = re.sub(r"\s*\([^()]*\)", "", t)          # Klammern (Autor, Jahr, Aussprache) weg
    saetze, aus = re.split(r"(?<=[.!?])\s+(?=[A-ZÄÖÜ])", t), ""
    for s in saetze:
        if len(aus) + len(s) + 1 > grenze and aus:
            break
        aus = (aus + " " + s).strip()
    return aus[:grenze]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ein", required=True)
    ap.add_argument("--aus", required=True)
    ap.add_argument("--anzahl", type=int, default=120)
    a = ap.parse_args()
    auswahl = json.load(open(a.ein, encoding="utf-8"))
    namen = sorted({e["latein"] for e in auswahl})
    wd = {}
    for i in range(0, len(namen), 40):
        wd.update(wikidata(namen[i:i + 40]))
        time.sleep(1)
    aus, ids = [], set()
    for e in auswahl:
        w = wd.get(e["latein"])
        if not w or not w["de"] or re.match(r"^Q\d+$", w["de"]) or w["id"] in ids:
            print(f"  weg: {e['latein']}", file=sys.stderr)
            continue
        ids.add(w["id"])
        de = w["de"][:1].upper() + w["de"][1:]
        fam = ""
        if w["fam"] and not re.match(r"^Q\d+$", w["fam"]):
            fam = w["fam"][:1].upper() + w["fam"][1:]
            fam = f"Familie: {fam} ({w['famlat']})" if w["famlat"] and w["famlat"] != w["fam"] else f"Familie: {fam}"
        e = dict(e, titel=de, latein=w["gueltig"] or e["latein"], familie=fam, text=kurztext(w["artikel"]))
        aus.append(e)
        print(f"{de} · {e['latein']} · {fam}", flush=True)
        time.sleep(.3)
        if len(aus) >= a.anzahl:
            break
    aus = aus[:1] + sorted(aus[1:], key=lambda e: hashlib.md5(e["quelle"].encode()).hexdigest())
    for t, e in enumerate(aus, 1):
        e["tag"] = t
    with open(a.aus, "w", encoding="utf-8") as f:
        json.dump(aus, f, ensure_ascii=False, indent=1)
    print(f"== {len(aus)} Vögel")


if __name__ == "__main__":
    main()
