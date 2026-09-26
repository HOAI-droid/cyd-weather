#include "eggs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "config.h"
#include "fonts/font_b.h"
#include "fonts/font_c.h"
#include "fonts/font_h.h"
#include "fonts/font_m.h"
#include "fonts/font_s.h"
#include "hw.h"
#include "icons.h"

static EggMode mode = EGG_NONE;
static uint32_t startMs = 0, lastFrame = 0;
static bool needDraw = true;

static int rnd(int a, int b) { return a + rand() % (b - a); }
static float frnd() { return (rand() % 10000) / 10000.0f; }

// ---------------------------------------------------------------- Melodien
struct Note { uint16_t f, ms; };
static const Note* melody = nullptr;
static int melodyLen = 0, melodyPos = 0;
static uint32_t melodyNext = 0;

static const Note BIRTHDAY_SONG[] = {{262, 250}, {262, 250}, {294, 500}, {262, 500}, {349, 500}, {330, 900},
                                     {262, 250}, {262, 250}, {294, 500}, {262, 500}, {392, 500}, {349, 900}};
static const Note DISCO_SONG[] = {{523, 150}, {0, 50}, {659, 150}, {0, 50}, {784, 150}, {0, 50}, {659, 150},
                                  {0, 50}, {587, 150}, {0, 50}, {740, 150}, {0, 50}, {880, 300}, {0, 200}};
static const Note GAMEOVER_SONG[] = {{392, 200}, {330, 200}, {262, 400}};
static const Note HIGHSCORE_SONG[] = {{523, 120}, {659, 120}, {784, 120}, {1047, 360}};

static void play(const Note* m, int n) {
  if (!SOUND_ENABLED) return;
  melody = m; melodyLen = n; melodyPos = 0; melodyNext = 0;
}
static void melodyLoop(bool repeat) {
  if (!melody) return;
  uint32_t now = hwMillis();
  if (now < melodyNext) return;
  if (melodyPos >= melodyLen) {
    if (!repeat) { melody = nullptr; hwTone(0, 0); return; }
    melodyPos = 0;
  }
  const Note& n = melody[melodyPos++];
  hwTone(n.f, n.ms);
  melodyNext = now + n.ms + 20;
}

// ---------------------------------------------------------------- Texte
static const char* const JOKES[] = {
    "Wie nennt man einen Schneemann mit Sonnenbrand?\nPfütze.",
    "Was ist ein Keks unter einem Baum?\nEin schattiges Plätzchen.",
    "Treffen sich zwei Wolken. Sagt die eine: \"Du siehst heute aber bedeckt aus.\"",
    "Warum ist der Meteorologe immer so entspannt?\nEr sieht alles kommen.",
    "Was ist weiß und stört beim Essen?\nEine Lawine.",
    "Treffen sich zwei Magneten. Sagt der eine: \"Was soll ich heute bloß anziehen?\"",
    "Was sagt der Donner zum Blitz?\n\"Immer musst du dich vordrängeln!\"",
    "Wie nennt man einen Bumerang, der nicht zurückkommt?\nStock.",
    "Was liegt am Strand und spricht undeutlich?\nEine Nuschel.",
    "Welche Wolke ist immer müde?\nDie Schlummerwolke.",
    "Was macht ein Clown im Büro?\nFaxen.",
    "Warum hat der Regenbogen keine Freunde?\nEr ist zu bunt für diese Welt.",
};
static const int N_JOKES = sizeof(JOKES) / sizeof(JOKES[0]);

static const char* const FROG_GOOD[] = {
    "Quak! Ganz oben auf der Leiter. Raus mit dir!",
    "Kräht der Hahn auf dem Mist, ändert sich das Wetter oder es bleibt, wie es ist.",
    "Abendrot, Gutwetterbot'. Morgenrot, schlecht Wetter droht.",
};
static const char* const FROG_MID[] = {
    "Quak. Halbe Leiter, halbe Sonne. Jacke mitnehmen!",
    "Ist der Mai kühl und nass, füllt's dem Bauern Scheun' und Fass.",
    "Wenn die Schwalben tief fliegen, werden wir Regen kriegen.",
};
static const char* const FROG_BAD[] = {
    "Quak! Ich bleib heute unten. Wer hoch klettert, wird nass.",
    "Regnet's an Siebenschläfer, regnet's sieben Wochen. Hoffentlich nicht heute.",
    "Kein schlechtes Wetter, nur falsche Kleidung. Sagt jedenfalls meine Oma.",
};
static int jokeIdx = 0, frogIdx = 0;

// ---------------------------------------------------------------- Wetterfrosch
static void drawFrog(Gfx& g, float fy) {
  uint16_t t, b;
  themeColors(app.wx.code, app.wx.isDay, t, b);
  g.background(t, b);
  g.fillRect(34, 24, 5, 206, col::wood);
  g.fillRect(86, 24, 5, 206, col::wood);
  for (int i = 0; i < 9; i++) g.fillRect(39, 36 + i * 23, 47, 4, col::wood);

  g.line(40, fy + 10, 30, fy + 20, 5, col::frog);
  g.line(84, fy + 10, 94, fy + 20, 5, col::frog);
  g.ellipse(62, fy, 24, 17, col::frog);
  g.ellipse(62, fy + 5, 15, 9, col::frogBelly);
  bool blink = ((hwMillis() - startMs) / 150) % 20 == 0;
  for (int s = -1; s <= 1; s += 2) {
    g.disc(62 + s * 12, fy - 16, 8, col::frog);
    if (blink) g.line(62 + s * 12 - 5, fy - 16, 62 + s * 12 + 5, fy - 16, 2, col::frogDark);
    else { g.disc(62 + s * 12, fy - 16, 5.5f, col::white); g.disc(63 + s * 12, fy - 15, 2.6f, col::ink); }
  }
  g.arc(62, fy - 6, 8, 2, 0.6f, 2.5f, col::frogDark);

  // Sprechblase
  float ty = fy - 10;
  if (ty < 56) ty = 56;
  if (ty > 160) ty = 160;
  g.rrect(112, 40, 196, 140, 14, col::white);
  g.tri(100, ty + 4, 116, ty - 8, 116, ty + 12, col::white);
  g.text("Der Wetterfrosch sagt:", 126, 52, font_s, rgb(74, 138, 90));
  float h = frogHeight(app.wx);
  const char* txt = h > 0.65f ? FROG_GOOD[frogIdx % 3] : (h > 0.3f ? FROG_MID[frogIdx % 3] : FROG_BAD[frogIdx % 3]);
  g.wrap(txt, 126, 74, 170, font_b, col::ink, 21);
  g.text("Tippen zum Schließen", 210, 206, font_s, g.dim(A_SECONDARY, 210), AL_C);
}

// ---------------------------------------------------------------- Tropfenfänger
struct Drop { float x, y, vy; uint8_t type; bool alive; };  // type 0 Regen, 1 Sonne, 2 Blitz
static Drop drops[14];
static float bucketX = 160;
static int score = 0, lives = 3, highscore = 0;
static bool gameOver = false, newRecord = false;
static uint32_t nextSpawn = 0, overSince = 0;

static void gameReset() {
  memset(drops, 0, sizeof(drops));
  bucketX = 160; score = 0; lives = 5; gameOver = false; newRecord = false;
  highscore = hwLoadInt("hiscore", 0);
  nextSpawn = hwMillis() + 600;
}

static void gameUpdate(const TouchNow& t, float dt) {
  if (gameOver) return;
  if (t.down) bucketX += (t.x - bucketX) * fminf(1.0f, dt * 14);
  if (bucketX < 26) bucketX = 26;
  if (bucketX > 294) bucketX = 294;
  float speedUp = 1.0f + score / 50.0f;
  uint32_t now = hwMillis();
  if (now >= nextSpawn) {
    for (auto& d : drops) {
      if (d.alive) continue;
      int r = rnd(0, 100);
      d.type = r < 8 ? 1 : (r < 22 ? 2 : 0);
      d.x = rnd(16, 304); d.y = 50; d.vy = (45 + rnd(0, 35)) * speedUp; d.alive = true;
      break;
    }
    nextSpawn = now + (uint32_t)(900 / speedUp) + rnd(0, 300);
  }
  for (auto& d : drops) {
    if (!d.alive) continue;
    d.y += d.vy * dt;
    if (d.y > 200 && d.y < 222 && fabsf(d.x - bucketX) < 26) {
      d.alive = false;
      if (d.type == 0) { score++; hwTone(1200, 25); }
      else if (d.type == 1) { score += 5; hwTone(1600, 60); }
      else { lives--; hwTone(160, 200); }
    } else if (d.y > 244) {
      d.alive = false;
      if (d.type == 0) { lives--; hwTone(220, 80); }
    }
  }
  if (lives <= 0) {
    gameOver = true; overSince = now;
    if (score > highscore) { highscore = score; newRecord = true; hwSaveInt("hiscore", score); play(HIGHSCORE_SONG, 4); }
    else play(GAMEOVER_SONG, 3);
  }
}

static void drawGame(Gfx& g) {
  g.background(rgb(16, 26, 51), rgb(30, 48, 87));
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", score);
  g.text(buf, 14, 8, font_c, col::white);
  snprintf(buf, sizeof(buf), "Rekord %d", highscore);
  g.text(buf, 14, 40, font_s, g.dim(A_SECONDARY, 44));
  for (int i = 0; i < 5; i++) glyphHeart(g, 300 - i * 18, 20, 14, i < lives ? col::heart : rgb(70, 80, 110));
  for (auto& d : drops) {
    if (!d.alive) continue;
    if (d.type == 0) g.drop(d.x, d.y, 5, col::rain);
    else if (d.type == 1) drawSun(g, d.x, d.y, 22);
    else glyphBolt(g, d.x, d.y, 26, col::bolt);
  }
  float bx = bucketX;
  // Eimer als Trapez (oben 48 px, unten 36 px breit)
  g.sdf(bx - 25, 203, bx + 25, 233, col::cloud, 255, [&](float px, float py) {
    float half = 24 - (py - 204) * 6 / 28;
    return fmaxf(fabsf(px - bx) - half, fmaxf(204 - py, py - 232));
  });
  g.fillRect((int)bx - 20, 211, 40, 5, col::rain);
  g.arc(bx, 204, 24, 2, 3.1416f, 6.2832f, col::cloud);
  if (gameOver) {
    g.rrect(60, 70, 200, 100, 14, col::black, 170);
    g.text("Game Over", 160, 86, font_m, col::white, AL_C);
    snprintf(buf, sizeof(buf), "%d Tropfen", score);
    g.text(buf, 160, 116, font_b, col::rainLight, AL_C);
    g.text(newRecord ? "Neuer Rekord!" : "Tippen zum Beenden", 160, 144, font_s, newRecord ? col::tagline : col::white, AL_C);
  }
}

// ---------------------------------------------------------------- Matrix
struct Column { float head, speed; char ch[20]; };
static Column cols[27];
static const char MATRIX_CHARS[] = "0123456789ABCDEFXYZ%+-=*<>";

static void matrixReset() {
  for (auto& c : cols) {
    c.head = rnd(-20, 20); c.speed = 6 + frnd() * 12;
    for (auto& ch : c.ch) ch = MATRIX_CHARS[rnd(0, sizeof(MATRIX_CHARS) - 1)];
  }
}
static void matrixUpdate(float dt) {
  for (auto& c : cols) {
    c.head += c.speed * dt;
    if (c.head > 32) { c.head = rnd(-12, 0); c.speed = 6 + frnd() * 12; }
    if (rand() % 6 == 0) c.ch[rnd(0, 20)] = MATRIX_CHARS[rnd(0, sizeof(MATRIX_CHARS) - 1)];
  }
}
static void drawMatrix(Gfx& g) {
  g.fillRect(0, g.oy, 320, Gfx::BAND_H, col::black);
  char s[2] = {0, 0};
  for (int ci = 0; ci < 27; ci++) {
    const Column& c = cols[ci];
    int h = (int)c.head;
    for (int r = 0; r < 20; r++) {
      int d = h - r;
      if (d < 0 || d > 11) continue;
      int y = r * 12;
      if (!g.rowsVisible(y - 4, y + 14)) continue;
      s[0] = c.ch[r];
      uint16_t color = d == 0 ? rgb(216, 255, 216) : blend565(col::matrix, col::black, 255 - d * 21);
      g.text(s, 6 + ci * 12, y, font_s, color, AL_C);
    }
  }
  char buf[64];
  g.rrect(70, 92, 180, 58, 6, col::black);
  g.fillRect(70, 92, 180, 1, col::matrix); g.fillRect(70, 149, 180, 1, col::matrix);
  g.fillRect(70, 92, 1, 58, col::matrix); g.fillRect(249, 92, 1, 58, col::matrix);
  if (app.wx.valid) snprintf(buf, sizeof(buf), "%d°C  %s", (int)lroundf(app.wx.temp), app.city);
  else snprintf(buf, sizeof(buf), "NO SIGNAL");
  g.text(buf, 160, 100, font_b, col::matrix, AL_C);
  g.text("WAKE UP, NEO …", 160, 126, font_s, rgb(124, 255, 158), AL_C);
}

// ---------------------------------------------------------------- 1. April
static void drawApril(Gfx& g, bool reveal) {
  g.background(rgb(90, 15, 28), rgb(142, 27, 44));
  g.rrect(16, 24, 288, 34, 8, col::alert);
  g.text("UNWETTERWARNUNG", 160, 41, font_m, rgb(58, 10, 16), AL_C, VA_MID);
  g.text("Ab 14 Uhr regnet es", 160, 80, font_m, col::white, AL_C);
  g.text("Gummibärchen.", 160, 106, font_m, col::white, AL_C);
  static const uint16_t bears[] = {rgb(255, 90, 95), rgb(255, 210, 63), rgb(74, 222, 128), rgb(255, 159, 67), col::white};
  uint32_t t = hwMillis() - startMs;
  for (int i = 0; i < 9; i++) {
    float x = 40 + i * 30;
    float y = 150 + (i % 3) * 14 + (reveal ? 0 : fmodf(t / 30.0f + i * 17, 20));
    uint16_t c = bears[i % 5];
    g.rrect(x, y, 14, 18, 6, c);
    g.disc(x + 3, y + 1, 3, c);
    g.disc(x + 11, y + 1, 3, c);
  }
  if (reveal) g.text("… April, April!", 160, 212, font_b, col::tagline, AL_C);
}

// ---------------------------------------------------------------- Flachwitz
static void drawJoke(Gfx& g) {
  g.background(rgb(34, 30, 70), rgb(70, 52, 120));
  g.text("Flachwitz des Tages", 160, 16, font_s, col::tagline, AL_C);
  g.rrect(16, 38, 288, 160, 16, col::white, 30);
  int lines = g.wrap(JOKES[jokeIdx], 30, 0, 260, font_b, col::white, 22, AL_C, false);
  g.wrap(JOKES[jokeIdx], 30, 118 - lines * 11, 260, font_b, col::white, 22, AL_C);
  g.text("BOOT = nächster Witz · Tippen = zurück", 160, 214, font_s, g.dim(A_SECONDARY, 218), AL_C);
}

// ---------------------------------------------------------------- Disco
static void hsv(float h, uint8_t& r, uint8_t& g, uint8_t& b) {
  float x = 1 - fabsf(fmodf(h / 60, 2) - 1);
  float rr = 0, gg = 0, bb = 0;
  if (h < 60) { rr = 1; gg = x; } else if (h < 120) { rr = x; gg = 1; } else if (h < 180) { gg = 1; bb = x; }
  else if (h < 240) { gg = x; bb = 1; } else if (h < 300) { rr = x; bb = 1; } else { rr = 1; bb = x; }
  r = rr * 255; g = gg * 255; b = bb * 255;
}
static void drawDisco(Gfx& g) {
  uint32_t t = hwMillis() - startMs;
  float h = fmodf(t / 8.0f, 360);
  uint8_t r, gg, b, r2, g2, b2;
  hsv(h, r, gg, b);
  hsv(fmodf(h + 120, 360), r2, g2, b2);
  g.background(rgb(r / 2, gg / 2, b / 2), rgb(r2 / 2, g2 / 2, b2 / 2));
  for (int i = 0; i < 8; i++) {  // Lichtkegel der Discokugel
    float a = t / 700.0f + i * 0.785f;
    g.line(160, 30, 160 + cosf(a) * 260, 30 + fabsf(sinf(a)) * 260, 10, col::white, 40);
  }
  g.disc(160, 30, 22, rgb(200, 200, 210));
  for (int i = 0; i < 5; i++) g.fillRect(140, 16 + i * 6, 40, 1, rgb(120, 120, 140));
  char buf[12];
  snprintf(buf, sizeof(buf), "%d°", app.wx.valid ? (int)lroundf(app.wx.temp) : 0);
  float pulse = (t / 250) % 2 ? 2 : 0;
  g.text(buf, 160, 90 - pulse, font_h, col::white, AL_C);
  g.text("DISCO-WETTER", 160, 176, font_c, col::white, AL_C);
  hwLed((t / 200) % 3 == 0, (t / 200) % 3 == 1, (t / 200) % 3 == 2);
}

// ---------------------------------------------------------------- Licht aus
static void drawDark(Gfx& g) {
  g.fillRect(0, g.oy, 320, Gfx::BAND_H, col::black);
  uint32_t t = hwMillis() - startMs;
  bool blink = (t / 120) % 25 == 0;
  float look = sinf(t / 900.0f) * 5;
  for (int s = -1; s <= 1; s += 2) {
    float x = 160 + s * 34;
    if (blink) g.line(x - 16, 100, x + 16, 100, 3, col::white);
    else { g.ellipse(x, 100, 18, 22, col::white); g.disc(x + look, 104, 8, col::black); }
  }
  g.text("Huch! Wer hat das", 160, 160, font_m, col::white, AL_C);
  g.text("Licht ausgemacht?", 160, 186, font_m, col::white, AL_C);
}

// ---------------------------------------------------------------- Party (Silvester / Geburtstag)
struct Particle { float x, y, vx, vy; uint16_t c; uint16_t life; };
static Particle parts[90];
static bool partyBirthday = false;
static uint32_t nextBurst = 0;

static void burst(float x, float y, bool confetti) {
  static const uint16_t cs[] = {rgb(255, 90, 95), rgb(255, 210, 63), rgb(74, 222, 128), rgb(91, 179, 255), rgb(200, 120, 255), col::white};
  uint16_t c = cs[rnd(0, 6)];
  int made = 0;
  for (auto& p : parts) {
    if (p.life) continue;
    float a = frnd() * 6.283f, v = confetti ? 20 + frnd() * 40 : 40 + frnd() * 80;
    p = {x, y, cosf(a) * v, sinf(a) * v - (confetti ? 30 : 0), confetti ? cs[rnd(0, 6)] : c, (uint16_t)(confetti ? 120 : 60 + rnd(0, 20))};
    if (++made >= 30) break;
  }
}
static void partyUpdate(float dt) {
  uint32_t now = hwMillis();
  if (now >= nextBurst) {
    burst(rnd(40, 280), partyBirthday ? rnd(-10, 40) : rnd(30, 120), partyBirthday);
    nextBurst = now + rnd(300, 700);
  }
  for (auto& p : parts) {
    if (!p.life) continue;
    p.x += p.vx * dt; p.y += p.vy * dt; p.vy += 60 * dt; p.vx *= 0.99f;
    p.life--;
  }
}
static void drawParty(Gfx& g) {
  g.background(rgb(8, 10, 30), rgb(26, 20, 60));
  for (auto& p : parts)
    if (p.life) {
      if (partyBirthday) g.fillRect((int)p.x, (int)p.y, 4, 6, p.c);
      else g.disc(p.x, p.y, 1.8f, p.c, p.life > 40 ? 255 : p.life * 6);
    }
  char buf[48];
  if (partyBirthday) {
    g.text("Alles Gute", 160, 150, font_c, col::white, AL_C);
    if (BIRTHDAY_NAME[0]) snprintf(buf, sizeof(buf), "zum Geburtstag, %s!", BIRTHDAY_NAME);
    else snprintf(buf, sizeof(buf), "zum Geburtstag!");
    g.text(buf, 160, 186, font_m, col::tagline, AL_C);
  } else {
    snprintf(buf, sizeof(buf), "%d", app.now.tm_year + 1900);
    g.text(buf, 160, 130, font_h, col::white, AL_C);
    g.text("Frohes neues Jahr!", 160, 200, font_m, col::tagline, AL_C);
  }
}

// ================================================================= Steuerung
void eggStart(EggMode m) {
  mode = m;
  startMs = lastFrame = hwMillis();
  needDraw = true;
  switch (m) {
    case EGG_FROG: frogIdx = rand(); hwTone(880, 60); break;
    case EGG_GAME: gameReset(); break;
    case EGG_MATRIX: matrixReset(); break;
    case EGG_JOKE: jokeIdx = (jokeIdx + 1 + rand() % (N_JOKES - 1)) % N_JOKES; break;
    case EGG_DISCO: play(DISCO_SONG, sizeof(DISCO_SONG) / sizeof(Note)); break;
    case EGG_PARTY:
      memset(parts, 0, sizeof(parts));
      nextBurst = 0;
      if (partyBirthday) play(BIRTHDAY_SONG, sizeof(BIRTHDAY_SONG) / sizeof(Note));
      break;
    default: break;
  }
}

EggMode eggMode() { return mode; }

static void stop() {
  mode = EGG_NONE;
  melody = nullptr;
  hwTone(0, 0);
  hwLed(false, false, false);
}

bool eggLoop(Gfx& g, const TouchNow& t) {
  if (mode == EGG_NONE) return false;
  uint32_t now = hwMillis();
  uint32_t age = now - startMs;
  float dt = (now - lastFrame) / 1000.0f;
  bool animate = true;
  uint32_t frameMs = 33;

  switch (mode) {
    case EGG_FROG: animate = age < 2200; frameMs = 40; if (age > 30000) stop(); break;
    case EGG_GAME:
      gameUpdate(t, dt);
      if (gameOver && now - overSince > 15000) stop();
      break;
    case EGG_MATRIX: matrixUpdate(dt); frameMs = 50; if (age > 20000) stop(); break;
    case EGG_APRIL: animate = age < 4600; frameMs = 60; if (age > 12000) stop(); break;
    case EGG_JOKE: animate = false; if (age > 20000) stop(); break;
    case EGG_DISCO: melodyLoop(true); frameMs = 60; if (age > 10000) stop(); break;
    case EGG_DARK: frameMs = 80; break;
    case EGG_PARTY: partyUpdate(dt); melodyLoop(false); if (age > 15000) stop(); break;
    default: break;
  }
  melodyLoop(false);
  if (mode == EGG_NONE) return false;
  if (!needDraw && (!animate || now - lastFrame < frameMs)) return true;
  lastFrame = now;
  needDraw = false;

  switch (mode) {
    case EGG_FROG: {
      float target = 210 - frogHeight(app.wx) * 170;
      float p = age / 2000.0f;
      if (p > 1) p = 1;
      p = 1 - (1 - p) * (1 - p);
      float hop = fabsf(sinf(age / 110.0f)) * 5 * (1 - p);
      float fy = 214 + (target - 214) * p - hop;
      g.frame([&] { drawFrog(g, fy); });
      break;
    }
    case EGG_GAME: g.frame([&] { drawGame(g); }); break;
    case EGG_MATRIX: g.frame([&] { drawMatrix(g); }); break;
    case EGG_APRIL: { bool rev = age > 4500; g.frame([&] { drawApril(g, rev); }); break; }
    case EGG_JOKE: g.frame([&] { drawJoke(g); }); break;
    case EGG_DISCO: g.frame([&] { drawDisco(g); }); break;
    case EGG_DARK: g.frame([&] { drawDark(g); }); break;
    case EGG_PARTY: g.frame([&] { drawParty(g); }); break;
    default: break;
  }
  return true;
}

void eggTap(int x, int y) {
  (void)x; (void)y;
  if (mode == EGG_GAME && !gameOver) return;  // im Spiel steuert der Finger den Eimer
  if (mode == EGG_DARK) return;               // endet, wenn es wieder hell wird
  if (mode == EGG_APRIL && hwMillis() - startMs < 4500) { startMs = hwMillis() - 4500; needDraw = true; return; }
  stop();
}

// ---------------------------------------------------------------- Auslöser
static uint32_t iconTaps[5];
static int iconTapN = 0;

void eggHomeTap(int x, int y) {
  (void)x; (void)y;
  uint32_t now = hwMillis();
  if (iconTapN > 0 && now - iconTaps[iconTapN - 1] > 800) iconTapN = 0;
  iconTaps[iconTapN++] = now;
  if (iconTapN >= 5) { iconTapN = 0; eggStart(EGG_FROG); }
}

static int cornerStep = 0;
static uint32_t cornerTime = 0;

bool eggCornerTap(int x, int y) {
  const int C = 48;
  int corner = -1;
  if (x < C && y < C) corner = 0;
  else if (x > 320 - C && y < C) corner = 1;
  else if (x > 320 - C && y > 240 - C) corner = 2;
  else if (x < C && y > 240 - C) corner = 3;
  if (corner < 0) return false;
  uint32_t now = hwMillis();
  if (now - cornerTime > 4000) cornerStep = 0;
  cornerTime = now;
  if (corner == cornerStep) {
    cornerStep++;
    hwTone(600 + cornerStep * 200, 30);
    if (cornerStep == 4) { cornerStep = 0; eggStart(EGG_MATRIX); }
  } else {
    cornerStep = corner == 0 ? 1 : 0;
  }
  return true;
}

void eggCheckCalendar() {
  if (!app.timeValid || mode != EGG_NONE) return;
  static int aprilDone = -1, newYearDone = -1, birthdayDone = -1;
  const struct tm& t = app.now;
  if (t.tm_mon == 3 && t.tm_mday == 1 && t.tm_hour >= 7 && aprilDone != t.tm_year) {
    aprilDone = t.tm_year;
    eggStart(EGG_APRIL);
  } else if (t.tm_mon == 0 && t.tm_mday == 1 && t.tm_hour == 0 && t.tm_min < 10 && newYearDone != t.tm_year) {
    newYearDone = t.tm_year;
    partyBirthday = false;
    eggStart(EGG_PARTY);
  } else if (BIRTHDAY_MONTH > 0 && t.tm_mon + 1 == BIRTHDAY_MONTH && t.tm_mday == BIRTHDAY_DAY &&
             t.tm_hour >= 7 && birthdayDone != t.tm_year) {
    birthdayDone = t.tm_year;
    partyBirthday = true;
    eggStart(EGG_PARTY);
  }
}

void eggCheckLight() {
  if (!LDR_EGG_ENABLED) return;
  static int baseline = -1;
  static uint32_t darkSince = 0, lightSince = 0;
  int v = hwLightLevel();
  if (baseline < 0) { baseline = v; return; }
  bool dark = v - baseline > LDR_DARK_DELTA;  // CYD: dunkler = höherer Wert
  uint32_t now = hwMillis();
  if (!dark && mode != EGG_DARK) baseline = (baseline * 15 + v) / 16;  // langsam nachführen
  if (dark) { lightSince = 0; if (!darkSince) darkSince = now; }
  else { darkSince = 0; if (!lightSince) lightSince = now; }
  if (mode == EGG_NONE && darkSince && now - darkSince > 1500) eggStart(EGG_DARK);
  if (mode == EGG_DARK && lightSince && now - lightSince > 800) stop();
}
