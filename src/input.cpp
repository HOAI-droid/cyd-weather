#include "input.h"
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "config.h"

// Touch-Controller hängt am zweiten SPI-Bus (VSPI)
static const int T_IRQ = 36, T_MOSI = 32, T_MISO = 39, T_CLK = 25, T_CS = 33;
static const int PIN_BOOT = 0;

static SPIClass touchSpi(VSPI);
static XPT2046_Touchscreen ts(T_CS, T_IRQ);

static bool down = false, longFired = false;
static int sx, sy, lx, ly;
static uint32_t t0 = 0, lastSeen = 0, lastActivity = 0;
static bool bootDown = false, bootLong = false;
static uint32_t bootT0 = 0;

void inputBegin() {
  touchSpi.begin(T_CLK, T_MISO, T_MOSI, T_CS);
  ts.begin(touchSpi);
  ts.setRotation(1);
  pinMode(PIN_BOOT, INPUT_PULLUP);
}

static bool readTouch(int& x, int& y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  if (p.z < 200) return false;
  if (TOUCH_DEBUG) Serial.printf("Touch roh: x=%d y=%d z=%d\n", p.x, p.y, p.z);
  x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, 320);
  y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, 240);
  if (TOUCH_FLIP_X) x = 320 - x;
  if (TOUCH_FLIP_Y) y = 240 - y;
  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);
  return true;
}

bool inputDown(int& x, int& y) {
  if (!down) return false;
  x = lx; y = ly;
  return true;
}

uint32_t inputLastActivity() { return lastActivity; }

Gesture inputPoll() {
  Gesture g{GE_NONE, 0, 0};
  uint32_t now = millis();

  // BOOT-Taste: kurz = Witz, lang = Disco
  bool b = digitalRead(PIN_BOOT) == LOW;
  if (b && !bootDown) { bootDown = true; bootLong = false; bootT0 = now; }
  if (b && bootDown && !bootLong && now - bootT0 > 1000) { bootLong = true; lastActivity = now; return {GE_BOOT_LONG, 0, 0}; }
  if (!b && bootDown) {
    bootDown = false;
    if (!bootLong && now - bootT0 > 30) { lastActivity = now; return {GE_BOOT, 0, 0}; }
  }

  int x, y;
  if (readTouch(x, y)) {
    lastSeen = now;
    lastActivity = now;
    if (!down) { down = true; longFired = false; sx = lx = x; sy = ly = y; t0 = now; }
    else { lx = x; ly = y; }
    if (!longFired && now - t0 > 1500 && abs(lx - sx) < 20 && abs(ly - sy) < 20) {
      longFired = true;
      return {GE_LONG, sx, sy};
    }
    return g;
  }
  // kurze Aussetzer des Controllers nicht als Loslassen werten
  if (down && now - lastSeen > 70) {
    down = false;
    if (longFired) return g;
    int dx = lx - sx, dy = ly - sy;
    if (abs(dx) > 50 && abs(dx) > abs(dy)) return {dx < 0 ? GE_SWIPE_LEFT : GE_SWIPE_RIGHT, sx, sy};
    return {GE_TAP, sx, sy};
  }
  return g;
}
