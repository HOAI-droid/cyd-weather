// ============================================================================
//  CYD Wetterstation – Einstellungen
//  Nur diese Datei musst du anpassen.
// ============================================================================
#pragma once

// ---- WLAN & Ort ----------------------------------------------------------------
// WLAN, Passwort und Stadt gibst du NICHT hier ein, sondern beim ersten Start
// über dein Handy: Das CYD öffnet dann ein eigenes WLAN mit diesem Namen.
// Später kommst du wieder in die Einrichtung, indem du oben links auf den
// Stadtnamen 2 Sekunden drückst.
#define PORTAL_AP_NAME     "CYD-Wetter"
#define PORTAL_AP_PASSWORD ""          // leer = offenes Einrichtungs-WLAN

// ---- Zeitzone (POSIX-Format) --------------------------------------------------
// Deutschland/Österreich/Schweiz: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO         "CET-1CEST,M3.5.0,M10.5.0/3"
#define NTP_SERVER      "de.pool.ntp.org"

// ---- Aktualisierung -----------------------------------------------------------
#define WEATHER_INTERVAL_MIN   10     // Wetter alle x Minuten abrufen
#define HOME_TIMEOUT_S         60     // nach x Sekunden ohne Touch zurück zu "Jetzt"

// ---- Helligkeit ---------------------------------------------------------------
#define BRIGHTNESS_DAY    255         // 0..255
#define BRIGHTNESS_NIGHT   40
#define NIGHT_START_HOUR   22         // ab dieser Stunde gedimmt
#define NIGHT_END_HOUR      6         // bis zu dieser Stunde gedimmt

// ---- Extras -------------------------------------------------------------------
#define SOUND_ENABLED      true       // Lautsprecher für Melodien (Geburtstag, Disco, Spiel)
#define LDR_EGG_ENABLED    true       // "Licht aus"-Easter-Egg über den Lichtsensor
#define LDR_DARK_DELTA     250        // wie stark der Sensor sich ändern muss (ADC-Schritte)

// Geburtstag für das Geburtstags-Ei (Monat, Tag). 0 = aus.
#define BIRTHDAY_MONTH     0
#define BIRTHDAY_DAY       0
#define BIRTHDAY_NAME      ""         // z. B. "Sven" -> "Alles Gute, Sven!"

// ---- Touch-Kalibrierung (XPT2046 Rohwerte) --------------------------------------
// Falls Tipps nicht dort landen, wo du hinfasst: Werte anpassen oder die
// Rohwerte im seriellen Monitor ablesen (TOUCH_DEBUG auf true).
#define TOUCH_X_MIN   200
#define TOUCH_X_MAX  3700
#define TOUCH_Y_MIN   240
#define TOUCH_Y_MAX  3800
#define TOUCH_FLIP_X  false
#define TOUCH_FLIP_Y  false
#define TOUCH_DEBUG   false
