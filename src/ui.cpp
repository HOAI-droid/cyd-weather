#include "ui.h"
#include <stdio.h>
#include "app.h"
#include "config.h"
#include "fonts/font_b.h"
#include "fonts/font_c.h"
#include "fonts/font_h.h"
#include "fonts/font_m.h"
#include "fonts/font_s.h"
#include "icons.h"

static const float PI_F = 3.14159265f;
static const char* const WD_LONG[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
static const char* const WD_SHORT[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
static const char* const MONTHS[] = {"Januar", "Februar", "März", "April", "Mai", "Juni", "Juli",
                                     "August", "September", "Oktober", "November", "Dezember"};

static int iround(float v) { return (int)lroundf(v); }

bool uiIsLeetTime() { return app.timeValid && app.now.tm_hour == 13 && app.now.tm_min == 37; }
bool uiHitIcon(int x, int y) { return x > 14 && x < 110 && y > 48 && y < 140; }
bool uiHitClock(int x, int y) { return x > 210 && y < 48; }

static void bg(Gfx& g) {
  uint16_t t, b;
  if (app.wx.valid) themeColors(app.wx.code, app.wx.isDay, t, b);
  else { t = rgb(24, 38, 66); b = rgb(44, 64, 102); }
  g.background(t, b);
}

static void clockStr(char* buf, int n) {
  if (app.timeValid) snprintf(buf, n, "%02d:%02d", app.now.tm_hour, app.now.tm_min);
  else snprintf(buf, n, "--:--");
}

static void header(Gfx& g, const char* title) {
  char clk[8];
  clockStr(clk, sizeof(clk));
  g.text(title, 14, 10, font_b, col::white);
  g.text(clk, 306, 10, font_b, col::white, AL_R);
}

static void pageDots(Gfx& g) {
  if (!g.rowsVisible(226, 238)) return;
  int n = SCR_COUNT, tw = n * 6 + (n - 1) * 6 + 10;
  float x = 160 - tw / 2.0f;
  for (int i = 0; i < n; i++) {
    if (i == app.screen) { g.rrect(x, 229, 16, 6, 3, col::white); x += 22; }
    else { g.disc(x + 3, 232, 3, col::white, 100); x += 12; }
  }
}

// ------------------------------------------------------------------ Jetzt
static void drawHome(Gfx& g) {
  const WeatherData& w = app.wx;
  char buf[48];
  bg(g);
  const struct tm& t = app.now;
  bool halloween = app.timeValid && t.tm_mon == 9 && t.tm_mday == 31;
  bool xmas = app.timeValid && t.tm_mon == 11 && t.tm_mday >= 24 && t.tm_mday <= 26;

  g.text(CITY_NAME, 14, 10, font_b, col::white);
  if (halloween) drawPumpkin(g, 14 + g.textWidth(CITY_NAME, font_b) + 14, 17, 16);
  if (app.timeValid) snprintf(buf, sizeof(buf), "%s, %d. %s%s", WD_LONG[t.tm_wday], t.tm_mday, MONTHS[t.tm_mon], app.offline ? " · offline" : "");
  else snprintf(buf, sizeof(buf), "Zeit wird geladen …");
  g.text(buf, 14, 31, font_s, g.dim(A_SECONDARY, 36));

  clockStr(buf, sizeof(buf));
  if (uiIsLeetTime()) {
    int j = (app.glitch % 3) - 1;
    g.text(buf, 306 + j * 2, 8, font_c, col::leet, AL_R);
    g.fillRect(232, 20 + (app.glitch % 8), 40, 2, col::leet, 120);
    g.fillRect(258, 28 - (app.glitch % 5), 30, 1, col::leet, 160);
    g.text("LEET TIME", 306, 40, font_s, col::leet, AL_R);
  } else {
    g.text(buf, 306, 8, font_c, col::white, AL_R);
  }

  if (!w.valid) {
    g.text("Lade Wetterdaten …", 160, 120, font_m, col::white, AL_C, VA_MID);
    pageDots(g);
    return;
  }

  drawIcon(g, codeIcon(w.code, w.isDay), 62, 94, 84);
  if (xmas) drawSantaHat(g, 62, 94, 84);

  snprintf(buf, sizeof(buf), "%d°", iround(w.temp));
  g.text(buf, 116, 50, font_h, col::white);

  const DayData& d0 = w.days[0];
  glyphArrow(g, 262, 60, true, col::warm);
  snprintf(buf, sizeof(buf), "%d°", iround(d0.tmax));
  g.text(buf, 306, 53, font_b, col::white, AL_R);
  glyphArrow(g, 262, 80, false, col::rainLight);
  snprintf(buf, sizeof(buf), "%d°", iround(d0.tmin));
  g.text(buf, 306, 73, font_b, col::white, AL_R);
  snprintf(buf, sizeof(buf), "Gefühlt %d°", iround(w.feels));
  g.text(buf, 306, 94, font_s, g.dim(A_SECONDARY, 100), AL_R);

  g.text(codeText(w.code, w.isDay), 118, 114, font_m, col::white);
  g.text(w.tagline, 160, 139, font_s, col::tagline, AL_C);

  // Kacheln
  for (int i = 0; i < 4; i++) {
    int x = 10 + i * 77, y = 158, cx = x + 35;
    g.glass(x, y, 70, 66, 10);
    const char* label = "";
    switch (i) {
      case 0: g.drop(cx, y + 17, 4.5f, col::rainLight); snprintf(buf, sizeof(buf), "%d%%", iround(w.humidity)); label = "Feuchte"; break;
      case 1: glyphWind(g, cx, y + 15, rgb(216, 230, 247)); snprintf(buf, sizeof(buf), "%d km/h", iround(w.wind)); label = "Wind"; break;
      case 2: glyphUmbrella(g, cx, y + 16, col::rainLight); snprintf(buf, sizeof(buf), "%d%%", d0.pop); label = "Regen"; break;
      case 3: drawSun(g, cx, y + 15, 20); snprintf(buf, sizeof(buf), "%d", iround(d0.uv)); label = "UV"; break;
    }
    g.text(buf, cx, y + 29, font_b, col::white, AL_C);
    g.text(label, cx, y + 48, font_s, g.dim(A_SECONDARY, y + 50, true), AL_C);
  }
  pageDots(g);
}

// ------------------------------------------------------------------ 24 Stunden
static void drawHourly(Gfx& g) {
  const WeatherData& w = app.wx;
  bg(g);
  header(g, "Nächste 24 Stunden");
  g.glass(8, 36, 304, 186);
  int n = w.nHours;
  if (n < 2) { pageDots(g); return; }
  const float x0 = 22, x1 = 298;
  float tmin = 999, tmax = -999;
  for (int i = 0; i < n; i++) { tmin = fminf(tmin, w.hours[i].temp); tmax = fmaxf(tmax, w.hours[i].temp); }
  float span = tmax - tmin < 1 ? 1 : tmax - tmin;
  auto px = [&](int i) { return x0 + i * (x1 - x0) / (n - 1); };
  auto py = [&](float t) { return 150 - (t - tmin) / span * 44; };

  // Fläche unter der Kurve: senkrechte Linien mit Alpha-Verlauf
  if (g.rowsVisible(100, 172)) {
    for (int x = (int)x0; x <= (int)x1; x++) {
      float f = (x - x0) / (x1 - x0) * (n - 1);
      int i = (int)f;
      if (i >= n - 1) i = n - 2;
      float yt = py(w.hours[i].temp + (w.hours[i + 1].temp - w.hours[i].temp) * (f - i));
      for (int y = (int)yt; y < 172; y++) g.plot(x, y, col::line, (uint8_t)(115 * (172 - y) / (172 - yt + 1)));
    }
  }
  for (int i = 0; i < n - 1; i++) g.line(px(i), py(w.hours[i].temp), px(i + 1), py(w.hours[i + 1].temp), 2.5f, col::line);

  char buf[12];
  for (int i = 0; i < n; i++) {
    int p = w.hours[i].pop;
    int h = p * 26 / 100;
    if (h > 0) g.fillRect((int)px(i) - 4, 204 - h, 8, h, col::rain, 215);
  }
  g.fillRect(16, 204, 288, 1, col::white, 46);
  for (int i = 0; i < n; i += 3) {
    float x = px(i);
    struct tm lt;
    time_t tt = w.hours[i].t;
    localtime_r(&tt, &lt);
    if (i == 0) snprintf(buf, sizeof(buf), "Jetzt");
    else snprintf(buf, sizeof(buf), "%02d", lt.tm_hour);
    g.text(buf, x, 44, font_s, i ? g.dim(A_SECONDARY, 50, true) : col::white, AL_C);
    drawIcon(g, codeIcon(w.hours[i].code, w.hours[i].day), x, 72, 24);
    float y = py(w.hours[i].temp);
    g.disc(x, y, 3.5f, col::white);
    snprintf(buf, sizeof(buf), "%d°", iround(w.hours[i].temp));
    g.text(buf, x, y - 20, font_s, col::white, AL_C);
    snprintf(buf, sizeof(buf), "%d%%", w.hours[i].pop);
    g.text(buf, x, 207, font_s, col::rainLight, AL_C);
  }
  pageDots(g);
}

// ------------------------------------------------------------------ 5 Tage
static void drawDaily(Gfx& g) {
  const WeatherData& w = app.wx;
  bg(g);
  header(g, "5-Tage-Vorhersage");
  g.glass(8, 36, 304, 186);
  int n = w.nDays < 5 ? w.nDays : 5;
  if (n == 0) { pageDots(g); return; }
  float lo = 999, hi = -999;
  for (int i = 0; i < n; i++) { lo = fminf(lo, w.days[i].tmin); hi = fmaxf(hi, w.days[i].tmax); }
  if (hi - lo < 1) hi = lo + 1;
  const float bx0 = 182, bx1 = 256;
  auto bx = [&](float t) { return bx0 + (t - lo) / (hi - lo) * (bx1 - bx0); };
  char buf[12];
  for (int i = 0; i < n; i++) {
    const DayData& d = w.days[i];
    int y = 36 + i * 37;
    float cy = y + 18.5f;
    if (!g.rowsVisible(y - 2, y + 40)) continue;
    if (i) g.fillRect(18, y, 284, 1, col::white, 30);
    struct tm lt;
    time_t tt = d.t + 12 * 3600;
    localtime_r(&tt, &lt);
    g.text(i == 0 ? "Heute" : WD_SHORT[lt.tm_wday], 20, cy, font_b, col::white, AL_L, VA_MID);
    drawIcon(g, codeIcon(d.code, true), 84, cy, 28);
    if (d.pop >= 10) {
      g.drop(110, cy + 1, 2.6f, col::rainLight);
      snprintf(buf, sizeof(buf), "%d%%", d.pop);
      g.text(buf, 116, cy, font_s, col::rainLight, AL_L, VA_MID);
    }
    snprintf(buf, sizeof(buf), "%d°", iround(d.tmin));
    g.text(buf, 174, cy, font_b, g.dim(A_SECONDARY, (int)cy, true), AL_R, VA_MID);
    g.rrect(bx0, cy - 3, bx1 - bx0, 6, 3, col::black, 56);
    float a = bx(d.tmin), b = bx(d.tmax);
    uint16_t ca = tempColor(d.tmin), cb = tempColor(d.tmax);
    g.sdf(a - 3, cy - 3, b + 3, cy + 3, ca, 255, [&](float px_, float py_) {
      float qx = px_ < a + 3 ? a + 3 - px_ : (px_ > b - 3 ? px_ - (b - 3) : 0);
      return sqrtf(qx * qx + (py_ - cy) * (py_ - cy)) - 3;
    });
    // Farbverlauf in den Balken legen
    if (b - a > 6) {
      for (int x = (int)a + 3; x < (int)b - 2; x++) {
        uint16_t c = lerp565(ca, cb, (x - a) / (b - a));
        g.fillRect(x, (int)cy - 3, 1, 6, c);
      }
    }
    if (i == 0) {
      float cx = bx(fminf(fmaxf(w.temp, d.tmin), d.tmax));
      g.disc(cx, cy, 4.5f, col::white);
      g.disc(cx, cy, 2.5f, tempColor(w.temp));
    }
    snprintf(buf, sizeof(buf), "%d°", iround(d.tmax));
    g.text(buf, 296, cy, font_b, col::white, AL_R, VA_MID);
  }
  pageDots(g);
}

// ------------------------------------------------------------------ Sonne & Wind
static void drawDetails(Gfx& g) {
  const WeatherData& w = app.wx;
  bg(g);
  header(g, "Sonne & Wind");
  char buf[32];
  const DayData& d0 = w.days[0];
  struct tm lt;

  // Sonne
  g.glass(8, 36, 148, 104);
  g.text("Sonne", 18, 44, font_s, g.dim(A_SECONDARY, 50, true));
  long len = (long)(d0.sunset - d0.sunrise);
  snprintf(buf, sizeof(buf), "%ld h %ld min", len / 3600, (len % 3600) / 60);
  g.text(buf, 146, 44, font_s, col::white, AL_R);
  const float ax = 82, ay = 118, r = 46;
  for (int i = 0; i <= 18; i++) {  // gepunkteter Bogen
    float a = PI_F + i * PI_F / 18;
    g.disc(ax + cosf(a) * r, ay + sinf(a) * r, 1.1f, col::white, 120);
  }
  g.fillRect(22, (int)ay, 120, 1, col::white, 76);
  float f = 0;
  if (app.epoch > d0.sunrise && d0.sunset > d0.sunrise) f = (float)(app.epoch - d0.sunrise) / (d0.sunset - d0.sunrise);
  if (f > 1) f = 1;
  float sa = PI_F + f * PI_F;
  if (f > 0.01f) g.arc(ax, ay, r, 3, PI_F, sa, col::sunRay);
  if (f > 0 && f < 1) {
    g.disc(ax + cosf(sa) * r, ay + sinf(sa) * r, 9, col::sunRay, 76);
    g.disc(ax + cosf(sa) * r, ay + sinf(sa) * r, 5.5f, col::sunRay);
  }
  localtime_r(&d0.sunrise, &lt);
  snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  g.text(buf, (int)(ax - r), 122, font_s, col::white, AL_C);
  localtime_r(&d0.sunset, &lt);
  snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  g.text(buf, (int)(ax + r), 122, font_s, col::white, AL_C);

  // Wind
  g.glass(164, 36, 148, 104);
  g.text("Wind", 174, 44, font_s, g.dim(A_SECONDARY, 50, true));
  const float wx = 214, wy = 96, wr = 32;
  g.ring(wx, wy, wr, 1.5f, col::white, 90);
  for (int i = 0; i < 12; i++) {
    float a = i * PI_F / 6;
    g.line(wx + cosf(a) * (wr - 3), wy + sinf(a) * (wr - 3), wx + cosf(a) * wr, wy + sinf(a) * wr, 1.5f, col::white, 140);
  }
  g.text("N", (int)wx, (int)(wy - wr + 5), font_s, col::white, AL_C);
  // Pfeil zeigt, wohin der Wind weht (Richtung + 180°)
  float ta = (w.windDir + 180) * PI_F / 180 - PI_F / 2;
  float tx = wx + cosf(ta) * (wr - 8), ty = wy + sinf(ta) * (wr - 8);
  g.line(wx - cosf(ta) * (wr - 10), wy - sinf(ta) * (wr - 10), tx, ty, 3, col::white);
  g.tri(tx + cosf(ta) * 6, ty + sinf(ta) * 6, tx + cosf(ta + 2.4f) * 8, ty + sinf(ta + 2.4f) * 8,
        tx + cosf(ta - 2.4f) * 8, ty + sinf(ta - 2.4f) * 8, col::white);
  snprintf(buf, sizeof(buf), "%d", iround(w.wind));
  g.text(buf, 256, 58, font_m, col::white);
  g.text("km/h", 256, 80, font_s, g.dim(A_SECONDARY, 86, true));
  snprintf(buf, sizeof(buf), "Böen %d", iround(w.gusts));
  g.text(buf, 256, 100, font_s, col::white);
  snprintf(buf, sizeof(buf), "aus %s", windName(w.windDir));
  g.text(buf, 256, 118, font_s, g.dim(A_SECONDARY, 124, true));

  // Luftdruck, Bewölkung, UV
  for (int i = 0; i < 3; i++) {
    int x = 8 + i * 104, y = 148;
    g.glass(x, y, 96, 74, 10);
    const char* label; const char* unit;
    switch (i) {
      case 0: label = "Luftdruck"; snprintf(buf, sizeof(buf), "%d", iround(w.pressure)); unit = "hPa"; break;
      case 1: label = "Bewölkung"; snprintf(buf, sizeof(buf), "%d", w.cloud); unit = "%"; break;
      default: label = "UV-Index"; snprintf(buf, sizeof(buf), "%d", iround(d0.uv)); unit = uvText(d0.uv); break;
    }
    g.text(label, x + 10, y + 9, font_s, g.dim(A_SECONDARY, y + 14, true));
    g.text(buf, x + 10, y + 28, font_m, col::white);
    g.text(unit, x + 14 + g.textWidth(buf, font_m), y + 34, font_s, g.dim(A_SECONDARY, y + 40, true));
    float by = y + 60;
    if (i < 2) g.rrect(x + 10, by - 2, 76, 4, 2, col::black, 64);
    if (i == 0) {
      float p = (w.pressure - 970) / 80;
      p = p < 0 ? 0 : (p > 1 ? 1 : p);
      g.disc(x + 10 + p * 76, by, 4, col::white);
    } else if (i == 1) {
      if (w.cloud > 3) g.rrect(x + 10, by - 2, 76 * w.cloud / 100.0f, 4, 2, col::cloudRain);
    } else {
      static const uint16_t uvc[] = {rgb(74, 222, 128), rgb(250, 204, 21), rgb(251, 146, 60), rgb(239, 68, 68), rgb(168, 85, 247)};
      for (int k = 0; k < 76; k++) {
        float t = k / 75.0f * 4;
        int s = (int)t;
        uint16_t c = s >= 4 ? uvc[4] : lerp565(uvc[s], uvc[s + 1], t - s);
        g.fillRect(x + 10 + k, (int)by - 2, 1, 4, c);
      }
      float p = d0.uv / 11;
      g.disc(x + 10 + (p > 1 ? 1 : p) * 76, by, 4, col::white);
    }
  }
  pageDots(g);
}

void uiDrawScreen(Gfx& g) {
  if (!app.wx.valid && app.screen != SCR_HOME) app.screen = SCR_HOME;
  switch (app.screen) {
    case SCR_HOME: drawHome(g); break;
    case SCR_HOURLY: drawHourly(g); break;
    case SCR_DAILY: drawDaily(g); break;
    default: drawDetails(g); break;
  }
}

void uiDrawBoot(Gfx& g, const char* status, int step) {
  g.background(rgb(20, 44, 90), rgb(60, 110, 170));
  float bob = sinf(step * 0.6f) * 3;
  drawIcon(g, IC_PARTLY_DAY, 160, 86 + bob, 96);
  g.text("Wetterstation", 160, 146, font_m, col::white, AL_C);
  g.text(status, 160, 176, font_s, g.dim(A_SECONDARY, 180), AL_C);
  for (int i = 0; i < 3; i++) g.disc(148 + i * 12, 206, 3, col::white, (step % 3) == i ? 255 : 90);
}
