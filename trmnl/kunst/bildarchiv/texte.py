#!/usr/bin/env python3
"""
Ergänzt die Vogelauswahl um deutschen Namen, Familie und einen kurzen Text (deutsche Wikipedia, Familie über Wikidata).
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
WD = "https://www.wikidata.org/w/api.php"


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


_cache = {}


def eintrag(qid):
    if qid not in _cache:
        d = hole(WD + "?" + urllib.parse.urlencode({"action": "wbgetentities", "ids": qid, "props": "claims|labels",
                                                     "languages": "de", "format": "json"}))
        _cache[qid] = ((d or {}).get("entities") or {}).get(qid, {})
    return _cache[qid]


def wert(e, p):
    for c in e.get("claims", {}).get(p, []):
        v = c.get("mainsnak", {}).get("datavalue", {}).get("value")
        if v is not None:
            return v["id"] if isinstance(v, dict) and "id" in v else v
    return None


def familie(qid):
    """Dem Elterntaxon (P171) folgen, bis der Rang Familie (Q35409) erreicht ist."""
    for _ in range(6):
        e = eintrag(qid)
        if wert(e, "P105") == "Q35409":
            de = e.get("labels", {}).get("de", {}).get("value", "")
            return de, wert(e, "P225") or ""
        qid = wert(e, "P171")
        if not qid:
            break
    return "", ""


def art(latein):
    """Lateinischer Name -> deutscher Artikel (die Wikipedia leitet weiter), Kurztext, Wikidata-Nummer."""
    d = hole("https://de.wikipedia.org/api/rest_v1/page/summary/" + urllib.parse.quote(latein.replace(" ", "_"), safe=""))
    if not d or d.get("type") != "standard" or not d.get("wikibase_item"):
        return None
    e = eintrag(d["wikibase_item"])
    if wert(e, "P105") != "Q7432":          # nur Arten
        return None
    return {"id": d["wikibase_item"], "de": d.get("title", ""), "gueltig": wert(e, "P225") or latein,
            "text": d.get("extract", "")}


def kurztext(t, grenze=330):
    t = re.sub(r"\s+", " ", t or "").strip()
    t = re.sub(r"\s*\([^()]*\)", "", t)          # Klammern (Autor, Jahr, Aussprache) weg
    aus = ""
    for s in re.split(r"(?<=[.!?])\s+(?=[A-ZÄÖÜ])", t):
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
    aus, ids = [], set()
    for e in auswahl:
        w = art(e["latein"])
        if not w or w["id"] in ids:
            print(f"  weg: {e['latein']}", file=sys.stderr)
            continue
        ids.add(w["id"])
        fde, flat = familie(w["id"])
        fam = f"Familie: {fde} ({flat})" if fde and flat and fde != flat else (f"Familie: {fde or flat}" if fde or flat else "")
        de = re.sub(r"\s*\(.*\)$", "", w["de"])
        e = dict(e, titel=de, latein=w["gueltig"], familie=fam, text=kurztext(w["text"]))
        aus.append(e)
        print(f"{de} · {e['latein']} · {fam}", flush=True)
        time.sleep(.2)
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
