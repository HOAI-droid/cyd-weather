// Grafik-Schicht: rendert jeden Bildschirm in horizontalen Streifen in einen
// kleinen Sprite (flimmerfrei, wenig RAM) und bietet kantengeglättete Formen.
#pragma once
#include <TFT_eSPI.h>
#include <math.h>
#include "theme.h"

enum Align : uint8_t { AL_L, AL_C, AL_R };
enum VAlign : uint8_t { VA_TOP, VA_MID, VA_BASE };

uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t alpha);
uint16_t lerp565(uint16_t a, uint16_t b, float t);

class Gfx {
 public:
  static constexpr int BAND_H = 60;  // 4 Streifen à 320x60 px = 38 KB RAM

  explicit Gfx(TFT_eSPI* tft) : spr(tft) {}
  bool begin();

  // Zeichnet einen kompletten Frame: draw() wird pro Streifen aufgerufen.
  template <typename F>
  void frame(F&& draw) {
    for (oy = 0; oy < SCREEN_H; oy += BAND_H) {
      draw();
      spr.pushSprite(0, oy);
    }
    oy = 0;
  }

  // ---- Hintergrund & Flächen ----
  void background(uint16_t top, uint16_t bottom);           // Verlauf über den ganzen Schirm
  uint16_t bgAt(int y) const;                                // Hintergrundfarbe in Zeile y
  uint16_t dim(uint8_t alpha, int y, bool onGlass = false) const;  // gedämpftes Weiß
  void fillRect(int x, int y, int w, int h, uint16_t c, uint8_t a = 255);
  void vgrad(int x, int y, int w, int h, uint16_t c1, uint16_t c2);
  void glass(int x, int y, int w, int h, int r = 12);

  // ---- kantengeglättete Formen (Koordinaten in Bildschirmpixeln) ----
  void disc(float cx, float cy, float r, uint16_t c, uint8_t a = 255);
  void ring(float cx, float cy, float r, float w, uint16_t c, uint8_t a = 255);
  void line(float x0, float y0, float x1, float y1, float w, uint16_t c, uint8_t a = 255);
  void rrect(float x, float y, float w, float h, float r, uint16_t c, uint8_t a = 255);
  void ellipse(float cx, float cy, float rx, float ry, uint16_t c, uint8_t a = 255);
  void tri(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c, uint8_t a = 255);
  void arc(float cx, float cy, float r, float w, float a0, float a1, uint16_t c, uint8_t a = 255);
  void drop(float cx, float cy, float r, uint16_t c);        // Wassertropfen

  // Beliebige Form über eine Distanzfunktion (negativ = innen)
  template <typename D>
  void sdf(float bx0, float by0, float bx1, float by1, uint16_t c, uint8_t a, D dist) {
    int x0 = (int)floorf(bx0) - 1, x1 = (int)ceilf(bx1) + 1;
    int y0 = (int)floorf(by0) - 1, y1 = (int)ceilf(by1) + 1;
    if (x0 < 0) x0 = 0;
    if (x1 > SCREEN_W - 1) x1 = SCREEN_W - 1;
    if (y0 < oy) y0 = oy;
    if (y1 > oy + BAND_H - 1) y1 = oy + BAND_H - 1;
    for (int y = y0; y <= y1; y++) {
      float py = y + 0.5f;
      for (int x = x0; x <= x1; x++) {
        float d = dist(x + 0.5f, py);
        if (d >= 0.5f) continue;
        uint8_t aa = d <= -0.5f ? a : (uint8_t)((0.5f - d) * a);
        plot(x, y, c, aa);
      }
    }
  }

  // ---- Text (UTF-8, Umlaute) ----
  void text(const char* s, int x, int y, const uint8_t* font, uint16_t c,
            Align al = AL_L, VAlign va = VA_TOP);
  int textWidth(const char* s, const uint8_t* font);
  int ascent(const uint8_t* font) const;
  // Bricht Text in Zeilen um; gibt die Anzahl der Zeilen zurück
  int wrap(const char* s, int x, int y, int w, const uint8_t* font, uint16_t c,
           int lineH, Align al = AL_L, bool draw = true);

  void plot(int x, int y, uint16_t c, uint8_t a);  // y in Bildschirmkoordinaten
  bool rowsVisible(int y0, int y1) const { return y1 >= oy && y0 < oy + BAND_H; }

  int oy = 0;
  TFT_eSprite spr;

 private:
  void useFont(const uint8_t* font);
  const uint8_t* curFont = nullptr;
  uint16_t bgTop = 0, bgBot = 0;
};
