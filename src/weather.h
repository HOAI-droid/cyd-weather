// Wetterdaten von Open-Meteo (kostenlos, kein API-Schlüssel)
#pragma once
#include <stdint.h>
#include <time.h>

enum Icon : uint8_t {
  IC_SUN, IC_MOON, IC_PARTLY_DAY, IC_PARTLY_NIGHT, IC_CLOUD, IC_FOG,
  IC_DRIZZLE, IC_RAIN, IC_SLEET, IC_SNOW, IC_THUNDER
};

enum Group : uint8_t { G_CLEAR, G_PARTLY, G_CLOUD, G_FOG, G_DRIZZLE, G_RAIN, G_HEAVY, G_SNOW, G_THUNDER };

struct HourData {
  time_t t;
  float temp;
  uint8_t pop;   // Regenwahrscheinlichkeit %
  uint8_t code;  // WMO-Wettercode
  bool day;
};

struct DayData {
  time_t t;
  float tmin, tmax;
  uint8_t pop;
  uint8_t code;
  float uv;
  time_t sunrise, sunset;
};

struct WeatherData {
  bool valid = false;
  uint32_t fetchedMs = 0;
  float temp = 0, feels = 0, humidity = 0, wind = 0, gusts = 0, pressure = 0, precip = 0;
  int windDir = 0;
  uint8_t cloud = 0, code = 0;
  bool isDay = true;
  HourData hours[24];
  int nHours = 0;
  DayData days[6];
  int nDays = 0;
  char tagline[64] = "";
};

// JSON-Antwort der Open-Meteo-API auswerten
bool parseWeather(const char* json, WeatherData& out);
// Pfad + Query für die API (Host: api.open-meteo.com)
void buildWeatherUrl(char* buf, int len, float lat, float lon);

const char* codeText(uint8_t code, bool day);
Icon codeIcon(uint8_t code, bool day);
Group codeGroup(uint8_t code);
void themeColors(uint8_t code, bool day, uint16_t& top, uint16_t& bottom);
uint16_t tempColor(float t);
const char* windName(int deg);
const char* uvText(float uv);
void pickTagline(WeatherData& w, int month, int mday);
float frogHeight(const WeatherData& w);  // 0 = unten auf der Leiter, 1 = ganz oben
