#!/usr/bin/env python3
"""
Sammler für die "Tafel der Luftfahrzeuge" (TRMNL/LaraPaper).

Fragt alle paar Minuten ab, welche Flugzeuge im Umkreis um einen Ort fliegen
(ADS-B-Daten von adsb.lol oder vom eigenen Empfänger), merkt sich die
Überflüge der letzten 24 Stunden und schickt die Auswertung an den Webhook des
LaraPaper-Plugins. Nur Python 3 Standardbibliothek, keine Pakete nötig.

Einmal ausführen (für cron, z. B. alle 5 Minuten):
    python3 sammler.py --webhook https://DEIN-LARAPAPER/api/custom_plugins/UUID

Dauerbetrieb (z. B. im Docker-Container):
    python3 sammler.py --webhook ... --schleife 300

Optionen siehe: python3 sammler.py --help
"""
import argparse
import json
import math
import os
import sys
import time
import urllib.request
from datetime import datetime

try:
    from zoneinfo import ZoneInfo
except ImportError:  # Python < 3.9
    ZoneInfo = None

# ICAO-Typ -> (Anzeigename, Zeichnung, Kategorie). Kategorie: jet = Verkehr/Fracht, ga = allgemeine Luftfahrt, heli
TYPEN = {
    "A318": ("Airbus A318", "A20N", "jet"), "A319": ("Airbus A319", "A20N", "jet"), "A320": ("Airbus A320", "A20N", "jet"),
    "A19N": ("Airbus A319neo", "A20N", "jet"), "A20N": ("Airbus A320neo", "A20N", "jet"),
    "A321": ("Airbus A321", "A21N", "jet"), "A21N": ("Airbus A321neo", "A21N", "jet"),
    "BCS1": ("Airbus A220-100", "BCS3", "jet"), "BCS3": ("Airbus A220-300", "BCS3", "jet"),
    "A332": ("Airbus A330-200", "A333", "jet"), "A333": ("Airbus A330-300", "A333", "jet"),
    "A338": ("Airbus A330-800neo", "A333", "jet"), "A339": ("Airbus A330-900neo", "A333", "jet"),
    "A359": ("Airbus A350-900", "A359", "jet"), "A35K": ("Airbus A350-1000", "A359", "jet"),
    "A388": ("Airbus A380-800", "A388", "jet"),
    "B736": ("Boeing 737-600", "B738", "jet"), "B737": ("Boeing 737-700", "B738", "jet"), "B738": ("Boeing 737-800", "B738", "jet"),
    "B739": ("Boeing 737-900", "B738", "jet"), "B37M": ("Boeing 737 MAX 7", "B738", "jet"), "B38M": ("Boeing 737 MAX 8", "B738", "jet"),
    "B39M": ("Boeing 737 MAX 9", "B738", "jet"), "B3XM": ("Boeing 737 MAX 10", "B738", "jet"),
    "B744": ("Boeing 747-400", "B748", "jet"), "B748": ("Boeing 747-8", "B748", "jet"),
    "B772": ("Boeing 777-200", "B77L", "jet"), "B77L": ("Boeing 777-200LR/F", "B77L", "jet"), "B773": ("Boeing 777-300", "B77L", "jet"),
    "B77W": ("Boeing 777-300ER", "B77L", "jet"), "B778": ("Boeing 777-8", "B77L", "jet"), "B779": ("Boeing 777-9", "B77L", "jet"),
    "B788": ("Boeing 787-8", "B789", "jet"), "B789": ("Boeing 787-9", "B789", "jet"), "B78X": ("Boeing 787-10", "B789", "jet"),
    "E170": ("Embraer 170", "E190", "jet"), "E75L": ("Embraer 175", "E190", "jet"), "E75S": ("Embraer 175", "E190", "jet"),
    "E190": ("Embraer 190", "E190", "jet"), "E195": ("Embraer 195", "E190", "jet"), "E290": ("Embraer E190-E2", "E190", "jet"),
    "E295": ("Embraer E195-E2", "E190", "jet"),
    "B752": ("Boeing 757-200", None, "jet"), "B753": ("Boeing 757-300", None, "jet"), "B762": ("Boeing 767-200", None, "jet"),
    "B763": ("Boeing 767-300", None, "jet"), "B764": ("Boeing 767-400", None, "jet"), "MD11": ("McDonnell Douglas MD-11", None, "jet"),
    "A306": ("Airbus A300-600", None, "jet"), "A310": ("Airbus A310", None, "jet"), "A343": ("Airbus A340-300", None, "jet"),
    "A346": ("Airbus A340-600", None, "jet"), "CRJ7": ("Bombardier CRJ700", None, "jet"), "CRJ9": ("Bombardier CRJ900", None, "jet"),
    "CRJX": ("Bombardier CRJ1000", None, "jet"), "AT75": ("ATR 72-500", None, "jet"), "AT76": ("ATR 72-600", None, "jet"),
    "AT45": ("ATR 42-500", None, "jet"), "AT46": ("ATR 42-600", None, "jet"), "DH8D": ("De Havilland Dash 8-400", None, "jet"),
    "A400": ("Airbus A400M", None, "jet"), "C130": ("Lockheed C-130 Hercules", None, "jet"), "C17": ("Boeing C-17 Globemaster", None, "jet"),
    "A124": ("Antonow An-124", None, "jet"),
    "C750": ("Cessna Citation X", "C750", "ga"), "C68A": ("Cessna Citation Latitude", "C750", "ga"), "C680": ("Cessna Citation Sovereign", "C750", "ga"),
    "C56X": ("Cessna Citation Excel", "C750", "ga"), "C700": ("Cessna Citation Longitude", "C750", "ga"),
    "CL35": ("Bombardier Challenger 350", None, "ga"), "GLF6": ("Gulfstream G650", None, "ga"), "GLEX": ("Bombardier Global Express", None, "ga"),
    "F2TH": ("Dassault Falcon 2000", None, "ga"), "FA7X": ("Dassault Falcon 7X", None, "ga"), "PC24": ("Pilatus PC-24", None, "ga"),
    "PC12": ("Pilatus PC-12", "PC12", "ga"), "DA42": ("Diamond DA42", "DA42", "ga"), "DA62": ("Diamond DA62", "DA42", "ga"),
    "DA40": ("Diamond DA40", None, "ga"), "DA20": ("Diamond DA20", None, "ga"),
    "C150": ("Cessna 150", "C172", "ga"), "C152": ("Cessna 152", "C172", "ga"), "C172": ("Cessna 172", "C172", "ga"),
    "C182": ("Cessna 182", "C172", "ga"), "C206": ("Cessna 206", "C172", "ga"), "C208": ("Cessna 208 Caravan", None, "ga"),
    "P28A": ("Piper PA-28", None, "ga"), "P28R": ("Piper PA-28R Arrow", None, "ga"), "SR22": ("Cirrus SR22", None, "ga"),
    "SR20": ("Cirrus SR20", None, "ga"), "DR40": ("Robin DR400", None, "ga"), "BE20": ("Beechcraft King Air 200", None, "ga"),
    "EC45": ("Airbus H145", "EC45", "heli"), "H145": ("Airbus H145", "EC45", "heli"), "EC35": ("Airbus H135", "EC45", "heli"),
    "EC30": ("Airbus H130", "EC45", "heli"), "EC55": ("Airbus H155", "EC45", "heli"), "H160": ("Airbus H160", "EC45", "heli"),
    "R22": ("Robinson R22", "R44", "heli"), "R44": ("Robinson R44", "R44", "heli"), "R66": ("Robinson R66", "R44", "heli"),
    "A139": ("Leonardo AW139", None, "heli"), "A169": ("Leonardo AW169", None, "heli"), "NH90": ("NH Industries NH90", None, "heli"),
}
# Breite der Zeichnungen in Pixeln (aus der Tafel), damit alle im selben Maßstab erscheinen
BILDBREITE = {"A20N": 372, "A21N": 439, "A333": 617, "A359": 655, "A388": 713, "B738": 390, "B748": 747, "B77L": 630,
              "B789": 629, "BCS3": 378, "C172": 282, "C750": 736, "DA42": 294, "E190": 356, "EC45": 426, "PC12": 487, "R44": 400}

PAUSE_NEUER_UEBERFLUG = 30 * 60  # ein Flugzeug, das 30 Minuten weg war, zählt beim Wiederkommen neu
FENSTER = 24 * 3600


def entfernung_km(lat1, lon1, lat2, lon2):
    r = 6371.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = p2 - p1, math.radians(lon2 - lon1)
    a = math.sin(dp / 2) ** 2 + math.cos(p1) * math.cos(p2) * math.sin(dl / 2) ** 2
    return 2 * r * math.asin(math.sqrt(a))


def hole_json(url, timeout=25):
    req = urllib.request.Request(url, headers={"User-Agent": "cyd-weather-trmnl-sammler/1.0", "Accept": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read().decode("utf-8"))


def kategorie(ac, t):
    if t in TYPEN:
        return TYPEN[t][2]
    c = (ac.get("category") or "").upper()
    if c == "A7":
        return "heli"
    if c in ("A1", "A2", "B1", "B4"):
        return "ga"
    return "jet"


def main():
    ap = argparse.ArgumentParser(description="Sammelt Überflüge der letzten 24 Stunden und schickt sie an LaraPaper.")
    ap.add_argument("--webhook", default=os.environ.get("WEBHOOK_URL"), help="Webhook-Adresse des LaraPaper-Plugins")
    ap.add_argument("--lat", type=float, default=float(os.environ.get("LAT", "49.9016")))
    ap.add_argument("--lon", type=float, default=float(os.environ.get("LON", "8.6555")))
    ap.add_argument("--radius", type=float, default=float(os.environ.get("RADIUS_KM", "20")), help="Umkreis in km")
    ap.add_argument("--ort", default=os.environ.get("ORT", "Arheilgen"), help="Ortsname für die Überschrift")
    ap.add_argument("--quelle", default=os.environ.get("QUELLE", "https://api.adsb.lol/v2/point/{lat}/{lon}/{nm}"),
                    help="Datenquelle; eigener Empfänger z. B. http://raspberrypi.local/tar1090/data/aircraft.json")
    ap.add_argument("--zustand", default=os.environ.get("ZUSTAND", os.path.join(os.path.dirname(os.path.abspath(__file__)), "flugzeuge-zustand.json")))
    ap.add_argument("--zeitzone", default=os.environ.get("ZEITZONE", "Europe/Berlin"))
    ap.add_argument("--schleife", type=int, default=int(os.environ.get("SCHLEIFE", "0")), help="Sekunden zwischen Abfragen; 0 = einmal")
    ap.add_argument("--trocken", action="store_true", help="nichts senden, Ergebnis nur ausgeben")
    a = ap.parse_args()
    if not a.webhook and not a.trocken:
        ap.error("--webhook fehlt (oder Umgebungsvariable WEBHOOK_URL)")
    while True:
        try:
            lauf(a)
        except Exception as e:  # weiterlaufen, beim nächsten Mal klappt es vielleicht
            print(f"{datetime.now():%Y-%m-%d %H:%M} Fehler: {e}", file=sys.stderr)
            if not a.schleife:
                sys.exit(1)
        if not a.schleife:
            break
        time.sleep(a.schleife)


def lauf(a):
    jetzt = int(time.time())
    try:
        with open(a.zustand, encoding="utf-8") as f:
            z = json.load(f)
    except (OSError, ValueError):
        z = {}
    flieger = z.setdefault("flieger", {})
    bekannt = z.setdefault("bekannte_typen", {})
    start = z.setdefault("start", jetzt)

    url = a.quelle.format(lat=a.lat, lon=a.lon, nm=round(a.radius / 1.852, 1))
    daten = hole_json(url)
    liste = daten.get("ac") or daten.get("aircraft") or []
    for ac in liste:
        hexid = (ac.get("hex") or "").lower().strip("~")
        alt = ac.get("alt_baro", ac.get("alt_geom"))
        lat, lon = ac.get("lat"), ac.get("lon")
        if not hexid or lat is None or lon is None or not isinstance(alt, (int, float)):
            continue  # ohne Position oder am Boden
        if entfernung_km(a.lat, a.lon, lat, lon) > a.radius:
            continue
        t = (ac.get("t") or "").upper() or "????"
        hoehe = round(alt * 0.3048)
        e = flieger.setdefault(hexid, {"t": t, "u": []})
        e["t"] = t if t != "????" else e.get("t", t)
        e["k"] = kategorie(ac, e["t"])
        if ac.get("desc"):
            e["d"] = ac["desc"]
        if not e["u"] or jetzt - e["u"][-1][1] > PAUSE_NEUER_UEBERFLUG:
            e["u"].append([jetzt, jetzt, hoehe])  # Beginn, zuletzt gesehen, niedrigste Höhe
        else:
            u = e["u"][-1]
            u[1] = jetzt
            u[2] = min(u[2], hoehe)
        bekannt.setdefault(e["t"], jetzt)

    # Alles älter als 24 Stunden vergessen
    for hexid in list(flieger):
        flieger[hexid]["u"] = [u for u in flieger[hexid]["u"] if jetzt - u[1] <= FENSTER]
        if not flieger[hexid]["u"]:
            del flieger[hexid]

    # Auswerten
    typen = {}
    for e in flieger.values():
        t = e["t"]
        g = typen.setdefault(t, {"t": t, "n": 0, "hoehen": [], "k": e.get("k", "jet"), "d": e.get("d")})
        g["n"] += len(e["u"])
        g["hoehen"] += [u[2] for u in e["u"]]
    ausgabe = []
    for t, g in typen.items():
        name, bild, k = TYPEN.get(t, (None, None, g["k"]))
        if not name:
            name = "Unbekannter Typ" if t == "????" else (g.get("d") or t)
        ausgabe.append({
            "t": t, "name": name, "bild": bild or "", "bw": BILDBREITE.get(bild, 0), "k": k, "n": g["n"],
            "alt": int(round(sum(g["hoehen"]) / len(g["hoehen"]) / 50.0) * 50) if g["hoehen"] else 0,
            # erstmals gesehen: Typ ist in den letzten 24 Stunden neu dazugekommen (erst nach dem ersten Tag)
            "neu": 1 if jetzt - start > FENSTER and jetzt - bekannt.get(t, 0) <= FENSTER else 0,
        })
    ausgabe.sort(key=lambda x: (-x["n"], x["name"]))
    if ZoneInfo:
        lokal = datetime.fromtimestamp(jetzt, ZoneInfo(a.zeitzone))
    else:
        lokal = datetime.fromtimestamp(jetzt)
    ergebnis = {
        "ort": a.ort, "radius_km": a.radius, "stand": jetzt, "stand_text": lokal.strftime("%H:%M"),
        "datum_text": lokal.strftime("%d.%m.%Y"), "ueberfluege": sum(x["n"] for x in ausgabe),
        "flugzeuge": len(flieger), "anzahl_typen": len(ausgabe), "typen": ausgabe[:40],
    }

    tmp = a.zustand + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(z, f, separators=(",", ":"))
    os.replace(tmp, a.zustand)

    if a.trocken:
        print(json.dumps(ergebnis, ensure_ascii=False, indent=1))
        return
    body = json.dumps({"merge_variables": ergebnis}).encode("utf-8")
    req = urllib.request.Request(a.webhook, data=body, method="POST",
                                 headers={"Content-Type": "application/json", "Accept": "application/json"})
    with urllib.request.urlopen(req, timeout=25) as r:
        r.read()
    print(f"{lokal:%Y-%m-%d %H:%M} gesendet: {ergebnis['ueberfluege']} Überflüge, {ergebnis['anzahl_typen']} Typen")


if __name__ == "__main__":
    main()
