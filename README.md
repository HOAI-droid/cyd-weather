# CYD Wetterstation

Moderne, übersichtliche Wetterstation für das **Cheap Yellow Display** (ESP32-2432S028R, 2,8" 320×240 Touch) – mit kantengeglätteten Schriften, gezeichneten Wettersymbolen, Wischgesten und einem Haufen versteckter Späße.

![Jetzt](docs/screens/partly_home.png) ![24 Stunden](docs/screens/partly_hourly.png)
![5 Tage](docs/screens/partly_daily.png) ![Sonne & Wind](docs/screens/partly_details.png)

*Die Bilder stammen direkt aus dem Firmware-Code (Host-Simulator, siehe unten).*

## Funktionen

- **4 Ansichten:** Jetzt · Nächste 24 Stunden · 5-Tage-Vorhersage · Sonne & Wind
- Wetterdaten von [Open-Meteo](https://open-meteo.com) – kostenlos, **kein API-Schlüssel**, Aktualisierung alle 10 Minuten
- Hintergrund passt sich der Wetterlage und Tageszeit an (Sonne, Wolken, Regen, Gewitter, Schnee, Nebel, Nacht)
- Uhrzeit per NTP inkl. Sommerzeit, deutsches Datum
- Nachts automatisch gedimmt (beim Antippen kurz hell)
- Nach 60 s ohne Berührung zurück zur Hauptansicht
- Flimmerfreies Zeichnen über einen Streifen-Puffer (nur 38 KB RAM)
- Umlaute und `°` in allen Schriften (Outfit, SIL Open Font License)

| Sonnig | Regen | Gewitter | Schnee | Nacht |
|---|---|---|---|---|
| ![](docs/screens/sunny_home.png) | ![](docs/screens/rain_home.png) | ![](docs/screens/storm_home.png) | ![](docs/screens/snow_home.png) | ![](docs/screens/night_home.png) |

## Installation

1. [VS Code](https://code.visualstudio.com/) + Erweiterung **PlatformIO IDE** installieren.
2. Dieses Repository öffnen.
3. `include/config.h` anpassen: WLAN, Stadtname, Koordinaten (Rechtsklick in Google Maps zeigt sie an).
4. CYD per USB anschließen, unten in der PlatformIO-Leiste die Umgebung wählen und **Upload** klicken:
   - `env:cyd` – das klassische CYD mit **einem** Micro-USB-Port (ILI9341)
   - `env:cyd2usb` – die Version mit **USB-C und Micro-USB** (ST7789)

Alternativ auf der Kommandozeile: `pio run -e cyd -t upload && pio device monitor`

Alle Bibliotheken (TFT_eSPI, XPT2046_Touchscreen, ArduinoJson) lädt PlatformIO automatisch. Die Display-Konfiguration steckt komplett in `platformio.ini`, du musst also keine `User_Setup.h` bearbeiten.

## Bedienung

| Geste | Wirkung |
|---|---|
| Nach links/rechts wischen | nächste / vorige Ansicht |
| Linkes / rechtes Drittel antippen | vorige / nächste Ansicht |
| Punkte unten | zeigen, wo du bist |

## Easter Eggs

<details>
<summary><b>Spoiler! Erst selbst suchen?</b></summary>

| | Auslöser | Was passiert |
|---|---|---|
| 🐸 **Wetterfrosch** | 5× schnell aufs Wettersymbol tippen | Der Frosch klettert die Leiter hoch – je besser das Wetter, desto höher – und gibt eine Bauernregel zum Besten |
| 💧 **Tropfenfänger** | Uhrzeit 2 s gedrückt halten | Minispiel: Regentropfen mit dem Eimer fangen (Finger steuert), Sonnen = +5, Blitze kosten ein Leben. Highscore bleibt gespeichert |
| 🟩 **Matrix-Wetter** | Ecken antippen: oben links → oben rechts → unten rechts → unten links | Grüner Zeichenregen mit „Wake up, Neo …“ |
| 🕜 **Leet Time** | um 13:37 Uhr | Die Uhr leuchtet grün und flackert |
| 🍬 **Unwetterwarnung** | am 1. April | „Ab 14 Uhr regnet es Gummibärchen.“ … April, April! |
| 🎅 **Feiertage** | 24.–26.12. / 31.10. / Neujahr 0:00 | Weihnachtsmütze aufs Wettersymbol, Kürbis neben der Stadt, Feuerwerk |
| 🎂 **Geburtstag** | `BIRTHDAY_*` in `config.h` setzen | Konfetti, Glückwunsch und „Happy Birthday“ aus dem Lautsprecher |
| 😄 **Flachwitz** | BOOT-Taste kurz drücken | Zufälliger (Wetter-)Flachwitz, nochmal drücken = nächster |
| 🪩 **Disco-Wetter** | BOOT-Taste 1 s halten | RGB-LED blinkt bunt, das Display pulsiert, der Lautsprecher dudelt |
| 👀 **Licht aus** | Lichtsensor vorne abdecken | „Huch! Wer hat das Licht ausgemacht?“ |
| 💬 **Sprüche** | immer | Unter der Wetterlage steht ein passender Spruch |

![](docs/screens/egg_frog.png) ![](docs/screens/egg_game.png) ![](docs/screens/egg_matrix.png)
![](docs/screens/egg_april.png) ![](docs/screens/egg_joke.png) ![](docs/screens/egg_disco.png)
![](docs/screens/egg_dark.png) ![](docs/screens/egg_newyear.png) ![](docs/screens/egg_leet.png)

</details>

## Fehlersuche

| Problem | Lösung |
|---|---|
| Farben falsch / invertiert | andere Umgebung (`cyd` ↔ `cyd2usb`) probieren oder in `platformio.ini` `-DTFT_INVERSION_ON=1` bzw. `OFF` tauschen |
| Bild gespiegelt oder weiß | Umgebung `cyd2usb` testen; bei manchen Boards hilft `-DSPI_FREQUENCY=40000000` |
| Tipps landen daneben | `TOUCH_DEBUG true` setzen, Rohwerte im seriellen Monitor ablesen und `TOUCH_X/Y_MIN/MAX` anpassen; ggf. `TOUCH_FLIP_X/Y` |
| „WLAN nicht erreichbar“ | SSID/Passwort prüfen – der ESP32 kann nur 2,4 GHz |
| Lautsprecher zu nervig | `SOUND_ENABLED false` |
| „Licht aus“ löst zu oft / nie aus | `LDR_DARK_DELTA` anpassen oder `LDR_EGG_ENABLED false` |

## Projektstruktur

```
include/config.h      Deine Einstellungen
src/main.cpp          WLAN, Zeit, Wetterabruf, Navigation, Dimmen
src/ui.cpp            Die vier Ansichten + Startbildschirm
src/icons.cpp         Gezeichnete Wettersymbole
src/gfx.cpp           Streifen-Rendering, kantengeglättete Formen, Text
src/weather.cpp       Open-Meteo-Parser, WMO-Codes → deutsche Texte, Sprüche
src/eggs.cpp          Alle Easter Eggs
src/input.cpp         Touch-Gesten und BOOT-Taste
src/hw.cpp            RGB-LED, Lautsprecher, Lichtsensor, Speicher
src/fonts/            Erzeugte Schriften (tools/make_fonts.py)
sim/                  Host-Simulator für Screenshots
```

## Schriften neu erzeugen

```
pip install pillow
python3 tools/make_fonts.py
```

Größen, Gewichte und Zeichensatz stehen oben im Skript.

## Simulator

`sim/run.sh` kompiliert den Zeichen-Code der Firmware (UI, Symbole, Easter Eggs, JSON-Parser) für den PC gegen eine kleine TFT_eSPI-Nachbildung und schreibt Screenshots nach `sim/build/`. Voraussetzung: `g++`, Python mit Pillow und die [ArduinoJson](https://github.com/bblanchon/ArduinoJson)-Quellen (Pfad per `ARDUINOJSON=.../src`).

## Lizenz

Code: MIT. Schrift Outfit: SIL Open Font License (`tools/Outfit-OFL.txt`).
