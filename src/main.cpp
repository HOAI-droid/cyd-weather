// ============================================================================
//  CYD Wetterstation – ESP32-2432S028R ("Cheap Yellow Display")
//  Wetterdaten: Open-Meteo.com (frei, ohne API-Schlüssel)
// ============================================================================
#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
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

// HTTPS-GET, Antwort in body. Zertifikat wird nicht geprüft (öffentliche Wetterdaten).
static bool httpsGet(const char* host, const char* path, String& body) {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  String url = String("https://") + host + path;
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  bool ok = code == 200;
  if (ok) body = http.getString();
  else Serial.printf("%s: HTTP %d\n", host, code);
  http.end();
  return ok;
}

static bool fetchWeather() {
  char path[512];
  buildWeatherUrl(path, sizeof(path), app.lat, app.lon);
  String body;
  if (!httpsGet("api.open-meteo.com", path, body)) return false;
  if (!parseWeather(body.c_str(), fresh)) return false;
  fresh.fetchedMs = millis();
  pickTagline(fresh, app.now.tm_mon + 1, app.now.tm_mday);
  app.wx = fresh;
  Serial.printf("Wetter: %.1f °C, Code %d\n", app.wx.temp, app.wx.code);
  return true;
}

// ---------------------------------------------------------------- Einrichtung (WLAN + Ort)
static Preferences settings;
static char portalNote[96] = "";

static bool loadLocation() {
  settings.begin("ort", true);
  String c = settings.getString("city", "");
  app.lat = settings.getFloat("lat", 0);
  app.lon = settings.getFloat("lon", 0);
  settings.end();
  snprintf(app.city, sizeof(app.city), "%s", c.c_str());
  return app.city[0] != 0;
}

static void saveLocation() {
  settings.begin("ort", false);
  settings.putString("city", app.city);
  settings.putFloat("lat", app.lat);
  settings.putFloat("lon", app.lon);
  settings.end();
}

static bool findCity(const char* query) {
  char path[256], name[48];
  float lat, lon;
  buildGeocodeUrl(path, sizeof(path), query);
  String body;
  if (!httpsGet("geocoding-api.open-meteo.com", path, body)) return false;
  if (!parseGeocode(body.c_str(), name, sizeof(name), lat, lon)) return false;
  snprintf(app.city, sizeof(app.city), "%s", name);
  app.lat = lat;
  app.lon = lon;
  saveLocation();
  Serial.printf("Ort: %s (%.4f, %.4f)\n", app.city, lat, lon);
  return true;
}

static void onPortalStart(WiFiManager*) {
  gfx.frame([&] { uiDrawPortal(gfx, PORTAL_AP_NAME, portalNote); });
}

// Öffnet die Einrichtungsseite, bis WLAN verbunden und Ort gefunden ist.
// force = true: Seite immer zeigen (z. B. nach langem Druck auf den Stadtnamen)
static void setupWifiAndCity(bool force) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("cyd-wetter");
  bool haveCity = loadLocation();
  bool firstTry = true;
  while (true) {
    WiFiManager wm;
    WiFiManagerParameter cityParam("city", "Stadt oder Ort (z. B. Leipzig)", app.city, 40);
    wm.addParameter(&cityParam);
    wm.setTitle("CYD Wetterstation");
    wm.setAPCallback(onPortalStart);
    wm.setConnectTimeout(20);
    wm.setConfigPortalTimeout(force ? 300 : 0);
    std::vector<const char*> menu = {"wifi", "exit"};
    wm.setMenu(menu);

    bool connected;
    if (force || !haveCity || !firstTry) {
      connected = wm.startConfigPortal(PORTAL_AP_NAME, PORTAL_AP_PASSWORD[0] ? PORTAL_AP_PASSWORD : nullptr);
    } else {
      boot("Verbinde mit WLAN …", 0);
      connected = wm.autoConnect(PORTAL_AP_NAME, PORTAL_AP_PASSWORD[0] ? PORTAL_AP_PASSWORD : nullptr);
    }
    firstTry = false;
    if (!connected) {
      if (force && haveCity) { WiFi.begin(); return; }  // Zeit abgelaufen: alte Einstellungen behalten
      snprintf(portalNote, sizeof(portalNote), "Verbindung fehlgeschlagen. Bitte WLAN und Passwort prüfen.");
      continue;
    }

    const char* wanted = cityParam.getValue();
    if (wanted[0] && strcmp(wanted, app.city) != 0) {
      boot("Suche Ort …", 0);
      if (!findCity(wanted)) {
        snprintf(portalNote, sizeof(portalNote), "Ort \"%.40s\" nicht gefunden. Bitte Schreibweise prüfen.", wanted);
        haveCity = false;
        continue;
      }
    }
    if (!app.city[0]) {
      snprintf(portalNote, sizeof(portalNote), "Bitte eine Stadt eintragen.");
      continue;
    }
    portalNote[0] = 0;
    return;
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
      else if (app.screen == SCR_HOME && uiHitCity(g.x, g.y)) {
        setupWifiAndCity(true);
        nextFetch = 0;  // sofort neues Wetter für den (evtl. neuen) Ort
        app.wx.valid = false;
        needRedraw = true;
      }
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

  setupWifiAndCity(false);
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
