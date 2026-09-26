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
#include "net.h"
#include "ui.h"

AppState app;
static TFT_eSPI tft;
static Gfx gfx(&tft);

static bool needRedraw = true;
static uint32_t nextFetch = 0, nextAir = 0, lastSecond = 0, lastGlitch = 0, wakeUntil = 0, lastReconnect = 0;
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
  char path[512];
  buildWeatherUrl(path, sizeof(path), app.lat, app.lon);
  String body;
  if (!netGet("api.open-meteo.com", path, body)) return false;
  if (!parseWeather(body.c_str(), fresh)) return false;
  fresh.fetchedMs = millis();
  pickTagline(fresh, app.now.tm_mon + 1, app.now.tm_mday);
  app.wx = fresh;
  Serial.printf("Wetter: %.1f °C, Code %d\n", app.wx.temp, app.wx.code);
  return true;
}

static void redraw() { gfx.frame([&] { uiDrawScreen(gfx); }); }

static void fetchAir() {
  bool ok = netFetchAir(app.air, app.lat, app.lon);
  nextAir = millis() + (ok ? 30 * 60000UL : 2 * 60000UL);
}

// Radar laden, wenn die Ansicht offen ist und das Bild älter als 10 Minuten ist
static void refreshRadar(bool force) {
  RadarView& v = app.radar;
  bool stale = !v.rainOk || millis() - v.fetchedMs > 10 * 60000UL;
  if (!force && !stale) return;
  if (!v.mapOk || !v.rainOk) { v.status[0] = 0; redraw(); }  // "wird geladen" zeigen
  netFetchRadar(v, app.lat, app.lon);
  if (!v.rainOk) v.fetchedMs = millis() - 8 * 60000UL;  // nach einem Fehler in 2 Minuten erneut
}

// ---------------------------------------------------------------- Einrichtung (WLAN + Ort)
static Preferences settings;
static char portalNote[96] = "";

// Gespeicherte Orte (bis zu MAX_PLACES)
static void loadPlaces() {
  settings.begin("orte", true);
  app.nPlaces = 0;
  int n = settings.getInt("n", 0);
  for (int i = 0; i < n && i < MAX_PLACES; i++) {
    char key[4] = {'p', (char)('0' + i), 0};
    if (settings.getBytes(key, &app.places[app.nPlaces], sizeof(Place)) == sizeof(Place)) {
      Place& p = app.places[app.nPlaces];
      p.name[sizeof(p.name) - 1] = 0;
      p.region[sizeof(p.region) - 1] = 0;
      app.nPlaces++;
    }
  }
  settings.end();
}

static void savePlaces() {
  settings.begin("orte", false);
  settings.putInt("n", app.nPlaces);
  for (int i = 0; i < app.nPlaces; i++) {
    char key[4] = {'p', (char)('0' + i), 0};
    settings.putBytes(key, &app.places[i], sizeof(Place));
  }
  settings.end();
}

static bool isCurrent(const Place& p) { return fabsf(p.lat - app.lat) < 0.01f && fabsf(p.lon - app.lon) < 0.01f; }

// Ort in die Liste aufnehmen (oder aktualisieren); bei voller Liste fliegt der älteste andere raus
static void rememberPlace(const Place& np) {
  for (int i = 0; i < app.nPlaces; i++) {
    Place& p = app.places[i];
    if (fabsf(p.lat - np.lat) < 0.01f && fabsf(p.lon - np.lon) < 0.01f) {
      if (np.region[0] || !p.region[0]) p = np;
      savePlaces();
      return;
    }
  }
  if (app.nPlaces == MAX_PLACES) {
    int drop = isCurrent(app.places[0]) ? 1 : 0;
    for (int i = drop; i < app.nPlaces - 1; i++) app.places[i] = app.places[i + 1];
    app.nPlaces--;
  }
  app.places[app.nPlaces++] = np;
  savePlaces();
}

static void rememberCurrent() {
  Place p{};
  snprintf(p.name, sizeof(p.name), "%s", app.city);
  p.lat = app.lat;
  p.lon = app.lon;
  rememberPlace(p);
}

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
  if (!netGet("geocoding-api.open-meteo.com", path, body)) return false;
  if (!parseGeocode(body.c_str(), name, sizeof(name), lat, lon)) return false;
  snprintf(app.city, sizeof(app.city), "%s", name);
  app.lat = lat;
  app.lon = lon;
  saveLocation();
  rememberCurrent();
  Serial.printf("Ort: %s (%.4f, %.4f)\n", app.city, lat, lon);
  return true;
}

// Wechselt zu einem gespeicherten oder gefundenen Ort und lädt alles neu
static void selectPlace(const Place& p) {
  snprintf(app.city, sizeof(app.city), "%s", p.name);
  app.lat = p.lat;
  app.lon = p.lon;
  saveLocation();
  rememberPlace(p);
  app.wx.valid = false;
  app.air.valid = false;
  app.radar.rainOk = false;  // die Karte merkt selbst, dass sie nicht mehr passt
  app.screen = SCR_HOME;
  redraw();
  nextFetch = 0;
  nextAir = 0;
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
  if (app.screen == SCR_RADAR) refreshRadar(false);
  if (app.screen == SCR_AIR && !app.air.valid) { redraw(); fetchAir(); }
}

// ---------------------------------------------------------------- Ortswahl
static void openPlaces() {
  if (app.nPlaces == 0 && app.city[0]) rememberCurrent();
  app.screen = SCR_PLACES;
  needRedraw = true;
}

// Hängt ein Zeichen an die Eingabe; Buchstaben werden am Wortanfang groß, sonst klein
static void typeChar(const char* ch) {
  size_t len = strlen(app.query);
  if (len + strlen(ch) >= sizeof(app.query) - 1 || len > 30) return;
  bool wordStart = len == 0 || app.query[len - 1] == ' ' || app.query[len - 1] == '-';
  char c[4];
  snprintf(c, sizeof(c), "%s", ch);
  if (!wordStart) {
    if (c[0] >= 'A' && c[0] <= 'Z' && c[1] == 0) c[0] += 32;
    else if (!strcmp(c, "Ä")) snprintf(c, sizeof(c), "ä");
    else if (!strcmp(c, "Ö")) snprintf(c, sizeof(c), "ö");
    else if (!strcmp(c, "Ü")) snprintf(c, sizeof(c), "ü");
  }
  strcat(app.query, c);
}

static void backspace() {
  size_t len = strlen(app.query);
  while (len > 0) {
    len--;
    bool cont = ((uint8_t)app.query[len] & 0xC0) == 0x80;  // UTF-8-Folgebyte
    app.query[len] = 0;
    if (!cont) break;
  }
}

static void runSearch() {
  if (strlen(app.query) < 2) {
    snprintf(app.searchMsg, sizeof(app.searchMsg), "Bitte mindestens 2 Buchstaben eingeben.");
    return;
  }
  snprintf(app.searchMsg, sizeof(app.searchMsg), "Suche …");
  redraw();
  int n = netSearchPlaces(app.query, app.results, 5);
  if (n < 0) snprintf(app.searchMsg, sizeof(app.searchMsg), "Keine Verbindung. Bitte später erneut versuchen.");
  else if (n == 0) snprintf(app.searchMsg, sizeof(app.searchMsg), "Kein Ort \"%.24s\" gefunden. Bitte Schreibweise prüfen.", app.query);
  else { app.searchMsg[0] = 0; app.nResults = n; app.screen = SCR_RESULTS; }
}

static void handleDialog(const Gesture& g) {
  switch (app.screen) {
    case SCR_PLACES: {
      if (g.type == GE_TAP && uiHitAction(g.x, g.y)) { app.screen = SCR_HOME; break; }
      int row = uiHitPlaceRow(g.x, g.y);
      if (row >= 0 && g.type == GE_LONG && !isCurrent(app.places[row])) {
        for (int i = row; i < app.nPlaces - 1; i++) app.places[i] = app.places[i + 1];
        app.nPlaces--;
        savePlaces();
      } else if (row >= 0 && g.type == GE_TAP) {
        Place p = app.places[row];
        if (isCurrent(p)) app.screen = SCR_HOME;
        else selectPlace(p);
      } else if (g.type == GE_TAP && uiHitPlacesSearch(g.x, g.y)) {
        app.query[0] = 0;
        app.searchMsg[0] = 0;
        app.kbAlt = false;
        app.screen = SCR_SEARCH;
      } else if (g.type == GE_TAP && uiHitPlacesWifi(g.x, g.y)) {
        setupWifiAndCity(true);
        app.screen = SCR_HOME;
        nextFetch = 0;
        nextAir = 0;
        app.wx.valid = false;
        app.air.valid = false;
        app.radar.rainOk = false;
      }
      break;
    }
    case SCR_SEARCH: {
      if (g.type != GE_TAP) break;
      if (uiHitAction(g.x, g.y)) { app.screen = SCR_PLACES; break; }
      const char* ch = "";
      switch (uiKeyAt(g.x, g.y, &ch)) {
        case KEY_CHAR: typeChar(ch); app.searchMsg[0] = 0; break;
        case KEY_BACK: backspace(); app.searchMsg[0] = 0; break;
        case KEY_ALT: app.kbAlt = !app.kbAlt; break;
        case KEY_SPACE: {
          size_t len = strlen(app.query);
          if (len && app.query[len - 1] != ' ') typeChar(" ");
          break;
        }
        case KEY_SEARCH: runSearch(); break;
        default: return;  // daneben getippt: nichts neu zeichnen
      }
      break;
    }
    case SCR_RESULTS: {
      if (g.type != GE_TAP) break;
      if (uiHitAction(g.x, g.y)) { app.screen = SCR_SEARCH; break; }
      int i = uiHitResult(g.x, g.y);
      if (i >= 0) selectPlace(app.results[i]);
      break;
    }
    default: break;
  }
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

  if (app.screen >= SCR_COUNT) { handleDialog(g); return; }

  switch (g.type) {
    case GE_SWIPE_LEFT: goScreen(+1); break;
    case GE_SWIPE_RIGHT: goScreen(-1); break;
    case GE_LONG:
      if (app.screen == SCR_HOME && uiHitClock(g.x, g.y)) eggStart(EGG_GAME);
      else if (app.screen == SCR_HOME && uiHitCity(g.x, g.y)) {
        setupWifiAndCity(true);
        nextFetch = 0;  // sofort neues Wetter für den (evtl. neuen) Ort
        nextAir = 0;
        app.wx.valid = false;
        app.air.valid = false;
        app.radar.rainOk = false;
        needRedraw = true;
      }
      break;
    case GE_TAP: {
      bool corner = eggCornerTap(g.x, g.y);
      if (eggMode() != EGG_NONE) break;  // Ecken-Code vollständig
      if (app.screen == SCR_HOME && uiHitCity(g.x, g.y)) { openPlaces(); break; }
      if (corner) break;
      if (app.screen == SCR_HOME && app.wx.valid && uiHitIcon(g.x, g.y)) { eggHomeTap(g.x, g.y); break; }
      if (g.x < 107) goScreen(-1);
      else if (g.x > 213) goScreen(+1);
      break;
    }
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

  loadPlaces();
  setupWifiAndCity(false);
  if (app.nPlaces == 0) rememberCurrent();
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
  fetchAir();
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
  if (app.screen < SCR_COUNT && (int32_t)(millis() - nextAir) >= 0) {
    fetchAir();
    if (app.screen == SCR_AIR) needRedraw = true;
  }
  if (app.screen == SCR_RADAR && app.wx.valid && millis() - app.radar.fetchedMs > 10 * 60000UL) {
    refreshRadar(false);
    needRedraw = true;
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
    redraw();
  }
  delay(5);
}
