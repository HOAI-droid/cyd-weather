#include "net.h"
#include <HTTPClient.h>
#include <new>
#include <PNGdec.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// Zertifikate werden nicht geprüft (öffentliche Daten, keine Zugangsdaten).
bool netGet(const char* host, const char* path, String& body) {
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

bool netFetchAir(AirData& out, float lat, float lon) {
  char path[320];
  buildAirUrl(path, sizeof(path), lat, lon);
  String body;
  if (!netGet("air-quality-api.open-meteo.com", path, body)) return false;
  AirData a;
  if (!parseAir(body.c_str(), a)) return false;
  a.fetchedMs = millis();
  out = a;
  Serial.printf("Luft: Index %.0f, PM2.5 %.1f\n", a.aqi, a.pm25);
  return true;
}

int netSearchPlaces(const char* query, Place* out, int maxN) {
  char path[256];
  buildGeocodeUrl(path, sizeof(path), query, maxN);
  String body;
  if (!netGet("geocoding-api.open-meteo.com", path, body)) return -1;
  return parseGeocodeList(body.c_str(), out, maxN);
}

// ---------------------------------------------------------------- Kacheln laden

// Sammelt eine HTTP-Antwort (auch Binärdaten) in einem wachsenden Puffer
class BufStream : public Stream {
 public:
  uint8_t* buf = nullptr;
  size_t len = 0, cap = 0;
  bool oom = false;
  ~BufStream() { free(buf); }
  size_t write(uint8_t c) override { return write(&c, 1); }
  size_t write(const uint8_t* d, size_t n) override {
    if (len + n > cap) {
      size_t nc = cap ? cap * 2 : 16384;
      while (nc < len + n) nc *= 2;
      if (nc > 96 * 1024) { oom = true; return 0; }
      uint8_t* nb = (uint8_t*)realloc(buf, nc);
      if (!nb) { oom = true; return 0; }
      buf = nb;
      cap = nc;
    }
    memcpy(buf + len, d, n);
    len += n;
    return n;
  }
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  void flush() override {}
};

static bool getBinary(const String& url, BufStream& out) {
  if (WiFi.status() != WL_CONNECTED) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  http.setUserAgent("cyd-wetter/2.0 (ESP32)");
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  bool ok = code == 200 && http.writeToStream(&out) > 0 && !out.oom;
  if (code != 200) Serial.printf("Kachel: HTTP %d\n", code);
  http.end();
  client.stop();  // TLS-Puffer freigeben, bevor der PNG-Decoder Speicher braucht
  return ok;
}

struct TileCtx {
  RadarView* v;
  TileRef t;
  bool map;
};

static int pngLine(PNGDRAW* d) {
  TileCtx* c = (TileCtx*)d->pUser;
  const uint8_t* p = d->pPixels;
  for (int x = 0; x < d->iWidth; x++) {
    uint8_t r = 0, g = 0, b = 0, a = 255;
    switch (d->iPixelType) {
      case PNG_PIXEL_TRUECOLOR_ALPHA: r = p[x * 4]; g = p[x * 4 + 1]; b = p[x * 4 + 2]; a = p[x * 4 + 3]; break;
      case PNG_PIXEL_TRUECOLOR: r = p[x * 3]; g = p[x * 3 + 1]; b = p[x * 3 + 2]; break;
      case PNG_PIXEL_GRAY_ALPHA: r = g = b = p[x * 2]; a = p[x * 2 + 1]; break;
      case PNG_PIXEL_GRAYSCALE: r = g = b = p[x]; break;
      case PNG_PIXEL_INDEXED: {
        int i;
        switch (d->iBpp) {
          case 1: i = (p[x >> 3] >> (7 - (x & 7))) & 1; break;
          case 2: i = (p[x >> 2] >> (6 - 2 * (x & 3))) & 3; break;
          case 4: i = (p[x >> 1] >> ((x & 1) ? 0 : 4)) & 15; break;
          default: i = p[x]; break;
        }
        r = d->pPalette[i * 3]; g = d->pPalette[i * 3 + 1]; b = d->pPalette[i * 3 + 2];
        a = d->iHasAlpha ? d->pPalette[768 + i] : 255;
        break;
      }
    }
    if (c->map) radarPutMap(*c->v, c->t, x, d->y, radarMapLevel(r, g, b));
    else radarPutRain(*c->v, c->t, x, d->y, radarRainLevel(r, g, b, a));
  }
  return 1;
}

static bool loadTile(const String& url, RadarView& v, const TileRef& t, bool map) {
  BufStream data;
  if (!getBinary(url, data)) return false;
  PNG* png = new (std::nothrow) PNG();
  if (!png) { Serial.println("PNG: zu wenig Speicher"); return false; }
  TileCtx ctx{&v, t, map};
  bool ok = png->openRAM(data.buf, (int)data.len, pngLine) == PNG_SUCCESS && png->decode(&ctx, 0) == PNG_SUCCESS;
  png->close();
  delete png;
  return ok;
}

bool netFetchRadar(RadarView& v, float lat, float lon) {
  if (!v.px) {
    v.px = (uint8_t*)calloc(RADAR_W * RADAR_H, 1);
    if (!v.px) { snprintf(v.status, sizeof(v.status), "Zu wenig Speicher für das Radar"); return false; }
  }
  TileRef tiles[RADAR_MAX_TILES];
  int n = radarTiles(lat, lon, RADAR_ZOOM, tiles);
  char buf[160];

  // Karte nur beim ersten Mal oder nach einem Ortswechsel
  if (!v.mapOk || fabsf(v.mapLat - lat) > 0.001f || fabsf(v.mapLon - lon) > 0.001f) {
    memset(v.px, 0, RADAR_W * RADAR_H);
    v.rainOk = false;
    bool ok = true;
    for (int i = 0; i < n && ok; i++) {
      snprintf(buf, sizeof(buf), "https://%c.basemaps.cartocdn.com/dark_all/%d/%d/%d.png", "abc"[i % 3], RADAR_ZOOM,
               tiles[i].x, tiles[i].y);
      ok = loadTile(buf, v, tiles[i], true);
    }
    v.mapOk = ok;
    v.mapLat = lat;
    v.mapLon = lon;
    if (!ok) { snprintf(v.status, sizeof(v.status), "Karte konnte nicht geladen werden"); return false; }
  }

  // neuestes Radarbild
  String json;
  if (!netGet("api.rainviewer.com", "/public/weather-maps.json", json)) {
    snprintf(v.status, sizeof(v.status), "Radardaten nicht erreichbar");
    return false;
  }
  char host[64], path[96];
  time_t t;
  if (!parseRainviewer(json.c_str(), host, sizeof(host), path, sizeof(path), t)) {
    snprintf(v.status, sizeof(v.status), "Radardaten unbekannt");
    return false;
  }
  json = String();
  radarClearRain(v);
  bool ok = true;
  for (int i = 0; i < n && ok; i++) {
    snprintf(buf, sizeof(buf), "%s%s/256/%d/%d/%d/2/1_1.png", host, path, RADAR_ZOOM, tiles[i].x, tiles[i].y);
    ok = loadTile(buf, v, tiles[i], false);
  }
  v.rainOk = ok;
  v.frameTime = t;
  v.fetchedMs = millis();
  if (!ok) snprintf(v.status, sizeof(v.status), "Radarbild unvollständig");
  else v.status[0] = 0;
  Serial.printf("Radar geladen (%d Kacheln), freier Speicher %u\n", n, (unsigned)ESP.getFreeHeap());
  return ok;
}
