// Netzwerkzugriffe (nur auf dem ESP32): Wetter, Luft, Ortssuche, Radar
#pragma once
#include <Arduino.h>
#include "air.h"
#include "radar.h"
#include "weather.h"

// HTTPS-GET, Antwort als Text
bool netGet(const char* host, const char* path, String& body);

bool netFetchAir(AirData& out, float lat, float lon);
// Ortssuche: Anzahl Treffer, -1 bei Verbindungsfehler
int netSearchPlaces(const char* query, Place* out, int maxN);
// Lädt Karte (falls nötig) und neuestes Radarbild für lat/lon
bool netFetchRadar(RadarView& v, float lat, float lon);
