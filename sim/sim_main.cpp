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
  setTime(2026, 9, 26, 14, 37);
  if (!parseWeather(json.c_str(), app.wx)) { printf("Parser-Fehler\n"); return 3; }
  pickTagline(app.wx, 9, 26);
  printf("[%s] temp %.1f code %d hours %d days %d tag '%s'\n", scen, app.wx.temp, app.wx.code, app.wx.nHours, app.wx.nDays, app.wx.tagline);

  std::string s = scen;
  const char* names[] = {"home", "hourly", "daily", "details"};
  for (int i = 0; i < SCR_COUNT; i++) {
    app.screen = (Screen)i;
    gfx.frame([&] { uiDrawScreen(gfx); });
    save((s + "_" + names[i]).c_str());
  }
  if (s != "partly") return 0;

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
