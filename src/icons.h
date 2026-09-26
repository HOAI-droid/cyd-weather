#pragma once
#include "gfx.h"
#include "weather.h"

void drawIcon(Gfx& g, Icon ic, float cx, float cy, float s);
void drawSun(Gfx& g, float cx, float cy, float s);
void drawCloud(Gfx& g, float cx, float cy, float s, uint16_t c);
void drawSantaHat(Gfx& g, float cx, float cy, float s);
void drawPumpkin(Gfx& g, float cx, float cy, float s);
// kleine Symbole für Kacheln
void glyphWind(Gfx& g, float cx, float cy, uint16_t c);
void glyphUmbrella(Gfx& g, float cx, float cy, uint16_t c);
void glyphArrow(Gfx& g, float cx, float cy, bool up, uint16_t c);
void glyphHeart(Gfx& g, float cx, float cy, float s, uint16_t c);
void glyphBolt(Gfx& g, float cx, float cy, float s, uint16_t c);
