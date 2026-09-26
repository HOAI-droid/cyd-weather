#include "air.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include "theme.h"

static const char* const POLLEN_KEYS[P_COUNT] = {"alder_pollen", "birch_pollen", "grass_pollen",
                                                 "mugwort_pollen", "olive_pollen", "ragweed_pollen"};
static const char* const POLLEN_NAMES[P_COUNT] = {"Erle", "Birke", "Gräser", "Beifuß", "Olive", "Ambrosia"};

void buildAirUrl(char* buf, int len, float lat, float lon) {
  snprintf(buf, len,
           "/v1/air-quality?latitude=%.4f&longitude=%.4f"
           "&current=european_aqi,pm2_5,pm10,nitrogen_dioxide,ozone,"
           "alder_pollen,birch_pollen,grass_pollen,mugwort_pollen,olive_pollen,ragweed_pollen"
           "&timezone=auto&timeformat=unixtime",
           lat, lon);
}

bool parseAir(const char* json, AirData& a) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  JsonObject c = doc["current"];
  if (c.isNull() || c["european_aqi"].isNull()) return false;
  a.aqi = c["european_aqi"] | 0.0f;
  a.pm25 = c["pm2_5"] | 0.0f;
  a.pm10 = c["pm10"] | 0.0f;
  a.no2 = c["nitrogen_dioxide"] | 0.0f;
  a.o3 = c["ozone"] | 0.0f;
  a.hasPollen = false;
  for (int i = 0; i < P_COUNT; i++) {
    JsonVariant v = c[POLLEN_KEYS[i]];
    a.pollen[i] = v | 0.0f;
    if (!v.isNull()) a.hasPollen = true;
  }
  a.valid = true;
  return true;
}

// Stufen des Europäischen Luftqualitätsindex (EAQI)
const char* aqiText(float aqi) {
  if (aqi < 20) return "Gut";
  if (aqi < 40) return "Befriedigend";
  if (aqi < 60) return "Mäßig";
  if (aqi < 80) return "Schlecht";
  if (aqi < 100) return "Sehr schlecht";
  return "Extrem schlecht";
}

uint16_t aqiColor(float aqi) {
  if (aqi < 20) return rgb(94, 156, 122);
  if (aqi < 40) return rgb(157, 180, 106);
  if (aqi < 60) return rgb(201, 166, 78);
  if (aqi < 80) return rgb(194, 122, 69);
  if (aqi < 100) return rgb(168, 83, 75);
  return rgb(140, 84, 150);
}

static const char* pollenLevel(float v) {
  if (v < 20) return "gering";
  if (v < 100) return "mäßig";
  return "stark";
}

void pollenSummary(const AirData& a, char* buf, int len) {
  buf[0] = 0;
  if (!a.hasPollen) return;
  int first = -1, second = -1;
  for (int i = 0; i < P_COUNT; i++) {
    if (a.pollen[i] < 1) continue;
    if (first < 0 || a.pollen[i] > a.pollen[first]) { second = first; first = i; }
    else if (second < 0 || a.pollen[i] > a.pollen[second]) second = i;
  }
  if (first < 0) { snprintf(buf, len, "keine Belastung"); return; }
  if (second < 0) snprintf(buf, len, "%s %s", POLLEN_NAMES[first], pollenLevel(a.pollen[first]));
  else snprintf(buf, len, "%s %s · %s %s", POLLEN_NAMES[first], pollenLevel(a.pollen[first]),
                POLLEN_NAMES[second], pollenLevel(a.pollen[second]));
}
