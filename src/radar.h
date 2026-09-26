// Regenradar: Kartenkacheln (CARTO/OpenStreetMap) + Radarkacheln (RainViewer),
// beide im Web-Mercator-Raster, deshalb passen sie Pixel für Pixel übereinander.
#pragma once
#include <stdint.h>
#include <time.h>

constexpr int RADAR_ZOOM = 7;   // 1 Pixel ≈ 0,75 km in Mitteleuropa
constexpr int RADAR_W = 320, RADAR_H = 240;
constexpr int RADAR_MAX_TILES = 6;

struct TileRef {
  int x, y;    // Kachelnummer
  int sx, sy;  // Bildschirmposition der linken oberen Kachelecke
};

// Alle Kacheln, die den Bildschirm rund um lat/lon abdecken (Ort in der Mitte)
int radarTiles(float lat, float lon, int zoom, TileRef* out);
float radarMetersPerPixel(float lat, int zoom);

struct RadarView {
  // Ein Byte je Pixel: obere 4 Bit = Karte (Helligkeit 0..15), untere 4 Bit = Regenstufe 0..5
  uint8_t* px = nullptr;
  bool mapOk = false, rainOk = false;
  float mapLat = 0, mapLon = 0;  // für welchen Ort die Karte geladen ist
  time_t frameTime = 0;          // Zeitpunkt des Radarbilds
  uint32_t fetchedMs = 0;
  char status[48] = "";          // Hinweis, falls etwas fehlt
};

// Farbe einer Radarkachel -> Regenstufe 0 (nichts) bis 5 (Unwetter)
uint8_t radarRainLevel(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
// Farbe einer (dunklen) Kartenkachel -> Helligkeitsstufe 0..15
uint8_t radarMapLevel(uint8_t r, uint8_t g, uint8_t b);

// Setzt Karten- bzw. Regenwert eines Kachelpixels, falls er auf dem Bildschirm liegt
void radarPutMap(RadarView& v, const TileRef& t, int x, int y, uint8_t level);
void radarPutRain(RadarView& v, const TileRef& t, int x, int y, uint8_t level);
void radarClearRain(RadarView& v);

// weather-maps.json von RainViewer: Host und Pfad des neuesten Radarbilds
bool parseRainviewer(const char* json, char* host, int hostLen, char* path, int pathLen, time_t& t);
