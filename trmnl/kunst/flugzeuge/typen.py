#!/usr/bin/env python3
"""Typ-Tabelle für die Tafel der Luftfahrzeuge. Erzeugt daraus plugins/flugzeuge/zusatz-typen.liquid:
    python3 trmnl/kunst/flugzeuge/typen.py
"""
import os

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



def main():
    ziel = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "plugins", "flugzeuge", "zusatz-typen.liquid")
    z = ["{%- comment -%}", '  ICAO-Flugzeugtyp -> "Name^Zeichnung^Kategorie^Bildbreite". Aufruf:',
         '  {% capture info %}{% render "fl_typ", t: "A20N" %}{% endcapture %}',
         "  Erzeugt aus kunst/flugzeuge/typen.py, nicht von Hand ändern.", "{%- endcomment -%}", "{% template fl_typ %}{%- case t -%}"]
    for t, (name, bild, k) in sorted(TYPEN.items()):
        z.append(f'{{%- when "{t}" -%}}{name}^{bild or ""}^{k}^{BILDBREITE.get(bild, 0)}')
    z.append("{%- else -%}^^^0{%- endcase -%}{% endtemplate %}")
    with open(ziel, "w", encoding="utf-8") as f:
        f.write("\n".join(z) + "\n")
    print(ziel)


if __name__ == "__main__":
    main()
