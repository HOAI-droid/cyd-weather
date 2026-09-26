#include "TFT_eSPI.h"
#include "../../src/gfx.h"

uint16_t TFT_eSprite::utf8(const char*& s) {
  uint8_t c = *s++;
  if (c < 0x80) return c;
  if ((c & 0xE0) == 0xC0) { uint16_t r = ((c & 0x1F) << 6) | (*s++ & 0x3F); return r; }
  if ((c & 0xF0) == 0xE0) { uint16_t r = ((c & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F); s += 2; return r; }
  return '?';
}

bool TFT_eSprite::glyph(uint16_t code, int& idx, const uint8_t*& bmp) {
  int n = be(font);
  const uint8_t* bm = font + 24 + n * 28;
  for (int i = 0; i < n; i++) {
    const uint8_t* m = font + 24 + i * 28;
    if (be(m) == code) { idx = i; bmp = bm; return true; }
    bm += be(m + 4) * be(m + 8);
  }
  return false;
}

int16_t TFT_eSprite::textWidth(const char* s) {
  int asc = be(font + 16), desc = be(font + 20), space = (asc + desc) * 2 / 7, w = 0;
  while (*s) {
    uint16_t c = utf8(s);
    if (c == 0x20) { w += space; continue; }
    int i; const uint8_t* b;
    if (!glyph(c, i, b)) { w += space + 1; continue; }
    const uint8_t* m = font + 24 + i * 28;
    if (w == 0 && be(m + 20) < 0) w -= be(m + 20);
    w += *s ? be(m + 12) : be(m + 20) + be(m + 8);
  }
  return w;
}

void TFT_eSprite::drawString(const char* s, int x, int y) {
  int asc = be(font + 16), desc = be(font + 20), space = (asc + desc) * 2 / 7;
  int cx = x;
  bool first = true;
  while (*s) {
    uint16_t c = utf8(s);
    if (c == 0x20) { cx += space; first = false; continue; }
    int i; const uint8_t* b;
    if (!glyph(c, i, b)) { cx += space + 1; continue; }
    const uint8_t* m = font + 24 + i * 28;
    int h = be(m + 4), w = be(m + 8), adv = be(m + 12), dy = be(m + 16), dx = be(m + 20);
    if (first && dx < 0) cx -= dx;  // wie TFT_eSPI (cursor_x == 0 Sonderfall ähnelt dem)
    first = false;
    int gy = y + asc - dy, gx = cx + dx;
    for (int yy = 0; yy < h; yy++)
      for (int xx = 0; xx < w; xx++) {
        uint8_t a = b[yy * w + xx];
        if (a) drawPixel(gx + xx, gy + yy, a == 255 ? fg : blend565(fg, readPixel(gx + xx, gy + yy), a));
      }
    cx += adv;
  }
}
