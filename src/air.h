// Luftqualität und Pollen von Open-Meteo (kostenlos, kein API-Schlüssel)
#pragma once
#include <stdint.h>

enum Pollen : uint8_t { P_ALDER, P_BIRCH, P_GRASS, P_MUGWORT, P_OLIVE, P_RAGWEED, P_COUNT };

struct AirData {
  bool valid = false;
  uint32_t fetchedMs = 0;
  float aqi = 0;                         // Europäischer Luftqualitätsindex
  float pm25 = 0, pm10 = 0, no2 = 0, o3 = 0;  // µg/m³
  float pollen[P_COUNT] = {};            // Pollenkörner/m³
  bool hasPollen = false;                // nur in Europa verfügbar
};

// Pfad + Query (Host: air-quality-api.open-meteo.com)
void buildAirUrl(char* buf, int len, float lat, float lon);
bool parseAir(const char* json, AirData& out);

const char* aqiText(float aqi);   // "Gut", "Mäßig", …
uint16_t aqiColor(float aqi);
// Kurzfassung der stärksten Pollen, z. B. "Gräser mäßig · Beifuß gering"
void pollenSummary(const AirData& a, char* buf, int len);
