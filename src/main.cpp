// ============================================================================
//  CYD Wetterstation – ESP32-2432S028R ("Cheap Yellow Display")
//  Wetterdaten: Open-Meteo.com (frei, ohne API-Schlüssel)
// ============================================================================
#include <Arduino.h>
#include <HTTPClient.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include "app.h"
#include "config.h"
#include "eggs.h"
#include "gfx.h"
#include "hw.h"
#include "input.h"
#include "ui.h"

AppState app;
static TFT_eSPI tft;
static Gfx gfx(&tft);

static bool needRedraw = true;
static uint32_t nextFetch = 0, lastSecond = 0, lastGlitch = 0, wakeUntil = 0, lastReconnect = 0;
static int lastMinute = -1;
static WeatherData fresh;  // Puffer für neue Daten (zu groß für den Stack)

// ---------------------------------------------------------------- Hilfen
static void boot(const char* msg, int step) {
  gfx.frame([&] { uiDrawBoot(gfx, msg, step); });
}

static void updateClock() {
  time_t now = time(nullptr);
  app.timeValid = now > 1700000000;
  app.epoch = now;
  localtime_r(&now, &app.now);
}

static bool fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) return false;
  char path[512];
  buildWeatherUrl(path, sizeof(path), LATITUDE, LONGITUDE);
  String url = String("http://api.open-meteo.com") + path;
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(url);
  int code = http.GET();
  bool ok = false;
  if (code == 200) {
    String body = http.getString();
    ok = parseWeather(body.c_str(), fresh);
  } else {
    Serial.printf("Open-Meteo: HTTP %d\n", code);
  }
  http.end();
  if (ok) {
    fresh.fetchedMs = millis();
    pickTagline(fresh, app.now.tm_mon + 1, app.now.tm_mday);
    app.wx = fresh;
    Serial.printf("Wetter: %.1f °C, Code %d\n", app.wx.temp, app.wx.code);
  }
  return ok;
}

static void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("cyd-wetter");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int step = 0;
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    boot("Verbinde mit WLAN …", step++);
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    boot("WLAN nicht erreichbar – prüfe config.h", 0);
    delay(3000);
  }
}

static void applyBrightness() {
  if (!app.timeValid) return;
  int h = app.now.tm_hour;
  bool night = NIGHT_START_HOUR > NIGHT_END_HOUR ? (h >= NIGHT_START_HOUR || h < NIGHT_END_HOUR)
                                                 : (h >= NIGHT_START_HOUR && h < NIGHT_END_HOUR);
  static int last = -1;
  int level = (night && millis() > wakeUntil) ? BRIGHTNESS_NIGHT : BRIGHTNESS_DAY;
  if (level != last) { hwBacklight(level); last = level; }
}

static void goScreen(int delta) {
  if (!app.wx.valid) return;
  app.screen = (Screen)((app.screen + SCR_COUNT + delta) % SCR_COUNT);
  needRedraw = true;
}

// ---------------------------------------------------------------- Eingaben
static void handleGesture(const Gesture& g) {
  if (g.type == GE_NONE) return;
  wakeUntil = millis() + 15000;  // nachts kurz aufhellen

  if (g.type == GE_BOOT) {
    if (eggMode() == EGG_NONE || eggMode() == EGG_JOKE) eggStart(EGG_JOKE);
    return;
  }
  if (g.type == GE_BOOT_LONG) { eggStart(EGG_DISCO); return; }

  if (eggMode() != EGG_NONE) {
    if (g.type == GE_TAP) eggTap(g.x, g.y);
    return;
  }

  switch (g.type) {
    case GE_SWIPE_LEFT: goScreen(+1); break;
    case GE_SWIPE_RIGHT: goScreen(-1); break;
    case GE_LONG:
      if (app.screen == SCR_HOME && uiHitClock(g.x, g.y)) eggStart(EGG_GAME);
      break;
    case GE_TAP:
      if (eggCornerTap(g.x, g.y)) break;
      if (app.screen == SCR_HOME && app.wx.valid && uiHitIcon(g.x, g.y)) { eggHomeTap(g.x, g.y); break; }
      if (g.x < 107) goScreen(-1);
      else if (g.x > 213) goScreen(+1);
      break;
    default: break;
  }
}

// ---------------------------------------------------------------- Setup & Loop
void setup() {
  Serial.begin(115200);
  srand(esp_random());
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  hwBegin();
  if (!gfx.begin()) {
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Zu wenig RAM fuer den Sprite", 10, 10, 2);
    while (true) delay(1000);
  }
  inputBegin();

  connectWifi();
  configTzTime(TZ_INFO, NTP_SERVER, "pool.ntp.org");
  for (int i = 0; i < 40 && !app.timeValid; i++) {
    boot("Hole Uhrzeit …", i);
    delay(250);
    updateClock();
  }
  boot("Lade Wetterdaten …", 0);
  bool ok = fetchWeather();
  app.offline = !ok;
  nextFetch = millis() + (ok ? WEATHER_INTERVAL_MIN * 60000UL : 60000UL);
  needRedraw = true;
}

void loop() {
  uint32_t now = millis();
  updateClock();

  if (WiFi.status() != WL_CONNECTED && now - lastReconnect > 30000) {
    lastReconnect = now;
    WiFi.reconnect();
  }

  handleGesture(inputPoll());

  if ((int32_t)(now - nextFetch) >= 0) {
    bool ok = fetchWeather();
    app.offline = !ok;
    nextFetch = now + (ok ? WEATHER_INTERVAL_MIN * 60000UL : 60000UL);
    if (ok) needRedraw = true;
  }

  if (now - lastSecond >= 1000) {
    lastSecond = now;
    eggCheckCalendar();
    applyBrightness();
    if (app.now.tm_min != lastMinute) { lastMinute = app.now.tm_min; needRedraw = true; }
    if (app.screen != SCR_HOME && now - inputLastActivity() > HOME_TIMEOUT_S * 1000UL) {
      app.screen = SCR_HOME;
      needRedraw = true;
    }
  }
  eggCheckLight();

  // Aktives Easter Egg zeichnet sich selbst
  if (eggMode() != EGG_NONE) {
    TouchNow t{false, 0, 0};
    t.down = inputDown(t.x, t.y);
    if (!eggLoop(gfx, t)) needRedraw = true;
    return;
  }

  // 13:37 – die Uhr flackert
  if (app.screen == SCR_HOME && uiIsLeetTime() && now - lastGlitch > 400) {
    lastGlitch = now;
    app.glitch = random(0, 255);
    needRedraw = true;
  }

  if (needRedraw) {
    needRedraw = false;
    gfx.frame([&] { uiDrawScreen(gfx); });
  }
  delay(5);
}
