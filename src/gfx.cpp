#include "gfx.h"
#include <stdio.h>
#include <string.h>

uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t a) {
  if (a == 255) return fg;
  if (a == 0) return bg;
  uint32_t fr = fg >> 11, fgg = (fg >> 5) & 0x3F, fb = fg & 0x1F;
  uint32_t br = bg >> 11, bgg = (bg >> 5) & 0x3F, bb = bg & 0x1F;
  uint32_t r = (fr * a + br * (255 - a) + 127) / 255;
  uint32_t g = (fgg * a + bgg * (255 - a) + 127) / 255;
  uint32_t b = (fb * a + bb * (255 - a) + 127) / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

uint16_t lerp565(uint16_t a, uint16_t b, float t) {
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  return blend565(b, a, (uint8_t)(t * 255));
}

bool Gfx::begin() {
  spr.setColorDepth(16);
  if (spr.createSprite(SCREEN_W, BAND_H) == nullptr) return false;
  spr.setTextWrap(false, false);  // sonst bricht TFT_eSPI Text am Streifenrand um
  return true;
}

void Gfx::plot(int x, int y, uint16_t c, uint8_t a) {
  int ly = y - oy;
  if (x < 0 || x >= SCREEN_W || ly < 0 || ly >= BAND_H || a == 0) return;
  if (a == 255) spr.drawPixel(x, ly, c);
  else spr.drawPixel(x, ly, blend565(c, spr.readPixel(x, ly), a));
}

// ---------------------------------------------------------------- Flächen
void Gfx::background(uint16_t top, uint16_t bottom) {
  bgTop = top;
  bgBot = bottom;
  for (int y = oy; y < oy + BAND_H; y++) spr.drawFastHLine(0, y - oy, SCREEN_W, bgAt(y));
}

uint16_t Gfx::bgAt(int y) const { return lerp565(bgTop, bgBot, y / (float)(SCREEN_H - 1)); }

uint16_t Gfx::dim(uint8_t alpha, int y, bool onGlass) const {
  uint16_t base = onGlass ? col::panel : bgAt(y);
  return blend565(col::text, base, alpha);
}

void Gfx::fillRect(int x, int y, int w, int h, uint16_t c, uint8_t a) {
  int y0 = y < oy ? oy : y, y1 = y + h > oy + BAND_H ? oy + BAND_H : y + h;
  if (y1 <= y0) return;
  if (a == 255) { spr.fillRect(x, y0 - oy, w, y1 - y0, c); return; }
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x; xx < x + w; xx++) plot(xx, yy, c, a);
}

void Gfx::vgrad(int x, int y, int w, int h, uint16_t c1, uint16_t c2) {
  for (int yy = y; yy < y + h; yy++) {
    if (yy < oy || yy >= oy + BAND_H) continue;
    spr.drawFastHLine(x, yy - oy, w, lerp565(c1, c2, (yy - y) / (float)(h > 1 ? h - 1 : 1)));
  }
}

// Karte im Instrument-Stil: dunkle Fläche mit feinem Rand
void Gfx::glass(int x, int y, int w, int h, int r) {
  if (!rowsVisible(y, y + h)) return;
  rrect(x, y, w, h, r, col::panelEdge);
  rrect(x + 1, y + 1, w - 2, h - 2, r - 1, col::panel);
}

// ---------------------------------------------------------------- Formen
static inline float len2(float x, float y) { return sqrtf(x * x + y * y); }

void Gfx::disc(float cx, float cy, float r, uint16_t c, uint8_t a) {
  if (!rowsVisible(cy - r - 1, cy + r + 1)) return;
  sdf(cx - r, cy - r, cx + r, cy + r, c, a, [&](float px, float py) { return len2(px - cx, py - cy) - r; });
}

void Gfx::ring(float cx, float cy, float r, float w, uint16_t c, uint8_t a) {
  float o = r + w / 2;
  if (!rowsVisible(cy - o - 1, cy + o + 1)) return;
  sdf(cx - o, cy - o, cx + o, cy + o, c, a,
      [&](float px, float py) { return fabsf(len2(px - cx, py - cy) - r) - w / 2; });
}

static float segDist(float px, float py, float x0, float y0, float x1, float y1) {
  float dx = x1 - x0, dy = y1 - y0, l = dx * dx + dy * dy;
  float t = l > 0 ? ((px - x0) * dx + (py - y0) * dy) / l : 0;
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  return len2(px - (x0 + t * dx), py - (y0 + t * dy));
}

void Gfx::line(float x0, float y0, float x1, float y1, float w, uint16_t c, uint8_t a) {
  float h = w / 2;
  float minx = fminf(x0, x1) - h, maxx = fmaxf(x0, x1) + h;
  float miny = fminf(y0, y1) - h, maxy = fmaxf(y0, y1) + h;
  if (!rowsVisible(miny - 1, maxy + 1)) return;
  sdf(minx, miny, maxx, maxy, c, a, [&](float px, float py) { return segDist(px, py, x0, y0, x1, y1) - h; });
}

void Gfx::rrect(float x, float y, float w, float h, float r, uint16_t c, uint8_t a) {
  if (!rowsVisible(y - 1, y + h + 1)) return;
  float cx = x + w / 2, cy = y + h / 2, hx = w / 2 - r, hy = h / 2 - r;
  sdf(x, y, x + w, y + h, c, a, [&](float px, float py) {
    float qx = fabsf(px - cx) - hx, qy = fabsf(py - cy) - hy;
    float ox = qx > 0 ? qx : 0, oy_ = qy > 0 ? qy : 0;
    float in = fmaxf(qx, qy);
    return len2(ox, oy_) + (in < 0 ? in : 0) - r;
  });
}

void Gfx::ellipse(float cx, float cy, float rx, float ry, uint16_t c, uint8_t a) {
  if (!rowsVisible(cy - ry - 1, cy + ry + 1)) return;
  float m = fminf(rx, ry);
  sdf(cx - rx, cy - ry, cx + rx, cy + ry, c, a,
      [&](float px, float py) { return (len2((px - cx) / rx, (py - cy) / ry) - 1.0f) * m; });
}

void Gfx::tri(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c, uint8_t a) {
  float miny = fminf(y0, fminf(y1, y2)), maxy = fmaxf(y0, fmaxf(y1, y2));
  if (!rowsVisible(miny - 1, maxy + 1)) return;
  float minx = fminf(x0, fminf(x1, x2)), maxx = fmaxf(x0, fmaxf(x1, x2));
  // Orientierung, damit "innen" immer negativ ist
  float area = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
  sdf(minx, miny, maxx, maxy, c, a, [&](float px, float py) {
    float d = fminf(segDist(px, py, x0, y0, x1, y1), fminf(segDist(px, py, x1, y1, x2, y2), segDist(px, py, x2, y2, x0, y0)));
    float e0 = (x1 - x0) * (py - y0) - (y1 - y0) * (px - x0);
    float e1 = (x2 - x1) * (py - y1) - (y2 - y1) * (px - x1);
    float e2 = (x0 - x2) * (py - y2) - (y0 - y2) * (px - x2);
    bool inside = area >= 0 ? (e0 >= 0 && e1 >= 0 && e2 >= 0) : (e0 <= 0 && e1 <= 0 && e2 <= 0);
    return inside ? -d : d;
  });
}

// Winkel im Bogenmaß, 0 = rechts, im Uhrzeigersinn (Bildschirm-y zeigt nach unten)
void Gfx::arc(float cx, float cy, float r, float w, float a0, float a1, uint16_t c, uint8_t a) {
  float o = r + w / 2;
  if (!rowsVisible(cy - o - 1, cy + o + 1)) return;
  const float TWO_PI_F = 6.2831853f;
  float span = a1 - a0;
  float ex0 = cx + cosf(a0) * r, ey0 = cy + sinf(a0) * r;
  float ex1 = cx + cosf(a1) * r, ey1 = cy + sinf(a1) * r;
  sdf(cx - o, cy - o, cx + o, cy + o, c, a, [&](float px, float py) {
    float ang = atan2f(py - cy, px - cx) - a0;
    while (ang < 0) ang += TWO_PI_F;
    while (ang >= TWO_PI_F) ang -= TWO_PI_F;
    if (ang <= span) return fabsf(len2(px - cx, py - cy) - r) - w / 2;
    return fminf(len2(px - ex0, py - ey0), len2(px - ex1, py - ey1)) - w / 2;
  });
}

void Gfx::drop(float cx, float cy, float r, uint16_t c) {
  disc(cx, cy + r * 0.2f, r, c);
  tri(cx, cy - r * 1.7f, cx + r * 0.93f, cy - r * 0.15f, cx - r * 0.93f, cy - r * 0.15f, c);
}

// ---------------------------------------------------------------- Text
static int32_t be32(const uint8_t* p) {
  return (int32_t)((uint32_t)pgm_read_byte(p) << 24 | (uint32_t)pgm_read_byte(p + 1) << 16 |
                   (uint32_t)pgm_read_byte(p + 2) << 8 | pgm_read_byte(p + 3));
}

int Gfx::ascent(const uint8_t* font) const { return be32(font + 16); }

void Gfx::useFont(const uint8_t* font) {
  if (curFont == font) return;
  if (curFont) spr.unloadFont();
  spr.loadFont(font);
  curFont = font;
}

int Gfx::textWidth(const char* s, const uint8_t* font) {
  useFont(font);
  return spr.textWidth(s);
}

void Gfx::text(const char* s, int x, int y, const uint8_t* font, uint16_t c, Align al, VAlign va) {
  int asc = ascent(font);
  int top = y;
  if (va == VA_MID) top = y - asc / 2;
  else if (va == VA_BASE) top = y - asc;
  int h = asc + be32(font + 20) + 4;
  if (!rowsVisible(top - asc / 3, top + h)) return;  // Umlaute ragen etwas nach oben
  useFont(font);
  int w = (al == AL_L) ? 0 : spr.textWidth(s);
  if (al == AL_C) x -= w / 2;
  else if (al == AL_R) x -= w;
  spr.setTextDatum(TL_DATUM);
  spr.setTextColor(c);  // gleiche Vorder-/Hintergrundfarbe => Kantenglättung gegen den Sprite-Inhalt
  spr.drawString(s, x, top - oy);
}

int Gfx::wrap(const char* s, int x, int y, int w, const uint8_t* font, uint16_t c, int lineH, Align al, bool draw) {
  char line[96] = "";
  char test[96];
  int lines = 0;
  const char* p = s;
  while (*p) {
    while (*p == ' ') p++;
    const char* e = p;
    while (*e && *e != ' ' && *e != '\n') e++;
    char word[48];
    int wl = (int)(e - p) < 47 ? (int)(e - p) : 47;
    memcpy(word, p, wl);
    word[wl] = 0;
    snprintf(test, sizeof(test), "%s%s%s", line, line[0] ? " " : "", word);
    if (line[0] && textWidth(test, font) > w) {
      if (draw) text(line, al == AL_C ? x + w / 2 : (al == AL_R ? x + w : x), y + lines * lineH, font, c, al);
      lines++;
      snprintf(line, sizeof(line), "%s", word);
    } else {
      snprintf(line, sizeof(line), "%s", test);
    }
    p = e;
    if (*p == '\n') {
      if (draw) text(line, al == AL_C ? x + w / 2 : (al == AL_R ? x + w : x), y + lines * lineH, font, c, al);
      lines++;
      line[0] = 0;
      p++;
    }
  }
  if (line[0]) {
    if (draw) text(line, al == AL_C ? x + w / 2 : (al == AL_R ? x + w : x), y + lines * lineH, font, c, al);
    lines++;
  }
  return lines;
}
