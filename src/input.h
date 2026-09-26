// Touch (XPT2046) und BOOT-Taste -> Gesten
#pragma once
#include <stdint.h>

enum GestureType : uint8_t { GE_NONE, GE_TAP, GE_SWIPE_LEFT, GE_SWIPE_RIGHT, GE_LONG, GE_BOOT, GE_BOOT_LONG };

struct Gesture { GestureType type; int x, y; };

void inputBegin();
Gesture inputPoll();          // einmal pro loop() aufrufen
bool inputDown(int& x, int& y);  // aktueller Finger (für das Spiel)
uint32_t inputLastActivity();
