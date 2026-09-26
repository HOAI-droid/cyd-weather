// Minimaler Nachbau von TFT_eSPI für den Host-Simulator (nur was die Firmware nutzt)
#pragma once
#include <stdint.h>
#include <string.h>
#include <vector>
#include "pgmspace.h"

#define TL_DATUM 0
#define TFT_BLACK 0x0000
#define TFT_WHITE 0xFFFF

class TFT_eSPI {
 public:
  uint16_t fb[240][320];
  TFT_eSPI() { memset(fb, 0, sizeof(fb)); }
};

class TFT_eSprite {
 public:
  explicit TFT_eSprite(TFT_eSPI* t) : tft(t) {}
  void setColorDepth(int) {}
  void* createSprite(int w, int h) { W = w; H = h; buf.assign(w * h, 0); return buf.data(); }
  void fillSprite(uint16_t c) { std::fill(buf.begin(), buf.end(), c); }
  void drawPixel(int x, int y, uint16_t c) { if (x >= 0 && y >= 0 && x < W && y < H) buf[y * W + x] = c; }
  uint16_t readPixel(int x, int y) { return (x >= 0 && y >= 0 && x < W && y < H) ? buf[y * W + x] : 0; }
  void drawFastHLine(int x, int y, int w, uint16_t c) { for (int i = 0; i < w; i++) drawPixel(x + i, y, c); }
  void fillRect(int x, int y, int w, int h, uint16_t c) { for (int j = 0; j < h; j++) drawFastHLine(x, y + j, w, c); }
  void pushSprite(int x, int y) {
    for (int j = 0; j < H; j++) for (int i = 0; i < W; i++)
      if (y + j < 240 && x + i < 320) tft->fb[y + j][x + i] = buf[j * W + i];
  }
  void loadFont(const uint8_t* f) { font = f; loads++; }
  void unloadFont() { font = nullptr; }
  void setTextDatum(int) {}
  void setTextWrap(bool, bool) {}
  void setTextColor(uint16_t c) { fg = c; }
  int16_t textWidth(const char* s);
  void drawString(const char* s, int x, int y);
  int loads = 0;

 private:
  static int32_t be(const uint8_t* p) { return (int32_t)((uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]); }
  bool glyph(uint16_t code, int& idx, const uint8_t*& bmp);
  static uint16_t utf8(const char*& s);
  TFT_eSPI* tft;
  int W = 0, H = 0;
  std::vector<uint16_t> buf;
  const uint8_t* font = nullptr;
  uint16_t fg = 0xFFFF;
};
