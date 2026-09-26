#include "radar.h"
#include <ArduinoJson.h>
#include <math.h>
#include <stdio.h>

static const float PI_F = 3.14159265f;

static void project(float lat, float lon, int zoom, double& fx, double& fy) {
  double world = 256.0 * (1 << zoom);
  double la = lat * PI_F / 180.0;
  fx = (lon + 180.0) / 360.0 * world;
  fy = (1.0 - log(tan(la) + 1.0 / cos(la)) / PI_F) / 2.0 * world;
}

int radarTiles(float lat, float lon, int zoom, TileRef* out) {
  double fx, fy;
  project(lat, lon, zoom, fx, fy);
  int left = (int)floor(fx) - RADAR_W / 2, top = (int)floor(fy) - RADAR_H / 2;
  int n = 1 << zoom, cnt = 0;
  for (int ty = (int)floor(top / 256.0); ty <= (int)floor((top + RADAR_H - 1) / 256.0); ty++) {
    if (ty < 0 || ty >= n) continue;
    for (int tx = (int)floor(left / 256.0); tx <= (int)floor((left + RADAR_W - 1) / 256.0); tx++) {
      if (cnt >= RADAR_MAX_TILES) return cnt;
      TileRef& t = out[cnt++];
      t.x = ((tx % n) + n) % n;
      t.y = ty;
      t.sx = tx * 256 - left;
      t.sy = ty * 256 - top;
    }
  }
  return cnt;
}

float radarMetersPerPixel(float lat, int zoom) {
  return 156543.03f * cosf(lat * PI_F / 180) / (1 << zoom);
}

uint8_t radarRainLevel(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  if (a < 48) return 0;
  // Farbskalen von RainViewer: helles Blau (schwach) -> dunkles Blau -> Gelb -> Rot/Magenta (Unwetter)
  if (r > 200 && g > 160 && b < 130) return 4;
  if (r > 170 && g < 150) return 5;
  int lum = (r * 3 + g * 6 + b) / 10;
  if (lum > 150) return 1;
  if (lum > 90) return 2;
  return 3;
}

uint8_t radarMapLevel(uint8_t r, uint8_t g, uint8_t b) {
  int lum = (r * 3 + g * 6 + b) / 10;
  int l = lum * 15 / 160;
  return (uint8_t)(l > 15 ? 15 : l);
}

void radarPutMap(RadarView& v, const TileRef& t, int x, int y, uint8_t level) {
  int X = t.sx + x, Y = t.sy + y;
  if (!v.px || X < 0 || Y < 0 || X >= RADAR_W || Y >= RADAR_H) return;
  uint8_t& p = v.px[Y * RADAR_W + X];
  p = (uint8_t)((level << 4) | (p & 0x0F));
}

void radarPutRain(RadarView& v, const TileRef& t, int x, int y, uint8_t level) {
  int X = t.sx + x, Y = t.sy + y;
  if (!v.px || X < 0 || Y < 0 || X >= RADAR_W || Y >= RADAR_H) return;
  uint8_t& p = v.px[Y * RADAR_W + X];
  p = (uint8_t)((p & 0xF0) | (level & 0x0F));
}

void radarClearRain(RadarView& v) {
  if (!v.px) return;
  for (int i = 0; i < RADAR_W * RADAR_H; i++) v.px[i] &= 0xF0;
}

bool parseRainviewer(const char* json, char* host, int hostLen, char* path, int pathLen, time_t& t) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return false;
  JsonArray past = doc["radar"]["past"];
  if (past.size() == 0) return false;
  JsonObject last = past[past.size() - 1];
  const char* h = doc["host"].as<const char*>();
  const char* p = last["path"].as<const char*>();
  snprintf(host, hostLen, "%s", h ? h : "https://tilecache.rainviewer.com");
  snprintf(path, pathLen, "%s", p ? p : "");
  t = (time_t)(last["time"] | 0L);
  return path[0] != 0;
}
