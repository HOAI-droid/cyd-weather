# CYD-Wetter für TRMNL (LaraPaper)

Zwei Seiten für ein TRMNL bzw. das Seeed Studio TRMNL 7,5" (800 × 480, Schwarz-Weiß), gedacht für den eigenen Server [LaraPaper](https://github.com/usetrmnl/larapaper):

| Seite 1: Abfahrt, Wetter & Müll | Seite 2: Wetter ausführlich |
|---|---|
| ![](docs/seite1.png) | ![](docs/seite2.png) |

**Seite 1** (alle 5 Minuten neu)
- **links:** Abfahrten an deiner Haltestelle in eine Richtung. Die erste Fahrt, die du mit deinem Fußweg noch erreichst, steht oben im Kasten, mit „losgehen in … Min.“. Fahrten, die du nicht mehr schaffst oder die ausfallen, sind durchgestrichen. Verspätungen erscheinen als „+3“.
- **rechts:** das Wetter des Tages. Jetzt-Wert, Höchst- und Tiefstwert, Regen in den nächsten 2 Stunden, Temperaturkurve von 0 bis 24 Uhr mit Regenbalken (gestrichelter Strich = jetzt), dazu morgens, mittags, nachmittags und abends.
- **unten:** die nächsten 3 Mülltermine. Ab der eingestellten Uhrzeit am Vorabend (Standard 17:00) wird der Termin für morgen schwarz hervorgehoben: „heute Abend raus!“. Am Abholtag bleibt der Hinweis bis 10 Uhr stehen.

| Erinnerung am Vorabend | Am Abholtag morgens |
|---|---|
| ![](docs/seite1-erinnerung.png) | ![](docs/seite1-abholtag.png) |

**Seite 2** (alle 30 Minuten neu): ausführliches Wetter wie auf der CYD-Wetterstation. Enthält die nächsten 24 Stunden, 5 Tage, Luftqualität mit Pollen sowie Sonne und Wind.

Beide Seiten gibt es auch in den kleineren Größen für Mashups (halb waagrecht, halb senkrecht, Viertel), siehe [docs/](docs/).

*Die Bilder sind mit Beispieldaten erzeugt. Linien und Zeiten sind ausgedacht.*

## Einrichten in LaraPaper

1. **ZIP-Dateien holen:** unter [Releases](../../releases) im Release „TRMNL-Plugins“ die Dateien `trmnl-abfahrt.zip` und `trmnl-wetter.zip` herunterladen. Für Testversionen aus einem Branch gibt es eigene Vorab-Releases „TRMNL-Plugins (Branch)“. Alternativ selbst bauen mit `trmnl/build.sh`.
2. **Importieren:** in LaraPaper **Plugins → Import Recipe Archive**, ZIP auswählen, **Import**. Das für beide Dateien wiederholen.
3. **Einstellen:** beim Plugin auf **Configure** klicken. Die Felder sind schon für Darmstadt, „Im Fiedlersee“ und Richtung Willy-Brandt-Platz vorbelegt (siehe unten).
4. **Playlist:** beide Plugins der Playlist deines Geräts hinzufügen. Soll Seite 1 meistens zu sehen sein, Seite 2 seltener oder nur zu bestimmten Zeiten einplanen.
5. **Aktualisierung am Gerät:** Damit die Abfahrten stimmen, muss das Gerät selbst alle 5 Minuten aufwachen. Das stellst du in LaraPaper beim **Gerät** über das Aktualisierungsintervall (Refresh Rate) auf 300 Sekunden ein. Der Akku hält dann deutlich kürzer.

**Updates:** neue ZIP herunterladen und genauso importieren. Die Plugins haben eine feste Kennung (`cyd-abfahrt`, `cyd-wetter`). LaraPaper ersetzt deshalb das vorhandene Plugin und behält deine Einstellungen.

## Einstellungen

| Feld | Bedeutung |
|---|---|
| Haltestelle | Name wie in der RMV-Auskunft. Prüfen: [dbf.finalrewind.org/?hafas=RMV](https://dbf.finalrewind.org/?hafas=RMV), Haltestelle suchen und den Namen genau übernehmen. Standard: `Im Fiedlersee, Darmstadt` |
| Richtung (über) | Nur Fahrten, die über diese Haltestelle fahren. Standard: `Willy-Brandt-Platz`. Leer = alle. Mehrere mit Komma trennen. |
| Verkehrsverbund | RMV für Darmstadt. Weitere zur Auswahl. |
| Fußweg | Minuten bis zur Haltestelle. Davon hängen „losgehen in …“ und das Durchstreichen ab. |
| Ortsname, Breiten- und Längengrad | Für das Wetter. Nachschlagen auf [open-meteo.com](https://open-meteo.com/en/docs) (oben Ort suchen) oder [latlong.net](https://www.latlong.net/) (auf die Karte klicken). Dezimalgrad mit Punkt, z. B. `49.87`. |
| Straße für den Müllkalender | Straßenname wie im [EAD-Abfallkalender](https://www.ead.darmstadt.de/unser-angebot/privathaushalte/abfallkalender), **ohne Hausnummer**. Bei Straßen, die der EAD aufteilt, den Abschnitt mitschreiben, z. B. `Heinheimer Straße 1-15, 2-16`. Leer = keine Müll-Leiste. |
| Müll-Erinnerung ab | Uhrzeit am Vorabend, ab der der Termin hervorgehoben wird (15–20 Uhr). |
| Wetterspruch | Ein Spruch pro Tag wie auf der CYD. Auf Seite 1 und 2 abschaltbar. |

## Datenquellen (alle ohne API-Schlüssel)

| Was | Quelle |
|---|---|
| Wetter, Luftqualität, Pollen | [Open-Meteo](https://open-meteo.com) |
| Abfahrten | [dbf.finalrewind.org](https://dbf.finalrewind.org) (DB-Infoscreen) mit der RMV-Fahrplanauskunft (HAFAS) |
| Müll | EAD Darmstadt, Abfallkalender nach Straße (dieselbe Schnittstelle nutzt [Waste Collection Schedule](https://github.com/mampfes/hacs_waste_collection_schedule)) |

Müllarten: `RM1/RM2/RM4` = Restmüll, `WET` = Gelbe Tonne, `BIO` = Bio, `PPK` = Papier. Unbekannte Kürzel erscheinen unverändert.

## Wenn etwas nicht passt

| Anzeige | Lösung |
|---|---|
| „Haltestelle mehrdeutig“ oder „Keine Abfahrten“ | Namen auf dbf.finalrewind.org prüfen (siehe oben). Vielleicht heißt die Haltestelle beim RMV z. B. `Darmstadt Im Fiedlersee`. |
| Keine Fahrten mit Richtung | Schreibweise der Richtungs-Haltestelle prüfen oder das Feld testweise leeren |
| „Straße im EAD-Kalender nicht gefunden“ | Straßenname wie im EAD-Abfallkalender schreiben, ggf. mit Abschnitt |
| Uhrzeit falsch | Die Uhrzeit kommt aus der Zeitzone der Wetterdaten. Breiten- und Längengrad prüfen. |

## Für Entwickler

```
trmnl/
  common/basis.liquid     Schrift, Stil, Symbole, Uhrzeit/Datum, Bausteine cw_icon und cw_text
  common/spruch.liquid    Wetterspruch
  plugins/abfahrt/        Seite 1: settings.yml, logik.liquid, full/half_*/quadrant.liquid
  plugins/wetter/         Seite 2: settings.yml, logik.liquid, full.liquid, kompakt.liquid (für alle kleinen Größen)
  build.sh                baut dist/trmnl-*.zip (shared.liquid = basis + logik + spruch)
  vorschau/vorschau.sh    rendert eine Seite mit Beispieldaten als PNG (gleiche Liquid-Engine wie LaraPaper)
```

- Uhrzeit und Datum werden ohne Datumsfilter berechnet: aus `trmnl.system.timestamp_utc` und dem UTC-Versatz von Open-Meteo. Das funktioniert unabhängig von der Zeitzone des LaraPaper-Servers.
- Alles ist nur Schwarz/Weiß ohne Graustufen gestaltet und mit eigenem Stil, unabhängig vom TRMNL-Framework.
- Eigenheiten der Liquid-Engine in LaraPaper: `slice` mit negativem Start liefert bei kurzen Texten nichts, und `"" | split` ergibt einen Eintrag. Deshalb wird beides hier vermieden.
