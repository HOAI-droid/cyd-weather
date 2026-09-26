#include "ui.h"
#include <stdio.h>
#include "app.h"
#include "config.h"
#include "fonts/font_b.h"
#include "fonts/font_c.h"
#include "fonts/font_g.h"
#include "fonts/font_h.h"
#include "fonts/font_m.h"
#include "fonts/font_s.h"
#include "fonts/font_x.h"
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
bool uiHitCity(int x, int y) { return x < 200 && y < 48; }

static void bg(Gfx& g) {
  uint16_t t, b;
  if (app.wx.valid) themeColors(app.wx.code, app.wx.isDay, t, b);
  else { t = rgb(36, 38, 42); b = rgb(16, 17, 20); }
  g.background(t, b);
}

static void clockStr(char* buf, int n) {
  if (app.timeValid) snprintf(buf, n, "%02d:%02d", app.now.tm_hour, app.now.tm_min);
  else snprintf(buf, n, "--:--");
}

static void header(Gfx& g, const char* title) {
  char clk[8];
  clockStr(clk, sizeof(clk));
  g.text(title, 14, 10, font_b, col::text);
  g.text(clk, 306, 10, font_b, col::text, AL_R);
}

static void pageDots(Gfx& g) {
  if (!g.rowsVisible(226, 238)) return;
  int n = SCR_COUNT, tw = n * 6 + (n - 1) * 6 + 10;
  float x = 160 - tw / 2.0f;
  for (int i = 0; i < n; i++) {
    if (i == app.screen) { g.rrect(x, 229, 16, 6, 3, col::gold); x += 22; }
    else { g.disc(x + 3, 232, 3, col::muted, 150); x += 12; }
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

  g.text(app.city, 14, 10, font_b, col::text);
  if (halloween) drawPumpkin(g, 14 + g.textWidth(app.city, font_b) + 14, 17, 16);
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
    g.text(buf, 306, 8, font_c, col::text, AL_R);
  }

  if (!w.valid) {
    g.text("Lade Wetterdaten …", 160, 120, font_m, col::text, AL_C, VA_MID);
    pageDots(g);
    return;
  }

  drawIcon(g, codeIcon(w.code, w.isDay), 62, 94, 84);
  if (xmas) drawSantaHat(g, 62, 94, 84);

  snprintf(buf, sizeof(buf), "%d°", iround(w.temp));
  g.text(buf, 116, 50, font_h, col::text);

  const DayData& d0 = w.days[0];
  glyphArrow(g, 262, 60, true, col::gold);
  snprintf(buf, sizeof(buf), "%d°", iround(d0.tmax));
  g.text(buf, 306, 53, font_b, col::text, AL_R);
  glyphArrow(g, 262, 80, false, col::teal);
  snprintf(buf, sizeof(buf), "%d°", iround(d0.tmin));
  g.text(buf, 306, 73, font_b, col::text, AL_R);
  snprintf(buf, sizeof(buf), "Gefühlt %d°", iround(w.feels));
  g.text(buf, 306, 94, font_s, g.dim(A_SECONDARY, 100), AL_R);

  g.text(codeText(w.code, w.isDay), 118, 114, font_m, col::text);
  g.text(w.tagline, 160, 139, font_s, col::gold, AL_C);

  // Kacheln
  for (int i = 0; i < 4; i++) {
    int x = 10 + i * 77, y = 158, cx = x + 35;
    g.glass(x, y, 70, 66, 10);
    const char* label = "";
    switch (i) {
      case 0: g.drop(cx, y + 17, 4.5f, col::teal); snprintf(buf, sizeof(buf), "%d%%", iround(w.humidity)); label = "Feuchte"; break;
      case 1: glyphWind(g, cx, y + 15, col::muted); snprintf(buf, sizeof(buf), "%d km/h", iround(w.wind)); label = "Wind"; break;
      case 2: glyphUmbrella(g, cx, y + 16, col::teal); snprintf(buf, sizeof(buf), "%d%%", d0.pop); label = "Regen"; break;
      case 3: drawSun(g, cx, y + 15, 20); snprintf(buf, sizeof(buf), "%d", iround(d0.uv)); label = "UV"; break;
    }
    g.text(buf, cx, y + 29, font_b, col::text, AL_C);
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
      for (int y = (int)yt; y < 172; y++) g.plot(x, y, col::gold, (uint8_t)(115 * (172 - y) / (172 - yt + 1)));
    }
  }
  for (int i = 0; i < n - 1; i++) g.line(px(i), py(w.hours[i].temp), px(i + 1), py(w.hours[i + 1].temp), 2.5f, col::gold);

  char buf[12];
  for (int i = 0; i < n; i++) {
    int p = w.hours[i].pop;
    int h = p * 26 / 100;
    if (h > 0) g.fillRect((int)px(i) - 4, 204 - h, 8, h, col::tealDark, 215);
  }
  g.fillRect(16, 204, 288, 1, col::text, 46);
  for (int i = 0; i < n; i += 3) {
    float x = px(i);
    struct tm lt;
    time_t tt = w.hours[i].t;
    localtime_r(&tt, &lt);
    if (i == 0) snprintf(buf, sizeof(buf), "Jetzt");
    else snprintf(buf, sizeof(buf), "%02d", lt.tm_hour);
    g.text(buf, x, 44, font_s, i ? g.dim(A_SECONDARY, 50, true) : col::text, AL_C);
    drawIcon(g, codeIcon(w.hours[i].code, w.hours[i].day), x, 72, 24);
    float y = py(w.hours[i].temp);
    g.disc(x, y, 3.5f, col::text);
    snprintf(buf, sizeof(buf), "%d°", iround(w.hours[i].temp));
    g.text(buf, x, y - 20, font_s, col::text, AL_C);
    snprintf(buf, sizeof(buf), "%d%%", w.hours[i].pop);
    g.text(buf, x, 207, font_s, col::teal, AL_C);
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
    if (i) g.fillRect(18, y, 284, 1, col::text, 30);
    struct tm lt;
    time_t tt = d.t + 12 * 3600;
    localtime_r(&tt, &lt);
    g.text(i == 0 ? "Heute" : WD_SHORT[lt.tm_wday], 20, cy, font_b, col::text, AL_L, VA_MID);
    drawIcon(g, codeIcon(d.code, true), 84, cy, 28);
    if (d.pop >= 10) {
      g.drop(110, cy + 1, 2.6f, col::teal);
      snprintf(buf, sizeof(buf), "%d%%", d.pop);
      g.text(buf, 116, cy, font_s, col::teal, AL_L, VA_MID);
    }
    snprintf(buf, sizeof(buf), "%d°", iround(d.tmin));
    g.text(buf, 174, cy, font_b, g.dim(A_SECONDARY, (int)cy, true), AL_R, VA_MID);
    g.rrect(bx0, cy - 3, bx1 - bx0, 6, 3, col::tick);
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
      g.disc(cx, cy, 4.5f, col::text);
      g.disc(cx, cy, 2.5f, tempColor(w.temp));
    }
    snprintf(buf, sizeof(buf), "%d°", iround(d.tmax));
    g.text(buf, 296, cy, font_b, col::text, AL_R, VA_MID);
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
  g.text(buf, 146, 44, font_s, col::text, AL_R);
  const float ax = 82, ay = 118, r = 46;
  for (int i = 0; i <= 18; i++) {  // gepunkteter Bogen
    float a = PI_F + i * PI_F / 18;
    g.disc(ax + cosf(a) * r, ay + sinf(a) * r, 1.1f, col::text, 120);
  }
  g.fillRect(22, (int)ay, 120, 1, col::text, 76);
  float f = 0;
  if (app.epoch > d0.sunrise && d0.sunset > d0.sunrise) f = (float)(app.epoch - d0.sunrise) / (d0.sunset - d0.sunrise);
  if (f > 1) f = 1;
  float sa = PI_F + f * PI_F;
  if (f > 0.01f) g.arc(ax, ay, r, 3, PI_F, sa, col::gold);
  if (f > 0 && f < 1) {
    g.disc(ax + cosf(sa) * r, ay + sinf(sa) * r, 9, col::gold, 76);
    g.disc(ax + cosf(sa) * r, ay + sinf(sa) * r, 5.5f, col::gold);
  }
  localtime_r(&d0.sunrise, &lt);
  snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  g.text(buf, (int)(ax - r), 122, font_s, col::text, AL_C);
  localtime_r(&d0.sunset, &lt);
  snprintf(buf, sizeof(buf), "%02d:%02d", lt.tm_hour, lt.tm_min);
  g.text(buf, (int)(ax + r), 122, font_s, col::text, AL_C);

  // Wind
  g.glass(164, 36, 148, 104);
  g.text("Wind", 174, 44, font_s, g.dim(A_SECONDARY, 50, true));
  const float wx = 214, wy = 96, wr = 32;
  g.ring(wx, wy, wr, 1.5f, col::text, 90);
  for (int i = 0; i < 12; i++) {
    float a = i * PI_F / 6;
    g.line(wx + cosf(a) * (wr - 3), wy + sinf(a) * (wr - 3), wx + cosf(a) * wr, wy + sinf(a) * wr, 1.5f, col::text, 140);
  }
  g.text("N", (int)wx, (int)(wy - wr + 5), font_s, col::text, AL_C);
  // Pfeil zeigt, wohin der Wind weht (Richtung + 180°)
  float ta = (w.windDir + 180) * PI_F / 180 - PI_F / 2;
  float tx = wx + cosf(ta) * (wr - 8), ty = wy + sinf(ta) * (wr - 8);
  g.line(wx - cosf(ta) * (wr - 10), wy - sinf(ta) * (wr - 10), tx, ty, 3, col::text);
  g.tri(tx + cosf(ta) * 6, ty + sinf(ta) * 6, tx + cosf(ta + 2.4f) * 8, ty + sinf(ta + 2.4f) * 8,
        tx + cosf(ta - 2.4f) * 8, ty + sinf(ta - 2.4f) * 8, col::text);
  snprintf(buf, sizeof(buf), "%d", iround(w.wind));
  g.text(buf, 256, 58, font_m, col::text);
  g.text("km/h", 256, 80, font_s, g.dim(A_SECONDARY, 86, true));
  snprintf(buf, sizeof(buf), "Böen %d", iround(w.gusts));
  g.text(buf, 256, 100, font_s, col::text);
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
    g.text(buf, x + 10, y + 28, font_m, col::text);
    g.text(unit, x + 14 + g.textWidth(buf, font_m), y + 34, font_s, g.dim(A_SECONDARY, y + 40, true));
    float by = y + 60;
    if (i < 2) g.rrect(x + 10, by - 2, 76, 4, 2, col::tick);
    if (i == 0) {
      float p = (w.pressure - 970) / 80;
      p = p < 0 ? 0 : (p > 1 ? 1 : p);
      g.disc(x + 10 + p * 76, by, 4, col::text);
    } else if (i == 1) {
      if (w.cloud > 3) g.rrect(x + 10, by - 2, 76 * w.cloud / 100.0f, 4, 2, col::muted);
    } else {
      static const uint16_t uvc[] = {rgb(74, 222, 128), rgb(250, 204, 21), rgb(251, 146, 60), rgb(239, 68, 68), rgb(168, 85, 247)};
      for (int k = 0; k < 76; k++) {
        float t = k / 75.0f * 4;
        int s = (int)t;
        uint16_t c = s >= 4 ? uvc[4] : lerp565(uvc[s], uvc[s + 1], t - s);
        g.fillRect(x + 10 + k, (int)by - 2, 1, 4, c);
      }
      float p = d0.uv / 11;
      g.disc(x + 10 + (p > 1 ? 1 : p) * 76, by, 4, col::text);
    }
  }
  pageDots(g);
}

// ------------------------------------------------------------------ Regenradar
static void rainStatus(char* buf, int n, uint16_t& c) {
  int start;
  float peak;
  int r = rainOutlook(app.wx, app.epoch, start, peak);
  const char* s = peak < 2.5f ? "leicht" : (peak < 10 ? "mäßig" : "stark");
  if (r == 1) { snprintf(buf, n, "Es regnet (%s)", s); c = col::teal; }
  else if (r == 2) {
    time_t t = app.epoch + start * 60;
    struct tm lt;
    localtime_r(&t, &lt);
    snprintf(buf, n, "Regen ab %02d:%02d", lt.tm_hour, lt.tm_min);
    c = col::gold;
  } else if (app.wx.nRain15 > 0) { snprintf(buf, n, "Kein Regen in 2 Std."); c = col::muted; }
  else buf[0] = 0;
}

static const uint16_t RAIN_COL[6] = {0, rgb(62, 142, 143), rgb(111, 195, 201), rgb(178, 228, 232),
                                     rgb(216, 188, 138), rgb(224, 122, 95)};
static const uint8_t RAIN_A[6] = {0, 150, 190, 215, 235, 245};

static void drawRadar(Gfx& g) {
  const RadarView& v = app.radar;
  char buf[48];
  if (v.px && v.mapOk) {
    uint16_t lut[16];
    for (int i = 0; i < 16; i++) lut[i] = lerp565(col::mapLow, col::mapHigh, powf(i / 15.0f, 0.85f));
    for (int y = g.oy; y < g.oy + Gfx::BAND_H; y++) {
      const uint8_t* row = v.px + y * RADAR_W;
      for (int x = 0; x < RADAR_W; x++) {
        uint8_t p = row[x], r = p & 0x0F;
        uint16_t c = lut[p >> 4];
        if (r) c = blend565(RAIN_COL[r > 5 ? 5 : r], c, RAIN_A[r > 5 ? 5 : r]);
        g.spr.drawPixel(x, y - g.oy, c);
      }
    }
    // Entfernungsringe 25 und 50 km
    float mpp = radarMetersPerPixel(app.lat, RADAR_ZOOM);
    for (int k = 1; k <= 2; k++) {
      float r = 25000.0f * k / mpp;
      for (int i = 0; i < 72; i++) {
        float a = i * PI_F / 36;
        g.disc(160 + cosf(a) * r, 120 + sinf(a) * r, 0.8f, col::text, 90);
      }
      snprintf(buf, sizeof(buf), "%d km", 25 * k);
      g.text(buf, 157 - (int)(r * 0.71f), 118 + (int)(r * 0.71f), font_x, col::text, AL_R);
    }
    g.disc(160, 120, 8, col::gold, 70);
    g.disc(160, 120, 3.5f, col::gold);
    g.text("CARTO · OSM · RainViewer", 314, 184, font_x, col::muted, AL_R);
  } else {
    bg(g);
    g.text(v.status[0] ? v.status : "Radar wird geladen …", 160, 116, font_s, col::muted, AL_C, VA_MID);
  }

  // Kopfzeile
  g.fillRect(0, 0, 320, 36, col::panel, 215);
  g.text("Regenradar", 14, 10, font_b, col::text);
  uint16_t sc = col::muted;
  rainStatus(buf, sizeof(buf), sc);
  g.text(buf, 306, 12, font_s, sc, AL_R);

  // Fußzeile: Stand des Radarbilds und Regen der nächsten 2 Stunden
  g.fillRect(0, 194, 320, 46, col::panel, 225);
  if (v.rainOk && v.frameTime) {
    struct tm lt;
    localtime_r(&v.frameTime, &lt);
    snprintf(buf, sizeof(buf), "RADAR %02d:%02d", lt.tm_hour, lt.tm_min);
  } else snprintf(buf, sizeof(buf), "RADAR –");
  g.text(buf, 14, 200, font_x, col::muted);
  for (int i = 1; i <= 5; i++) g.rrect(14 + (i - 1) * 19, 214, 17, 6, 2, RAIN_COL[i]);
  g.text("schwach", 14, 223, font_x, col::muted);
  g.text("stark", 106, 223, font_x, col::muted, AL_R);

  const WeatherData& w = app.wx;
  g.text("NÄCHSTE 2 STD.", 306, 200, font_x, col::muted, AL_R);
  int shown = 0;
  for (int i = 0; i < w.nRain15 && shown < 8; i++) {
    if (w.rain15T0 + i * 900 + 900 <= app.epoch) continue;
    float mmh = w.rain15[i] * 4;
    float x = 170 + shown * 17.5f;
    if (mmh < 0.2f) g.rrect(x, 224, 12, 2, 1, col::tick);
    else {
      float h = fminf(14, 3 + mmh * 2.5f);
      g.rrect(x, 226 - h, 12, h, 2, mmh < 2.5f ? col::teal : (mmh < 10 ? col::gold : col::coral));
    }
    shown++;
  }
  pageDots(g);
}

// ------------------------------------------------------------------ Luftqualität
static void subLabel(Gfx& g, int x, int y, const char* main, const char* sub) {
  g.text(main, x, y, font_x, col::muted);
  if (sub) g.text(sub, x + g.textWidth(main, font_x) + 1, y + 3, font_x, col::muted);
}

static void drawAir(Gfx& g) {
  const AirData& a = app.air;
  bg(g);
  header(g, "Luftqualität");
  if (!a.valid) {
    g.text("Luftdaten werden geladen …", 160, 120, font_s, col::muted, AL_C, VA_MID);
    pageDots(g);
    return;
  }
  const float cx = 160, cy = 150, R = 90;
  for (int i = 0; i < 5; i++)
    g.arc(cx, cy, R, 4, PI_F + (i / 5.0f) * PI_F + 0.02f, PI_F + ((i + 1) / 5.0f) * PI_F - 0.02f, aqiColor(i * 20 + 10));
  for (int i = 0; i <= 50; i++) {
    float an = PI_F + i / 50.0f * PI_F;
    bool maj = i % 10 == 0;
    float r0 = R - (maj ? 16 : 10), r1 = R - 6;
    g.line(cx + cosf(an) * r0, cy + sinf(an) * r0, cx + cosf(an) * r1, cy + sinf(an) * r1, maj ? 1.4f : 1,
           maj ? col::tickMajor : col::tick);
    if (maj) {
      char l[4];
      snprintf(l, sizeof(l), "%d", i * 2);
      g.text(l, (int)(cx + cosf(an) * (R - 26)), (int)(cy + sinf(an) * (R - 26)), font_x, col::muted, AL_C, VA_MID);
    }
  }
  float t = fminf(fmaxf(a.aqi / 100.0f, 0), 1);
  float an = PI_F + t * PI_F;
  g.line(cx + cosf(an) * 50, cy + sinf(an) * 50, cx + cosf(an) * (R - 4), cy + sinf(an) * (R - 4), 2, col::gold);
  g.disc(cx + cosf(an) * (R - 4), cy + sinf(an) * (R - 4), 3.2f, col::gold);

  char buf[48];
  snprintf(buf, sizeof(buf), "%d", iround(a.aqi));
  g.text(buf, (int)cx, 138, font_g, col::text, AL_C, VA_BASE);
  g.text(aqiText(a.aqi), (int)cx, 143, font_s, col::gold, AL_C);

  g.fillRect(14, 166, 292, 1, col::tick);
  const float vals[4] = {a.pm25, a.pm10, a.o3, a.no2};
  for (int i = 0; i < 4; i++) {
    int x = 14 + i * 75;
    if (i) g.fillRect(x - 8, 172, 1, 30, col::tick);
    switch (i) {
      case 0: subLabel(g, x, 172, "PM", "2.5"); break;
      case 1: subLabel(g, x, 172, "PM", "10"); break;
      case 2: subLabel(g, x, 172, "OZON", nullptr); break;
      default: subLabel(g, x, 172, "NO", "2"); break;
    }
    snprintf(buf, sizeof(buf), "%d", iround(vals[i]));
    g.text(buf, x, 186, font_b, col::text);
    g.text("µg/m³", x + g.textWidth(buf, font_b) + 3, 191, font_x, col::muted);
  }
  pollenSummary(a, buf, sizeof(buf));
  if (buf[0]) {
    g.text("POLLEN", 14, 213, font_x, col::muted);
    g.text(buf, 60, 210, font_s, col::text);
  }
  pageDots(g);
}

// ------------------------------------------------------------------ Ortswahl
static void dialogHeader(Gfx& g, const char* title, const char* action) {
  bg(g);
  g.text(title, 14, 10, font_b, col::text);
  if (action) g.text(action, 306, 12, font_s, col::gold, AL_R);
  g.fillRect(14, 36, 292, 1, col::tick);
}

static void pin(Gfx& g, float cx, float cy, uint16_t c) {
  g.ring(cx, cy - 2, 3.6f, 1.4f, c);
  g.tri(cx - 3.2f, cy, cx + 3.2f, cy, cx, cy + 6, c);
}

bool uiHitAction(int x, int y) { return x > 220 && y < 38; }

int uiHitPlaceRow(int x, int y) {
  (void)x;
  if (y < 40 || y >= 40 + app.nPlaces * 32 || y >= 198) return -1;
  return (y - 40) / 32;
}
bool uiHitPlacesSearch(int x, int y) { return y >= 200 && x < 160; }
bool uiHitPlacesWifi(int x, int y) { return y >= 200 && x >= 160; }

static bool samePlace(const Place& p) { return fabsf(p.lat - app.lat) < 0.01f && fabsf(p.lon - app.lon) < 0.01f; }

static void drawPlaces(Gfx& g) {
  dialogHeader(g, "Orte", "Fertig");
  for (int i = 0; i < app.nPlaces; i++) {
    const Place& p = app.places[i];
    int y = 40 + i * 32;
    bool cur = samePlace(p);
    pin(g, 22, y + 15, cur ? col::gold : col::muted);
    g.text(p.name, 36, y + 8, font_b, cur ? col::gold : col::text);
    g.text(p.region, 306, y + 12, font_x, col::muted, AL_R);
    g.fillRect(36, y + 31, 270, 1, col::panelEdge);
  }
  if (app.nPlaces < MAX_PLACES)
    g.text("Tippen = wechseln · lange drücken = löschen", 160, 48 + app.nPlaces * 32, font_x, col::muted, AL_C);
  g.rrect(10, 202, 144, 30, 8, col::gold);
  g.text("+ Ort suchen", 82, 217, font_s, col::panel, AL_C, VA_MID);
  g.glass(166, 202, 144, 30, 8);
  g.text("WLAN ändern", 238, 217, font_s, col::text, AL_C, VA_MID);
}

// Tastatur: gemeinsame Geometrie für Zeichnen und Antippen
struct Key { float x, y, w, h; const char* label; int code; };

template <typename F>
static void forEachKey(bool alt, F f) {
  static const char* const P[3][10] = {{"Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P"},
                                       {"A", "S", "D", "F", "G", "H", "J", "K", "L", nullptr},
                                       {"Y", "X", "C", "V", "B", "N", "M", nullptr}};
  static const char* const A[3][10] = {{"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
                                       {"Ä", "Ö", "Ü", "ß", "-", ".", "'", "/", "&", nullptr},
                                       {"(", ")", ",", ":", "!", "?", "é", nullptr}};
  const float m = 5, gap = 4, U = (320 - 2 * m + gap) / 10, kw = U - gap, kh = 29, gv = 5, y0 = 104;
  for (int r = 0; r < 3; r++) {
    const char* const* row = alt ? A[r] : P[r];
    int n = 0;
    while (n < 10 && row[n]) n++;
    float units = n + (r == 2 ? 1.5f : 0);
    float x = m + (10 - units) * U / 2, y = y0 + r * (kh + gv);
    for (int i = 0; i < n; i++, x += U) f(Key{x, y, kw, kh, row[i], KEY_CHAR});
    if (r == 2) f(Key{x, y, 1.5f * U - gap, kh, "", KEY_BACK});
  }
  float y = y0 + 3 * (kh + gv), x = m;
  f(Key{x, y, 2 * U - gap, kh, alt ? "ABC" : "ÄÖÜ", KEY_ALT});
  x += 2 * U;
  f(Key{x, y, 5 * U - gap, kh, "Leerzeichen", KEY_SPACE});
  x += 5 * U;
  f(Key{x, y, 3 * U - gap, kh, "Suchen", KEY_SEARCH});
}

int uiKeyAt(int px, int py, const char** ch) {
  int hit = -1;
  forEachKey(app.kbAlt, [&](const Key& k) {
    if (px >= k.x - 2 && px < k.x + k.w + 2 && py >= k.y - 2.5f && py < k.y + k.h + 2.5f) {
      hit = k.code;
      *ch = k.label;
    }
  });
  return hit;
}

static void drawSearch(Gfx& g) {
  bg(g);
  g.glass(10, 8, 234, 30, 9);
  g.ring(25, 21, 4.6f, 1.5f, col::muted);
  g.line(28.5f, 24.5f, 32, 28, 1.5f, col::muted);
  if (app.query[0]) {
    g.text(app.query, 40, 12, font_b, col::text);
    int cx = 41 + g.textWidth(app.query, font_b);
    if (cx < 238) g.fillRect(cx, 13, 2, 18, col::gold);
  } else {
    g.fillRect(40, 13, 2, 18, col::gold);
    g.text("Ort eingeben", 46, 13, font_b, col::muted);
  }
  g.text("Abbrechen", 306, 16, font_s, col::gold, AL_R);
  if (app.searchMsg[0]) g.wrap(app.searchMsg, 14, 52, 292, font_s, col::coral, 17, AL_C);
  else g.wrap("Namen eingeben und \"Suchen\" tippen. Gefundene Orte werden gespeichert.", 14, 52, 292, font_s,
              col::muted, 17, AL_C);

  forEachKey(app.kbAlt, [&](const Key& k) {
    if (!g.rowsVisible((int)k.y, (int)(k.y + k.h))) return;
    bool act = k.code == KEY_SEARCH, spec = k.code != KEY_CHAR && !act;
    g.rrect(k.x, k.y, k.w, k.h, 6, act ? col::gold : col::panelEdge);
    if (!act) g.rrect(k.x + 1, k.y + 1, k.w - 2, k.h - 2, 5, spec ? rgb(18, 20, 24) : col::panel);
    float cx = k.x + k.w / 2, cy = k.y + k.h / 2;
    if (k.code == KEY_BACK) {
      uint16_t c = col::text;
      g.line(cx - 9, cy, cx - 4, cy - 6, 1.4f, c);
      g.line(cx - 9, cy, cx - 4, cy + 6, 1.4f, c);
      g.line(cx - 4, cy - 6, cx + 9, cy - 6, 1.4f, c);
      g.line(cx - 4, cy + 6, cx + 9, cy + 6, 1.4f, c);
      g.line(cx + 9, cy - 6, cx + 9, cy + 6, 1.4f, c);
      g.line(cx - 1, cy - 3, cx + 5, cy + 3, 1.4f, c);
      g.line(cx + 5, cy - 3, cx - 1, cy + 3, 1.4f, c);
    } else {
      g.text(k.label, (int)cx, (int)cy, k.code == KEY_CHAR ? font_b : font_s, act ? col::panel : (spec ? col::muted : col::text),
             AL_C, VA_MID);
    }
  });
}

int uiHitResult(int x, int y) {
  (void)x;
  if (y < 40 || y >= 40 + app.nResults * 36) return -1;
  return (y - 40) / 36;
}

static void drawResults(Gfx& g) {
  char buf[64];
  snprintf(buf, sizeof(buf), "Treffer für \"%s\"", app.query);
  dialogHeader(g, buf, "Zurück");
  for (int i = 0; i < app.nResults; i++) {
    const Place& p = app.results[i];
    int y = 40 + i * 36;
    pin(g, 22, y + 17, col::gold);
    g.text(p.name, 36, y + 5, font_b, col::text);
    g.text(p.region, 36, y + 23, font_x, col::muted);
    g.fillRect(36, y + 35, 270, 1, col::panelEdge);
  }
}

void uiDrawScreen(Gfx& g) {
  if (!app.wx.valid && app.screen < SCR_COUNT && app.screen != SCR_HOME) app.screen = SCR_HOME;
  switch (app.screen) {
    case SCR_HOME: drawHome(g); break;
    case SCR_HOURLY: drawHourly(g); break;
    case SCR_DAILY: drawDaily(g); break;
    case SCR_RADAR: drawRadar(g); break;
    case SCR_AIR: drawAir(g); break;
    case SCR_PLACES: drawPlaces(g); break;
    case SCR_SEARCH: drawSearch(g); break;
    case SCR_RESULTS: drawResults(g); break;
    default: drawDetails(g); break;
  }
}

void uiDrawPortal(Gfx& g, const char* apName, const char* note) {
  g.background(rgb(38, 40, 45), rgb(14, 16, 19));
  g.text("Einrichtung", 160, 10, font_m, col::text, AL_C);
  char buf[96];
  const char* steps[3];
  snprintf(buf, sizeof(buf), "Mit dem Handy das WLAN \"%s\" verbinden.", apName);
  steps[0] = buf;
  steps[1] = "Die Einrichtungsseite öffnet sich. Falls nicht: im Browser 192.168.4.1 aufrufen.";
  steps[2] = "\"Configure WiFi\" tippen, dein WLAN wählen, Passwort und Stadt eingeben, \"Save\".";
  int y = 44;
  for (int i = 0; i < 3; i++) {
    g.disc(24, y + 8, 9, col::gold, 70);
    char n[2] = {(char)('1' + i), 0};
    g.text(n, 24, y + 8, font_s, col::text, AL_C, VA_MID);
    int lines = g.wrap(steps[i], 42, y, 264, font_s, col::text, 17);
    y += lines * 17 + 12;
  }
  if (note && note[0]) {
    g.glass(12, 188, 296, 46, 8);
    g.wrap(note, 20, 194, 280, font_s, col::gold, 17, AL_C);
  }
}

void uiDrawBoot(Gfx& g, const char* status, int step) {
  g.background(rgb(38, 40, 45), rgb(14, 16, 19));
  float bob = sinf(step * 0.6f) * 3;
  drawIcon(g, IC_PARTLY_DAY, 160, 86 + bob, 96);
  g.text("Wetterstation", 160, 146, font_m, col::text, AL_C);
  g.text(status, 160, 176, font_s, g.dim(A_SECONDARY, 180), AL_C);
  for (int i = 0; i < 3; i++) g.disc(148 + i * 12, 206, 3, (step % 3) == i ? col::gold : col::muted, (step % 3) == i ? 255 : 120);
}
