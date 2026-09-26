// Gemeinsamer Zustand der App
#pragma once
#include <time.h>
#include "air.h"
#include "radar.h"
#include "weather.h"

// Ansichten zum Durchwischen (bis SCR_COUNT) und Dialoge für die Ortswahl
enum Screen : uint8_t {
  SCR_HOME, SCR_HOURLY, SCR_DAILY, SCR_RADAR, SCR_AIR, SCR_DETAILS, SCR_COUNT,
  SCR_PLACES = SCR_COUNT,  // gespeicherte Orte
  SCR_SEARCH,              // Tastatur
  SCR_RESULTS              // Suchtreffer
};

constexpr int MAX_PLACES = 5;

struct AppState {
  Screen screen = SCR_HOME;
  WeatherData wx;
  AirData air;
  RadarView radar;
  struct tm now {};
  time_t epoch = 0;
  bool timeValid = false;
  bool offline = false;     // letzte Aktualisierung fehlgeschlagen
  uint8_t glitch = 0;       // Zufallswert für den 13:37-Effekt
  char city[48] = "";       // Anzeigename des Orts (aus der Einrichtung)
  float lat = 0, lon = 0;

  // Ortswahl
  Place places[MAX_PLACES];
  int nPlaces = 0;
  char query[40] = "";      // Eingabe auf der Tastatur (UTF-8)
  bool kbAlt = false;       // Tastatur zeigt Ziffern & Umlaute
  Place results[5];
  int nResults = 0;
  char searchMsg[64] = "";  // Hinweis unter dem Suchfeld
};

extern AppState app;
