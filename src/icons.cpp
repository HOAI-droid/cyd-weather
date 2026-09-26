// Wettersymbole, komplett gezeichnet (keine Bitmaps) – skalierbar und kantengeglättet
#include "icons.h"

static const float PI_F = 3.14159265f;

void drawSun(Gfx& g, float cx, float cy, float s) {
  g.disc(cx, cy, s * 0.36f, col::sunRay, 56);  // Schein
  for (int i = 0; i < 8; i++) {
    float a = i * PI_F / 4;
    g.line(cx + cosf(a) * s * 0.33f, cy + sinf(a) * s * 0.33f, cx + cosf(a) * s * 0.46f,
           cy + sinf(a) * s * 0.46f, s * 0.07f, col::sunRay);
  }
  g.disc(cx, cy, s * 0.25f, col::sunCore);
  g.disc(cx - s * 0.03f, cy - s * 0.03f, s * 0.2f, col::sunLight);
}

static void drawMoon(Gfx& g, float cx, float cy, float s) {
  float r1 = s * 0.3f, r2 = s * 0.26f, ox = cx + s * 0.16f, oy = cy - s * 0.12f;
  g.sdf(cx - r1, cy - r1, cx + r1, cy + r1, col::moon, 255, [&](float px, float py) {
    float a = sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy)) - r1;
    float b = sqrtf((px - ox) * (px - ox) + (py - oy) * (py - oy)) - r2;
    return fmaxf(a, -b);
  });
  g.disc(cx + s * 0.3f, cy + s * 0.12f, s * 0.03f, col::moon);
  g.disc(cx + s * 0.36f, cy - s * 0.3f, s * 0.025f, col::moon);
  g.disc(cx - s * 0.36f, cy - s * 0.34f, s * 0.022f, col::moon);
}

void drawCloud(Gfx& g, float cx, float cy, float s, uint16_t c) {
  g.rrect(cx - s * 0.36f, cy, s * 0.72f, s * 0.2f, s * 0.1f, c);
  g.disc(cx - s * 0.18f, cy + s * 0.03f, s * 0.16f, c);
  g.disc(cx + s * 0.04f, cy - s * 0.06f, s * 0.22f, c);
  g.disc(cx + s * 0.22f, cy + s * 0.05f, s * 0.14f, c);
}

void glyphBolt(Gfx& g, float cx, float cy, float s, uint16_t c) {
  // Blitz aus zwei Dreiecken, (cx,cy) = Mitte
  g.tri(cx + s * 0.1f, cy - s * 0.5f, cx - s * 0.28f, cy + s * 0.08f, cx + s * 0.06f, cy + s * 0.08f, c);
  g.tri(cx - s * 0.06f, cy - s * 0.08f, cx + s * 0.28f, cy - s * 0.08f, cx - s * 0.12f, cy + s * 0.5f, c);
}

static void flake(Gfx& g, float x, float y, float r, float w) {
  for (int i = 0; i < 3; i++) {
    float a = i * PI_F / 3;
    g.line(x - cosf(a) * r, y - sinf(a) * r, x + cosf(a) * r, y + sinf(a) * r, w, col::white);
  }
}

void drawIcon(Gfx& g, Icon ic, float cx, float cy, float s) {
  if (!g.rowsVisible(cy - s * 0.6f, cy + s * 0.6f)) return;
  switch (ic) {
    case IC_SUN: drawSun(g, cx, cy, s); break;
    case IC_MOON: drawMoon(g, cx, cy, s); break;
    case IC_PARTLY_DAY:
      drawSun(g, cx - s * 0.14f, cy - s * 0.14f, s * 0.7f);
      drawCloud(g, cx + s * 0.06f, cy + s * 0.02f, s * 0.85f, col::cloud);
      break;
    case IC_PARTLY_NIGHT:
      drawMoon(g, cx - s * 0.12f, cy - s * 0.12f, s * 0.7f);
      drawCloud(g, cx + s * 0.06f, cy + s * 0.04f, s * 0.85f, col::cloud);
      break;
    case IC_CLOUD:
      drawCloud(g, cx + s * 0.12f, cy - s * 0.1f, s * 0.7f, col::cloudBack);
      drawCloud(g, cx - s * 0.02f, cy, s * 0.9f, col::cloud);
      break;
    case IC_FOG:
      drawCloud(g, cx, cy - s * 0.14f, s, col::cloudFog);
      g.line(cx - s * 0.3f, cy + s * 0.22f, cx + s * 0.3f, cy + s * 0.22f, s * 0.05f, col::cloudFog);
      g.line(cx - s * 0.23f, cy + s * 0.32f, cx + s * 0.23f, cy + s * 0.32f, s * 0.05f, col::cloudFog);
      g.line(cx - s * 0.3f, cy + s * 0.42f, cx + s * 0.3f, cy + s * 0.42f, s * 0.05f, col::cloudFog);
      break;
    case IC_DRIZZLE:
      drawCloud(g, cx, cy - s * 0.14f, s, col::cloudRain);
      for (int i = -1; i <= 1; i++) {
        g.disc(cx + i * s * 0.2f, cy + s * 0.24f, s * 0.035f, col::rainLight);
        g.disc(cx + i * s * 0.2f - s * 0.05f, cy + s * 0.36f, s * 0.035f, col::rainLight);
      }
      break;
    case IC_RAIN:
      drawCloud(g, cx, cy - s * 0.14f, s, col::cloudRain);
      for (int i = -1; i <= 1; i++)
        g.line(cx + i * s * 0.2f, cy + s * 0.2f, cx + i * s * 0.2f - s * 0.06f, cy + s * 0.38f, s * 0.06f, col::rain);
      break;
    case IC_SLEET:
      drawCloud(g, cx, cy - s * 0.14f, s, col::cloudRain);
      g.line(cx - s * 0.2f, cy + s * 0.2f, cx - s * 0.26f, cy + s * 0.38f, s * 0.06f, col::rain);
      flake(g, cx, cy + s * 0.3f, s * 0.06f, s * 0.025f);
      g.line(cx + s * 0.2f, cy + s * 0.2f, cx + s * 0.14f, cy + s * 0.38f, s * 0.06f, col::rain);
      break;
    case IC_SNOW:
      drawCloud(g, cx, cy - s * 0.14f, s, col::cloud);
      flake(g, cx - s * 0.2f, cy + s * 0.26f, s * 0.06f, s * 0.025f);
      flake(g, cx + s * 0.02f, cy + s * 0.34f, s * 0.06f, s * 0.025f);
      flake(g, cx + s * 0.22f, cy + s * 0.26f, s * 0.06f, s * 0.025f);
      break;
    case IC_THUNDER:
      drawCloud(g, cx, cy - s * 0.14f, s, col::cloudDark);
      glyphBolt(g, cx + s * 0.01f, cy + s * 0.24f, s * 0.38f, col::bolt);
      break;
  }
}

void drawSantaHat(Gfx& g, float cx, float cy, float s) {
  // sitzt schräg oben rechts auf dem Symbol
  float x = cx + s * 0.02f, y = cy - s * 0.3f;
  g.tri(x - s * 0.2f, y + s * 0.02f, x + s * 0.18f, y + s * 0.02f, x + s * 0.26f, y - s * 0.3f, rgb(220, 38, 38));
  g.rrect(x - s * 0.24f, y - s * 0.02f, s * 0.46f, s * 0.09f, s * 0.045f, col::white);
  g.disc(x + s * 0.27f, y - s * 0.31f, s * 0.06f, col::white);
}

void drawPumpkin(Gfx& g, float cx, float cy, float s) {
  uint16_t o = rgb(255, 140, 26), d = rgb(214, 100, 10);
  g.ellipse(cx - s * 0.18f, cy, s * 0.24f, s * 0.36f, d);
  g.ellipse(cx + s * 0.18f, cy, s * 0.24f, s * 0.36f, d);
  g.ellipse(cx, cy, s * 0.28f, s * 0.4f, o);
  g.line(cx, cy - s * 0.38f, cx + s * 0.06f, cy - s * 0.52f, s * 0.08f, rgb(80, 140, 60));
  g.tri(cx - s * 0.2f, cy - s * 0.08f, cx - s * 0.06f, cy - s * 0.08f, cx - s * 0.13f, cy - s * 0.2f, rgb(40, 20, 0));
  g.tri(cx + s * 0.06f, cy - s * 0.08f, cx + s * 0.2f, cy - s * 0.08f, cx + s * 0.13f, cy - s * 0.2f, rgb(40, 20, 0));
  g.arc(cx, cy, s * 0.17f, s * 0.06f, 0.4f, 2.7f, rgb(40, 20, 0));
}

void glyphWind(Gfx& g, float cx, float cy, uint16_t c) {
  g.line(cx - 8, cy - 4, cx + 5, cy - 4, 2, c);
  g.arc(cx + 5, cy - 6.5f, 2.5f, 2, -1.6f, 1.6f, c);
  g.line(cx - 8, cy + 1, cx + 8, cy + 1, 2, c);
  g.line(cx - 4, cy + 6, cx + 3, cy + 6, 2, c);
  g.arc(cx + 3, cy + 8.5f, 2.5f, 2, -1.6f, 1.6f, c);
}

void glyphUmbrella(Gfx& g, float cx, float cy, uint16_t c) {
  g.sdf(cx - 9, cy - 9, cx + 9, cy + 1, c, 255, [&](float px, float py) {
    float d = sqrtf((px - cx) * (px - cx) + (py - cy) * (py - cy)) - 8.5f;
    return fmaxf(d, py - cy);
  });
  g.line(cx, cy, cx, cy + 7, 1.8f, c);
  g.arc(cx - 2, cy + 7, 2, 1.8f, 0, 3.1416f, c);
}

void glyphArrow(Gfx& g, float cx, float cy, bool up, uint16_t c) {
  if (up) g.tri(cx, cy - 4, cx + 4.5f, cy + 3, cx - 4.5f, cy + 3, c);
  else g.tri(cx, cy + 4, cx + 4.5f, cy - 3, cx - 4.5f, cy - 3, c);
}

void glyphHeart(Gfx& g, float cx, float cy, float s, uint16_t c) {
  g.disc(cx - s * 0.25f, cy - s * 0.1f, s * 0.28f, c);
  g.disc(cx + s * 0.25f, cy - s * 0.1f, s * 0.28f, c);
  g.tri(cx - s * 0.52f, cy, cx + s * 0.52f, cy, cx, cy + s * 0.55f, c);
}
