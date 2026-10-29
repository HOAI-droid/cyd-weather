#!/usr/bin/env python3
"""
Wählt aus den Kandidatenlisten (kandidaten-<sammlung>.json, von entdecken.py) je Sammlung
bis zu 120 Bilder aus und schreibt auswahl-<sammlung>.json mit Titel und Bildzeile.

    python3 auswaehlen.py --kandidaten ordner/ --ziel auswahl/ [--anzahl 120]

Nur Python-Standardbibliothek. Die Auswahl ist fest (kein Zufall), damit jeder Lauf gleich ausfällt.
"""
import argparse
import hashlib
import json
import os
import re

# ---------------------------------------------------------------- Merian
BAENDE = [  # (Kennwort im Dateinamen oder in der Kategorie, Bandtitel, Jahr)
    ("Helvetiae", "Topographia Helvetiae", 1642), ("Sueviae", "Topographia Sueviae", 1643),
    ("Alsatiae", "Topographia Alsatiae", 1644), ("Bavariae", "Topographia Bavariae", 1644),
    ("Palatinatus", "Topographia Palatinatus Rheni", 1645), ("Hassiae", "Topographia Hassiae", 1646),
    ("Mainz Trier", "Topographia Archiepiscopatuum Moguntinensis, Treverensis et Coloniensis", 1646),
    ("Moguntin", "Topographia Archiepiscopatuum Moguntinensis, Treverensis et Coloniensis", 1646),
    ("Westphal", "Topographia Westphaliae", 1647), ("Franconiae", "Topographia Franconiae", 1648),
    ("Austria", "Topographia Provinciarum Austriacarum", 1649), ("Superioris Saxoniae", "Topographia Superioris Saxoniae", 1650),
    ("Bohemiae", "Topographia Bohemiae, Moraviae et Silesiae", 1650),
    ("Brandenburg", "Topographia Electoratus Brandenburgici", 1652), ("Saxoniae Inferioris", "Topographia Saxoniae Inferioris", 1653),
    ("Braunschweig", "Topographia Braunschweig und Lüneburg", 1654), ("Burgund", "Topographia Circuli Burgundici", 1654),
    ("Zeiller", "Topographia Galliae", 1657), ("Galliae", "Topographia Galliae", 1657),
]
DEUTSCH = {"Cologne": "Köln", "Munich": "München", "Vienna": "Wien", "Prague": "Prag", "Nuremberg": "Nürnberg",
           "Brunswick": "Braunschweig", "Hanover": "Hannover", "Lorraine": "Lothringen", "Strasbourg": "Straßburg",
           "Geneva": "Genf", "Lucerne": "Luzern", "Brussels": "Brüssel", "Constance": "Konstanz", "Ratisbon": "Regensburg",
           "Aachen": "Aachen", "Wrocław": "Breslau", "Brno": "Brünn", "Olomouc": "Olmütz", "Cheb": "Eger",
           "Gdańsk": "Danzig", "Szczecin": "Stettin", "Ljubljana": "Laibach", "Bratislava": "Pressburg",
           "Bolzano": "Bozen", "Trento": "Trient", "Mulhouse": "Mülhausen", "Colmar": "Colmar", "Metz": "Metz",
           "Kassel": "Kassel", "Lvbecca": "Lübeck", "Luebeck": "Lübeck", "Graz": "Graz", "Linz": "Linz", "Innsbruck": "Innsbruck", "Basel": "Basel"}
# Bekannte Orte zuerst (in dieser Reihenfolge), danach der Rest
BEKANNT = """Frankfurt am Main|Köln|Nürnberg|Augsburg|München|Wien|Prag|Salzburg|Heidelberg|Mainz|Straßburg|Basel|Zürich|
Bern|Luzern|Genf|Konstanz|Ulm|Regensburg|Würzburg|Bamberg|Dresden|Leipzig|Erfurt|Weimar|Jena|Magdeburg|Halle|Berlin|
Potsdam|Braunschweig|Hannover|Lüneburg|Hildesheim|Goslar|Göttingen|Kassel|Marburg|Gießen|Darmstadt|Wiesbaden|Worms|
Speyer|Trier|Koblenz|Bonn|Aachen|Düsseldorf|Münster|Osnabrück|Bremen|Hamburg|Lübeck|Stralsund|Stettin|Danzig|Breslau|
Innsbruck|Graz|Linz|Bozen|Trient|Freiburg im Breisgau|Tübingen|Stuttgart|Esslingen|Heilbronn|Schaffhausen|St. Gallen|
Lindau|Überlingen|Meersburg|Rothenburg ob der Tauber|Dinkelsbühl|Nördlingen|Coburg|Eisenach|Gotha|Wittenberg|Meißen|
Freiberg|Görlitz|Bautzen|Zittau|Paris|Lyon|Rouen|Orléans|Reims|Amiens|Bordeaux|Marseille|Nîmes|Avignon|Dijon|Metz|Nancy|
Colmar|Hagenau|Landau in der Pfalz|Kreuznach|Bacharach|Oppenheim|Fulda|Hanau|Wetzlar|Limburg an der Lahn|Heilbad Heiligenstadt|
Quedlinburg|Wernigerode|Halberstadt|Celle|Wolfenbüttel|Hameln|Minden|Paderborn|Soest|Dortmund|Lemgo|Bielefeld|Hamm""".replace("\n", "").split("|")
UNSINN = re.compile(r"castles|canton|buildings|churches|by Johannes|Blaeu|Topographia|Merian|Germany|Austria|Bohemia|Saxony|"
                    r"Hesse|Bavaria|France|Switzerland|Tyrol|Swabia|Franconia|Lower|Upper|towns|cities|art |art$|region|district$", re.I)


def schluessel(o):
    """Gleiche Orte in alter und neuer Schreibweise zusammenlegen (Magdeburgk/Magdeburg, Statt Metz/Metz)."""
    o = re.sub(r"^(statt|stadt|schloß|closter) ", "", o.lower())
    for a, b in (("ß", "s"), ("ss", "s"), ("ue", "u"), ("ü", "u"), ("ö", "o"), ("ä", "a"), ("v", "u"), ("w", "u")):
        o = o.replace(a, b)
    return re.sub(r"[^a-z]", "", o)[:6]


def merian_band(x):
    for wort, titel, jahr in BAENDE:
        if wort in x["datei"] or wort in x["kategorie"] or wort in x["kategorien"]:
            return titel, jahr
    return "Topographia Germaniae", 1650


def merian_ort(x):
    ks = x["kategorien"].split("|")
    for k in ks:
        m = re.match(r"^(.+?) in the 1[67]th century$", k)
        if m:
            return m.group(1)
    m = re.match(r"^(.+?)[ _-]*\(Merian\)\.\w+$", x["datei"])
    if m and not re.search(r"\d{2,3}$", m.group(1)) and not m.group(1).startswith(("Topographia", "De Merian")):
        return m.group(1)
    m = re.match(r"^(.+?)[ -]+1\d{3}[ -]+(?:Merian|Zeiller)", x["datei"])
    if m:
        return m.group(1)
    for k in ks:
        m = re.match(r"^(?:Maps|History|Views|Engravings|Old maps) of (.+)$", k)
        if m and not UNSINN.search(m.group(1)):
            return m.group(1)
    return None


def merian(kand, anzahl):
    orte, namen = {}, {}
    for x in kand:
        if re.search(r"Self-published|Photographs taken|photographs taken", x["kategorien"]) or re.match(r"^(19|20)\d\d", x["datum"]):
            continue
        v = x["breite"] / max(1, x["hoehe"])
        if not 1.25 < v < 3.2:          # Ansichten sind Querformat, Textseiten nicht
            continue
        o = merian_ort(x)
        if not o:
            continue
        o = re.sub(r"\s*\((?!Merian).*?\)$", "", o).split(", before")[0].strip(" -")
        o = re.sub(r"\s+(J|Johan|Johann) Peeters$", "", o)
        o = re.sub(r"^(Maps|Views) of ", "", o)
        o = re.sub(r"\s+(Plan|CH)$", "", o)
        if re.match(r"^(Landkreis|Carte|Marckt in) ", o):
            continue
        o = o[:1] + o[1:].lower() if re.match(r"^[A-Z]{2}[a-z]", o) else o
        o = DEUTSCH.get(o, o)
        if UNSINN.search(o) or len(o) > 38:
            continue
        g = -abs(v - 1.86) + min(x["breite"], 4000) / 8000
        k = schluessel(o)
        namen.setdefault(k, set()).add(o)
        if k not in orte or g > orte[k][0]:
            orte[k] = (g, x)
    # Name: bekannte Schreibweise vor der kürzesten
    name = lambda k: min(namen[k], key=lambda n: (n not in BEKANNT, len(n)))
    orte = {name(k): v for k, v in orte.items()}
    # Danach reihum aus allen Bänden, damit nicht nur ein Landstrich kommt
    reihe = [o for o in BEKANNT if o in orte]
    nach_band = {}
    for o in sorted(o for o in orte if o not in BEKANNT):
        nach_band.setdefault(merian_band(orte[o][1])[0], []).append(o)
    while any(nach_band.values()) and len(reihe) < anzahl:
        for b in sorted(nach_band):
            if nach_band[b]:
                reihe.append(nach_band[b].pop(len(nach_band[b]) // 2))
    aus = []
    for o in reihe[:anzahl]:
        x = orte[o][1]
        band, jahr = merian_band(x)
        j = re.search(r"\b(1[56]\d\d)\b", x["datei"])
        jahr = int(j.group(1)) if j and 1600 < int(j.group(1)) < 1700 else jahr
        stecher = "Matthäus Merian" if "Galliae" not in band else "Merian & Zeiller"
        aus.append({"titel": o, "zeile": f"{stecher} · {band} · {jahr}", "jahr": jahr,
                    "quelle": x["url"], "seite": x["seite"]})
    return aus


# ---------------------------------------------------------------- Vögel
def voegel(kand, anzahl):
    arten, gesehen = {}, set()
    for x in kand:
        if "naumanni" in x["kategorie"].lower() or "Denkmal" in x["kategorie"] or "Self-published" in x["kategorien"]:
            continue
        if re.match(r"^(19[2-9]|20)\d\d", x["datum"]):
            continue
        a = [m.group(1) for k in x["kategorien"].split("|") for m in [re.match(r"^([A-Z][a-z]+ [a-z]+) \(illustrations\)$", k)] if m]
        if not a and x["datei"].startswith("Naumann ("):
            m = re.search(r"\b([A-Z][a-z]+ [a-z]{4,})\b", x["beschreibung"])
            a = [m.group(1)] if m else []
        if not a:
            continue
        # Tafeln mit einer Art zuerst, große Scans bevorzugt
        g = -len(a) * 2 + min(x["breite"], 2000) / 1000 + (1 if "Naumann" in x["kategorie"] else 0)
        for art in a[:1]:
            if art not in arten or g > arten[art][0]:
                arten[art] = (g, x, len(a))
    reihe = sorted(arten.items(), key=lambda kv: (-kv[1][0], kv[0]))
    aus = []
    for art, (g, x, n) in reihe:
        if x["datei"] in gesehen:
            continue
        gesehen.add(x["datei"])
        jahr = re.search(r"\b(18\d\d|190\d)\b", x["kategorie"] + " " + x["datum"])
        band = "Naturgeschichte der Vögel Mitteleuropas" if "Mitteleuropas" in x["kategorie"] or "Mitteleuropas" in x["kategorien"] \
            else "Naturgeschichte der Vögel Deutschlands"
        zeile = f"Naumann · {band}" + (f" · {jahr.group(1)}" if jahr else "")
        aus.append({"titel": art, "latein": art, "familie": "", "text": "", "zeile": zeile,
                    "quelle": x["url"], "seite": x["seite"]})
        if len(aus) >= anzahl:
            break
    return aus


# ---------------------------------------------------------------- Karten
def karten(kand, anzahl):
    blaeu, ortel, titel_gesehen = [], [], set()
    for x in kand:
        d = x["datei"]
        m = re.match(r"^(.+?) - Atlas Maior, vol (\d+), map (\d+) - Joan Blaeu, (\d{4})", d)
        if m:
            t = m.group(1).replace("…", "").strip(" .,")
            blaeu.append((int(m.group(2)), int(m.group(3)), t, int(m.group(4)), x))
            continue
        m = re.match(r"^(.+?) \(from Theatrum Orbis Terrarum, (\d{4})\), by Abraham Ortelius", d) or \
            re.match(r"^Theatrum Orbis Terrarum, by Abraham Ortelius, (.+?), (\d{4})", d)
        if m:
            if not re.search(r"\bmap\b", m.group(1), re.I):
                ortel.append((m.group(1), int(m.group(2)), x))
            continue
        if re.search(r"by Abraham Ortelius\.\w+$", d) and x["objekt"] and not re.search(r"\bmap\b", x["objekt"], re.I):
            j = re.search(r"\b(15\d\d|16[0-4]\d)\b", x["datum"] + " " + x["beschreibung"])
            ortel.append((x["objekt"], int(j.group(1)) if j else 1570, x))
    aus = []
    # Ortelius: je Titel die älteste Ausgabe
    for t, j, x in sorted(ortel, key=lambda e: (e[0], e[1])):
        k = re.sub(r"\W", "", t.lower())[:18]
        if k in titel_gesehen:
            continue
        titel_gesehen.add(k)
        aus.append({"titel": t, "zeile": f"Abraham Ortelius · Theatrum Orbis Terrarum · {j}", "jahr": j,
                    "quelle": x["url"], "seite": x["seite"]})
    hälfte = max(anzahl - min(len(aus), anzahl // 2), 0)
    aus = aus[: anzahl - hälfte]
    # Blaeu: gleichmäßig über alle Bände verteilt
    blaeu.sort(key=lambda e: e[:3])
    schritt = max(1, len(blaeu) / max(1, hälfte))
    i = 0.0
    while len(aus) < anzahl and int(i) < len(blaeu):
        band, nr, t, j, x = blaeu[int(i)]
        i += schritt
        k = re.sub(r"\W", "", t.lower())[:18]
        if k in titel_gesehen:
            continue
        titel_gesehen.add(k)
        aus.append({"titel": t, "zeile": f"Joan Blaeu · Atlas Maior, Band {band} · {j}", "jahr": j,
                    "quelle": x["url"], "seite": x["seite"]})
    return aus


def mischen(aus):
    """Tag 1..n vergeben, in fester, gemischter Reihenfolge (das erste Bild bleibt vorn)."""
    erg = aus[:1] + sorted(aus[1:], key=lambda e: hashlib.md5(e["quelle"].encode()).hexdigest())
    for t, e in enumerate(erg, 1):
        e["tag"] = t
    return erg


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kandidaten", required=True)
    ap.add_argument("--ziel", required=True)
    ap.add_argument("--anzahl", type=int, default=120)
    a = ap.parse_args()
    os.makedirs(a.ziel, exist_ok=True)
    for name, f in (("merian", merian), ("voegel", voegel), ("karten", karten)):
        p = os.path.join(a.kandidaten, f"kandidaten-{name}.json")
        if not os.path.exists(p):
            continue
        n = a.anzahl * 2 if name == "voegel" else a.anzahl   # Vogelnamen werden in texte.py geprüft und gekürzt
        aus = f(json.load(open(p, encoding="utf-8")), n)
        aus = aus if name == "voegel" else mischen(aus)
        with open(os.path.join(a.ziel, f"auswahl-{name}.json"), "w", encoding="utf-8") as fh:
            json.dump(aus, fh, ensure_ascii=False, indent=1)
        print(f"{name}: {len(aus)} Bilder")


if __name__ == "__main__":
    main()
