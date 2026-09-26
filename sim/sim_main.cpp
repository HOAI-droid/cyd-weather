// Host-Simulator: rendert Bildschirme der Firmware als Bilder (PPM)
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "../src/app.h"
#include "../src/eggs.h"
#include "../src/gfx.h"
#include "../src/ui.h"

AppState app;
extern uint32_t simMillis;
static TFT_eSPI tft;
static Gfx gfx(&tft);

static void save(const char* name) {
  std::string p = std::string("build/") + name + ".ppm";
  FILE* f = fopen(p.c_str(), "wb");
  fprintf(f, "P6\n320 240\n255\n");
  for (int y = 0; y < 240; y++)
    for (int x = 0; x < 320; x++) {
      uint16_t c = tft.fb[y][x];
      uint8_t rgb[3] = {(uint8_t)((c >> 11) * 255 / 31), (uint8_t)(((c >> 5) & 63) * 255 / 63), (uint8_t)((c & 31) * 255 / 31)};
      fwrite(rgb, 1, 3, f);
    }
  fclose(f);
}

static void setTime(int y, int mo, int d, int h, int mi) {
  struct tm t {};
  t.tm_year = y - 1900; t.tm_mon = mo - 1; t.tm_mday = d; t.tm_hour = h; t.tm_min = mi; t.tm_isdst = -1;
  app.epoch = mktime(&t);
  localtime_r(&app.epoch, &app.now);
  app.timeValid = true;
}

static std::string readFile(const std::string& path) {
  std::string out;
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return out;
  char b[4096]; size_t n;
  while ((n = fread(b, 1, sizeof(b), f)) > 0) out.append(b, n);
  fclose(f);
  return out;
}

static void shot(Screen s, const char* name) {
  app.screen = s;
  gfx.frame([&] { uiDrawScreen(gfx); });
  save(name);
}

static void egg(EggMode m, uint32_t runMs, const char* name, TouchNow t = {false, 0, 0}) {
  simMillis = 100000;
  eggStart(m);
  for (uint32_t ms = 0; ms <= runMs; ms += 40) { simMillis = 100000 + ms; eggLoop(gfx, t); }
  save(name);
}

int main(int argc, char** argv) {
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  srand(3);
  const char* scen = argc > 1 ? argv[1] : "partly";
  std::string path = std::string("build/sample_") + scen + ".json";
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) { printf("fehlt: %s\n", path.c_str()); return 1; }
  std::string json; char b[4096]; size_t n;
  while ((n = fread(b, 1, sizeof(b), f)) > 0) json.append(b, n);
  fclose(f);
  if (!gfx.begin()) return 2;
  snprintf(app.city, sizeof(app.city), "Berlin");
  app.lat = 52.52f; app.lon = 13.41f;
  setTime(2026, 9, 26, 14, 37);
  if (!parseWeather(json.c_str(), app.wx)) { printf("Parser-Fehler\n"); return 3; }
  pickTagline(app.wx, 9, 26);
  printf("[%s] temp %.1f code %d hours %d days %d tag '%s'\n", scen, app.wx.temp, app.wx.code, app.wx.nHours, app.wx.nDays, app.wx.tagline);

  std::string s = scen;
  std::string air = readFile("build/sample_air.json");
  if (!parseAir(air.c_str(), app.air)) { printf("Luft-Parser-Fehler\n"); return 4; }
  std::string radar = readFile("build/sample_radar.bin");
  static uint8_t radarPx[RADAR_W * RADAR_H];
  if (radar.size() == sizeof(radarPx)) {
    memcpy(radarPx, radar.data(), sizeof(radarPx));
    app.radar.px = radarPx;
    app.radar.mapOk = app.radar.rainOk = true;
    app.radar.frameTime = app.epoch - 7 * 60;
  }

  const char* names[] = {"home", "hourly", "daily", "radar", "air", "details"};
  for (int i = 0; i < SCR_COUNT; i++) {
    if (s != "partly" && (i == SCR_RADAR || i == SCR_AIR)) continue;
    shot((Screen)i, (s + "_" + names[i]).c_str());
  }
  if (s != "partly") return 0;

  // Ortswahl
  const Place demo[] = {{"Berlin", "Berlin, DE", 52.52f, 13.41f}, {"Hamburg", "Hamburg, DE", 53.55f, 9.99f},
                        {"München", "Bayern, DE", 48.14f, 11.58f}};
  for (const Place& p : demo) app.places[app.nPlaces++] = p;
  shot(SCR_PLACES, "places");
  snprintf(app.query, sizeof(app.query), "Münch");
  shot(SCR_SEARCH, "search");
  app.kbAlt = true; app.query[0] = 0;
  snprintf(app.searchMsg, sizeof(app.searchMsg), "Kein Ort \"Xyz\" gefunden. Bitte Schreibweise prüfen.");
  shot(SCR_SEARCH, "search_alt");
  app.kbAlt = false; app.searchMsg[0] = 0;
  snprintf(app.query, sizeof(app.query), "Münch");
  const Place res[] = {{"München", "Bayern, DE", 48.14f, 11.58f}, {"Münchberg", "Bayern, DE", 50.19f, 11.79f},
                       {"Münchenbernsdorf", "Thüringen, DE", 50.82f, 11.93f}, {"Münchwilen", "Thurgau, CH", 47.48f, 8.99f}};
  for (const Place& p : res) app.results[app.nResults++] = p;
  shot(SCR_RESULTS, "results");
  { RadarView keep = app.radar; app.radar = RadarView(); shot(SCR_RADAR, "radar_loading"); app.radar = keep; }

  setTime(2026, 9, 26, 14, 37);
  // Tastentreffer prüfen: "Q" oben links, "Suchen" unten rechts
  { const char* ch = ""; int k = uiKeyAt(20, 118, &ch); printf("Taste (20,118): %d '%s'\n", k, ch);
    k = uiKeyAt(280, 220, &ch); printf("Taste (280,220): %d\n", k); }
  setTime(2026, 9, 26, 13, 37); app.screen = SCR_HOME; app.glitch = 5;
  gfx.frame([&] { uiDrawScreen(gfx); }); save("egg_leet");
  setTime(2026, 12, 24, 10, 0); gfx.frame([&] { uiDrawScreen(gfx); }); save("egg_xmas");
  setTime(2026, 10, 31, 18, 0); gfx.frame([&] { uiDrawScreen(gfx); }); save("egg_halloween");
  setTime(2026, 9, 26, 14, 37);
  { WeatherData keep = app.wx; app.wx.valid = false; app.timeValid = false; gfx.frame([&] { uiDrawScreen(gfx); }); save("loading"); app.wx = keep; app.timeValid = true; }
  gfx.frame([&] { uiDrawBoot(gfx, "Verbinde mit WLAN …", 1); }); save("boot");
  gfx.frame([&] { uiDrawPortal(gfx, "CYD-Wetter", "Ort \"Xyz\" nicht gefunden. Bitte Schreibweise prüfen."); }); save("portal");

  egg(EGG_FROG, 2400, "egg_frog");
  egg(EGG_GAME, 6000, "egg_game", {true, 150, 220});
  egg(EGG_MATRIX, 2000, "egg_matrix");
  egg(EGG_APRIL, 5000, "egg_april");
  egg(EGG_JOKE, 100, "egg_joke");
  egg(EGG_DISCO, 1200, "egg_disco");
  egg(EGG_DARK, 400, "egg_dark");
  egg(EGG_PARTY, 2500, "egg_newyear");
  printf("Font-Ladevorgänge im Sprite: %d\n", gfx.spr.loads);
  return 0;
}
