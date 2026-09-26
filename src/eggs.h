// Easter Eggs
#pragma once
#include "gfx.h"

enum EggMode : uint8_t {
  EGG_NONE, EGG_FROG, EGG_GAME, EGG_MATRIX, EGG_APRIL, EGG_JOKE, EGG_DISCO, EGG_DARK, EGG_PARTY
};

struct TouchNow { bool down; int x, y; };

void eggStart(EggMode m);
EggMode eggMode();
// Aktualisiert und zeichnet das aktive Ei; gibt false zurück, wenn es beendet ist
bool eggLoop(Gfx& g, const TouchNow& t);
void eggTap(int x, int y);

// Auslöser, die main.cpp füttert
void eggHomeTap(int x, int y);      // zählt Tipps aufs Wettersymbol & Ecken-Code
bool eggCornerTap(int x, int y);    // true, wenn der Tipp in einer Ecke lag
void eggCheckCalendar();            // 1. April, Silvester, Geburtstag
void eggCheckLight();               // Lichtsensor
