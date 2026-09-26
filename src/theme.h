// Farben und Layout-Konstanten
#pragma once
#include <stdint.h>

constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

constexpr int SCREEN_W = 320;
constexpr int SCREEN_H = 240;

namespace col {
constexpr uint16_t white     = rgb(255, 255, 255);
constexpr uint16_t black     = rgb(0, 0, 0);
constexpr uint16_t tagline   = rgb(255, 224, 138);
constexpr uint16_t sunCore   = rgb(255, 182, 39);
constexpr uint16_t sunLight  = rgb(255, 207, 77);
constexpr uint16_t sunRay    = rgb(255, 197, 61);
constexpr uint16_t moon      = rgb(232, 237, 255);
constexpr uint16_t cloud     = rgb(241, 245, 251);
constexpr uint16_t cloudRain = rgb(213, 220, 232);
constexpr uint16_t cloudBack = rgb(174, 185, 204);
constexpr uint16_t cloudDark = rgb(140, 151, 173);
constexpr uint16_t cloudFog  = rgb(201, 209, 221);
constexpr uint16_t rain      = rgb(91, 179, 255);
constexpr uint16_t rainLight = rgb(140, 203, 255);
constexpr uint16_t bolt      = rgb(255, 210, 63);
constexpr uint16_t warm      = rgb(255, 179, 107);
constexpr uint16_t line      = rgb(255, 196, 90);
constexpr uint16_t leet      = rgb(57, 255, 136);
constexpr uint16_t frog      = rgb(76, 199, 107);
constexpr uint16_t frogBelly = rgb(155, 227, 168);
constexpr uint16_t frogDark  = rgb(30, 107, 51);
constexpr uint16_t wood      = rgb(201, 139, 75);
constexpr uint16_t ink       = rgb(24, 34, 47);
constexpr uint16_t heart     = rgb(255, 107, 129);
constexpr uint16_t matrix    = rgb(0, 255, 90);
constexpr uint16_t alert     = rgb(255, 210, 63);

// ---- Design "Instrument": Graphit, warmes Weiß, Champagner-Gold ----
constexpr uint16_t text      = rgb(236, 232, 225);  // Haupttext
constexpr uint16_t muted     = rgb(138, 143, 152);  // Beschriftungen
constexpr uint16_t gold      = rgb(216, 188, 138);  // Akzent
constexpr uint16_t goldDim   = rgb(120, 106, 82);
constexpr uint16_t teal      = rgb(111, 195, 201);  // Regen, Feuchte
constexpr uint16_t tealDark  = rgb(62, 142, 143);
constexpr uint16_t coral     = rgb(224, 122, 95);   // Starkregen, Warnung
constexpr uint16_t panel     = rgb(24, 27, 32);     // Kartenfläche
constexpr uint16_t panelEdge = rgb(46, 51, 59);     // Kartenrand
constexpr uint16_t tick      = rgb(58, 63, 72);     // Skalenstriche
constexpr uint16_t tickMajor = rgb(106, 112, 122);
constexpr uint16_t mapLow    = rgb(13, 15, 19);     // Radar-Karte dunkel
constexpr uint16_t mapHigh   = rgb(176, 181, 190);  // Radar-Karte hell
}  // namespace col

// Transparenzen für Sekundärtext (0..255, Weiß über Hintergrund bzw. Karte)
constexpr uint8_t A_SECONDARY = 150;
