#pragma once
#include "gfx.h"

void uiDrawScreen(Gfx& g);                       // aktuelle Ansicht (ein Streifen)
void uiDrawBoot(Gfx& g, const char* status, int step);
bool uiIsLeetTime();

// Touch-Zonen auf "Jetzt"
bool uiHitIcon(int x, int y);
bool uiHitClock(int x, int y);
