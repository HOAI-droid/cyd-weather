#include "weather.h"
#include <ArduinoJson.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "theme.h"

void buildWeatherUrl(char* buf, int len, float lat, float lon) {
  snprintf(buf, len,
           "/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,"
           "weather_code,cloud_cover,pressure_msl,wind_speed_10m,wind_direction_10m,wind_gusts_10m"
           "&hourly=temperature_2m,precipitation_probability,weather_code,is_day&forecast_hours=24"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min,sunrise,sunset,uv_index_max,"
           "precipitation_probability_max&forecast_days=6&minutely_15=precipitation&forecast_minutely_15=9"
           "&timezone=auto&timeformat=unixtime",
           lat, lon);
}

void buildGeocodeUrl(char* buf, int len, const char* query, int count) {
  // Nur den Ortsnamen suchen ("Leipzig, Sachsen" -> "Leipzig"), UTF-8 prozent-kodiert
  char enc[160];
  int o = 0;
  for (const unsigned char* p = (const unsigned char*)query; *p && *p != ',' && o < (int)sizeof(enc) - 4; p++) {
    unsigned char c = *p;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.') enc[o++] = c;
    else o += snprintf(enc + o, sizeof(enc) - o, "%%%02X", c);
  }
  while (o > 0 && enc[o - 1] == '0' && o >= 3 && enc[o - 3] == '%' && enc[o - 2] == '2') o -= 3;  // Leerzeichen am Ende
  enc[o] = 0;
  const char* start = enc;
  while (strncmp(start, "%20", 3) == 0) start += 3;  // Leerzeichen am Anfang
  snprintf(buf, len, "/v1/search?name=%s&count=%d&language=de&format=json", start, count);
}

bool parseGeocode(const char* json, char* name, int nameLen, float& lat, float& lon) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  JsonObject r = doc["results"][0];
  if (r.isNull()) return false;
  lat = r["latitude"] | 0.0f;
  lon = r["longitude"] | 0.0f;
  snprintf(name, nameLen, "%s", (const char*)(r["name"] | ""));
  return name[0] != 0;
}

int parseGeocodeList(const char* json, Place* out, int maxN) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return 0;
  int n = 0;
  for (JsonObject r : doc["results"].as<JsonArray>()) {
    if (n >= maxN) break;
    const char* name = r["name"] | "";
    if (!name[0]) continue;
    Place& p = out[n++];
    snprintf(p.name, sizeof(p.name), "%s", name);
    const char* a1 = r["admin1"] | "";
    const char* cc = r["country_code"] | "";
    if (a1[0] && cc[0]) snprintf(p.region, sizeof(p.region), "%s, %s", a1, cc);
    else snprintf(p.region, sizeof(p.region), "%s", a1[0] ? a1 : (const char*)(r["country"] | ""));
    p.lat = r["latitude"] | 0.0f;
    p.lon = r["longitude"] | 0.0f;
  }
  return n;
}

int rainOutlook(const WeatherData& w, time_t now, int& startMin, float& peak) {
  startMin = -1;
  peak = 0;
  for (int i = 0; i < w.nRain15; i++) {
    time_t t = w.rain15T0 + i * 900;
    if (t + 900 <= now) continue;  // schon vorbei
    float mmh = w.rain15[i] * 4;
    if (mmh > peak) peak = mmh;
    if (startMin < 0 && w.rain15[i] >= 0.05f) startMin = t <= now ? 0 : (int)((t - now) / 60);
  }
  if (startMin < 0) return 0;
  return startMin == 0 ? 1 : 2;
}

bool parseWeather(const char* json, WeatherData& w) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  JsonObject c = doc["current"];
  if (c.isNull()) return false;
  w.temp = c["temperature_2m"] | 0.0f;
  w.humidity = c["relative_humidity_2m"] | 0.0f;
  w.feels = c["apparent_temperature"] | w.temp;
  w.isDay = (c["is_day"] | 1) == 1;
  w.precip = c["precipitation"] | 0.0f;
  w.code = c["weather_code"] | 0;
  w.cloud = c["cloud_cover"] | 0;
  w.pressure = c["pressure_msl"] | 0.0f;
  w.wind = c["wind_speed_10m"] | 0.0f;
  w.windDir = c["wind_direction_10m"] | 0;
  w.gusts = c["wind_gusts_10m"] | 0.0f;

  JsonObject h = doc["hourly"];
  JsonArray ht = h["time"];
  w.nHours = 0;
  for (size_t i = 0; i < ht.size() && w.nHours < 24; i++) {
    HourData& d = w.hours[w.nHours++];
    d.t = ht[i] | 0L;
    d.temp = h["temperature_2m"][i] | 0.0f;
    d.pop = h["precipitation_probability"][i] | 0;
    d.code = h["weather_code"][i] | 0;
    d.day = (h["is_day"][i] | 1) == 1;
  }

  JsonObject dl = doc["daily"];
  JsonArray dt = dl["time"];
  w.nDays = 0;
  for (size_t i = 0; i < dt.size() && w.nDays < 6; i++) {
    DayData& d = w.days[w.nDays++];
    d.t = dt[i] | 0L;
    d.code = dl["weather_code"][i] | 0;
    d.tmax = dl["temperature_2m_max"][i] | 0.0f;
    d.tmin = dl["temperature_2m_min"][i] | 0.0f;
    d.sunrise = dl["sunrise"][i] | 0L;
    d.sunset = dl["sunset"][i] | 0L;
    d.uv = dl["uv_index_max"][i] | 0.0f;
    d.pop = dl["precipitation_probability_max"][i] | 0;
  }
  JsonObject m = doc["minutely_15"];
  JsonArray mt = m["time"];
  w.nRain15 = 0;
  w.rain15T0 = mt.size() ? (time_t)(mt[0] | 0L) : 0;
  for (size_t i = 0; i < mt.size() && w.nRain15 < 9; i++) w.rain15[w.nRain15++] = m["precipitation"][i] | 0.0f;

  w.valid = w.nDays > 0;
  return w.valid;
}

// ------------------------------------------------------------ WMO-Codes
Group codeGroup(uint8_t c) {
  switch (c) {
    case 0: return G_CLEAR;
    case 1: case 2: return G_PARTLY;
    case 3: return G_CLOUD;
    case 45: case 48: return G_FOG;
    case 51: case 53: case 55: case 56: case 57: return G_DRIZZLE;
    case 61: case 63: case 66: case 80: case 81: return G_RAIN;
    case 65: case 67: case 82: return G_HEAVY;
    case 71: case 73: case 75: case 77: case 85: case 86: return G_SNOW;
    default: return c >= 95 ? G_THUNDER : G_CLOUD;
  }
}

const char* codeText(uint8_t c, bool day) {
  switch (c) {
    case 0: return day ? "Sonnig" : "Klar";
    case 1: return day ? "Heiter" : "Überwiegend klar";
    case 2: return "Leicht bewölkt";
    case 3: return "Bedeckt";
    case 45: return "Nebel";
    case 48: return "Reifnebel";
    case 51: return "Leichter Niesel";
    case 53: return "Nieselregen";
    case 55: return "Starker Niesel";
    case 56: case 57: return "Gefrierender Niesel";
    case 61: return "Leichter Regen";
    case 63: return "Mäßiger Regen";
    case 65: return "Starker Regen";
    case 66: case 67: return "Gefrierender Regen";
    case 71: return "Leichter Schneefall";
    case 73: return "Schneefall";
    case 75: return "Starker Schneefall";
    case 77: return "Schneegriesel";
    case 80: return "Leichte Schauer";
    case 81: return "Regenschauer";
    case 82: return "Heftige Schauer";
    case 85: case 86: return "Schneeschauer";
    case 95: return "Gewitter";
    case 96: case 99: return "Gewitter mit Hagel";
    default: return "Unbekannt";
  }
}

Icon codeIcon(uint8_t c, bool day) {
  switch (codeGroup(c)) {
    case G_CLEAR: return day ? IC_SUN : IC_MOON;
    case G_PARTLY: return day ? IC_PARTLY_DAY : IC_PARTLY_NIGHT;
    case G_CLOUD: return IC_CLOUD;
    case G_FOG: return IC_FOG;
    case G_DRIZZLE: return (c == 56 || c == 57) ? IC_SLEET : IC_DRIZZLE;
    case G_RAIN: case G_HEAVY: return (c == 66 || c == 67) ? IC_SLEET : IC_RAIN;
    case G_SNOW: return IC_SNOW;
    case G_THUNDER: return IC_THUNDER;
  }
  return IC_CLOUD;
}

// Graphit-Hintergrund, leicht getönt nach Wetterlage
void themeColors(uint8_t code, bool day, uint16_t& top, uint16_t& bot) {
  Group g = codeGroup(code);
  if (!day) { top = rgb(20, 23, 32); bot = rgb(8, 9, 12); return; }
  switch (g) {
    case G_CLEAR:   top = rgb(44, 42, 38); bot = rgb(15, 16, 18); break;
    case G_PARTLY:  top = rgb(38, 40, 45); bot = rgb(14, 16, 19); break;
    case G_SNOW:    top = rgb(42, 46, 54); bot = rgb(17, 19, 24); break;
    case G_THUNDER: top = rgb(38, 32, 48); bot = rgb(14, 12, 19); break;
    case G_DRIZZLE: case G_RAIN: case G_HEAVY:
                    top = rgb(28, 36, 46); bot = rgb(12, 15, 19); break;
    default:        top = rgb(36, 38, 42); bot = rgb(16, 17, 20); break;
  }
}

uint16_t tempColor(float t) {
  struct Stop { float t; uint8_t r, g, b; };
  static const Stop st[] = {{-10, 94, 200, 255}, {0, 94, 200, 255}, {10, 125, 224, 192},
                            {20, 255, 209, 102}, {30, 255, 138, 76}, {40, 255, 90, 90}};
  if (t <= st[0].t) return rgb(st[0].r, st[0].g, st[0].b);
  for (int i = 1; i < 6; i++) {
    if (t <= st[i].t) {
      float f = (t - st[i - 1].t) / (st[i].t - st[i - 1].t);
      return rgb(st[i - 1].r + (st[i].r - st[i - 1].r) * f, st[i - 1].g + (st[i].g - st[i - 1].g) * f,
                 st[i - 1].b + (st[i].b - st[i - 1].b) * f);
    }
  }
  return rgb(255, 90, 90);
}

const char* windName(int deg) {
  static const char* n[] = {"Nord", "Nordost", "Ost", "Südost", "Süd", "Südwest", "West", "Nordwest"};
  return n[((deg + 22) / 45) % 8];
}

const char* uvText(float uv) {
  if (uv < 3) return "niedrig";
  if (uv < 6) return "mäßig";
  if (uv < 8) return "hoch";
  if (uv < 11) return "sehr hoch";
  return "extrem";
}

// ------------------------------------------------------------ Sprüche
static const char* pick(const char* const* list, int n) { return list[rand() % n]; }

void pickTagline(WeatherData& w, int month, int mday) {
  static const char* hot[] = {"Ist das noch Wetter oder schon Sauna?", "Eis essen ist heute Pflicht.",
                              "Sonnencreme! Ernsthaft."};
  static const char* sunny[] = {"Perfekter Tag für einen Spaziergang.", "Sonnenbrille auf, Laune hoch!",
                                "Vitamin D zum Nulltarif."};
  static const char* sunCold[] = {"Sonnig, aber Mütze auf!", "Kalt, aber schön. Wie ein Kühlschrank mit Licht."};
  static const char* night[] = {"Sternklar. Wünsch dir was!", "Perfekt zum Sterneschauen.",
                                "Der Mond hat heute Dienst."};
  static const char* partly[] = {"Wolken mit Sonnenpausen.", "Die Sonne spielt Verstecken.",
                                 "Gemischtes Doppel am Himmel."};
  static const char* cloud[] = {"Grau ist auch eine Farbe.", "Der Himmel hat heute Homeoffice.",
                                "Perfektes Sofa-Wetter."};
  static const char* fog[] = {"Sichtweite: bis zum Gartenzaun.", "Heute alles etwas mysteriös.",
                              "Nebel. Die Welt lädt noch."};
  static const char* drizzle[] = {"Nieselt nur. Das zählt nicht.", "Feuchtigkeitscreme gratis."};
  static const char* rainy[] = {"Schirm oder Frisur, du hast die Wahl.", "Gut für die Pflanzen!",
                                "Pfützenspringen erlaubt."};
  static const char* heavy[] = {"Bau schon mal eine Arche.", "Heute lieber Gummistiefel.",
                                "Die Wolken haben Ausverkauf."};
  static const char* snow[] = {"Schneemann-Alarm!", "Schneeballschlacht, jemand?", "Winterwunderland!"};
  static const char* storm[] = {"Zeus hat schlechte Laune.", "Stecker raus, Popcorn rein.",
                                "Donnerwetter!"};
  static const char* freezing[] = {"Pinguin-Wetter!", "Brrr. Handschuhe nicht vergessen."};
  static const char* windy[] = {"Festhalten! Die Frisur hat keine Chance.", "Drachen-Wetter!"};

  const char* t;
  Group g = codeGroup(w.code);
  if (month == 12 && mday >= 24 && mday <= 26) t = "Frohe Weihnachten!";
  else if (month == 10 && mday == 31) t = "Happy Halloween! Buuuh!";
  else if (month == 12 && mday == 31) t = "Guten Rutsch ins neue Jahr!";
  else if (w.wind >= 50) t = pick(windy, 2);
  else if (g == G_THUNDER) t = pick(storm, 3);
  else if (g == G_HEAVY) t = pick(heavy, 3);
  else if (g == G_RAIN) t = pick(rainy, 3);
  else if (g == G_DRIZZLE) t = pick(drizzle, 2);
  else if (g == G_SNOW) t = pick(snow, 3);
  else if (w.temp <= -5) t = pick(freezing, 2);
  else if (g == G_FOG) t = pick(fog, 3);
  else if (g == G_CLOUD) t = pick(cloud, 3);
  else if (!w.isDay) t = pick(night, 3);
  else if (w.temp >= 28) t = pick(hot, 3);
  else if (w.temp <= 5) t = pick(sunCold, 2);
  else if (g == G_PARTLY) t = pick(partly, 3);
  else t = pick(sunny, 3);
  snprintf(w.tagline, sizeof(w.tagline), "%s", t);
}

float frogHeight(const WeatherData& w) {
  static const float h[] = {1.0f, 0.8f, 0.55f, 0.45f, 0.35f, 0.2f, 0.1f, 0.3f, 0.05f};
  float v = h[codeGroup(w.code)];
  if (w.temp < 0) v -= 0.1f;
  if (w.temp > 32) v -= 0.1f;
  return v < 0.03f ? 0.03f : v;
}
