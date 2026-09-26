// Gemeinsamer Zustand der App
#pragma once
#include <time.h>
#include "weather.h"

enum Screen : uint8_t { SCR_HOME, SCR_HOURLY, SCR_DAILY, SCR_DETAILS, SCR_COUNT };

struct AppState {
  Screen screen = SCR_HOME;
  WeatherData wx;
  struct tm now {};
  time_t epoch = 0;
  bool timeValid = false;
  bool offline = false;     // letzte Aktualisierung fehlgeschlagen
  uint8_t glitch = 0;       // Zufallswert für den 13:37-Effekt
  char city[48] = "";       // Anzeigename des Orts (aus der Einrichtung)
  float lat = 0, lon = 0;
};

extern AppState app;
