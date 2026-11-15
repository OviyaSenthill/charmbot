#include "apps/FaceApp.h"
#include "apps/FaceStyle.h"
#include "core/WeatherService.h"
#include "core/Mood.h"
#include "core/Net.h"
#include "core/MusicWatch.h"
#include "core/Backlight.h"
#include "pins.h"
#include <time.h>

using namespace FaceStyle;

// ---------- building blocks ----------
enum class EyeShape : uint8_t { Oval, ClosedArc, Line, Heart, Spiral };
enum class Brow     : uint8_t { None, Worried, Angry };
enum class Mouth    : uint8_t { None, Smile, SmallSmile, Frown, O, Flat };
enum class Palette  : uint8_t { Cyan, Red, Pink };

struct ExprDef {
  EyeShape left, right;
  Brow brow;
  Mouth mouth;
  Palette palette;
  bool bigEyes;
  uint8_t openPct;  // how open oval eyes are (sleepy = less)
};

// One row per Expression (same order as the enum). Add your own here!
static const ExprDef EXPRESSIONS[] = {
  /* Neutral   */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::None,       Palette::Cyan, false, 100},
  /* Happy     */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::Smile,      Palette::Cyan, false, 100},
  /* Wink      */ {EyeShape::Oval,      EyeShape::ClosedArc, Brow::None,    Mouth::SmallSmile, Palette::Cyan, false, 100},
  /* Worried   */ {EyeShape::Oval,      EyeShape::Oval,      Brow::Worried, Mouth::Frown,      Palette::Cyan, false, 100},
  /* Angry     */ {EyeShape::Oval,      EyeShape::Oval,      Brow::Angry,   Mouth::Frown,      Palette::Red,  false, 100},
  /* Love      */ {EyeShape::Heart,     EyeShape::Heart,     Brow::None,    Mouth::SmallSmile, Palette::Pink, false, 100},
  /* Surprised */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::O,          Palette::Cyan, true , 100},
  /* Bored     */ {EyeShape::Line,      EyeShape::Line,      Brow::None,    Mouth::Flat,       Palette::Cyan, false, 100},
  /* Content   */ {EyeShape::ClosedArc, EyeShape::ClosedArc, Brow::None,    Mouth::SmallSmile, Palette::Cyan, false, 100},
  /* Sad       */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::Frown,      Palette::Cyan, false, 100},
  /* Sleepy    */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::Flat,       Palette::Cyan, false, 40 },
  /* Asleep    */ {EyeShape::Line,      EyeShape::Line,      Brow::None,    Mouth::None,       Palette::Cyan, false, 100},
  /* Yawn      */ {EyeShape::Line,      EyeShape::Line,      Brow::None,    Mouth::O,          Palette::Cyan, false, 100},
  /* Dizzy     */ {EyeShape::Spiral,    EyeShape::Spiral,    Brow::None,    Mouth::Flat,       Palette::Cyan, false, 100},
  /* Sing      */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::O,          Palette::Cyan, false, 100},
};
static_assert(sizeof(EXPRESSIONS) / sizeof(EXPRESSIONS[0]) == (size_t)Expression::COUNT,
              "EXPRESSIONS table must have one row per Expression");

// ---------- drawing helpers ----------
// Angles in degrees, screen convention (0 = right, 90 = down).
static void thickArc(TFT_eSPI& g, int cx, int cy, int r, int a0, int a1, int w, uint16_t c) {
  for (int a = a0; a <= a1; a += 4) {
    float rad = a * DEG_TO_RAD;
    g.fillCircle(cx + r * cosf(rad), cy + r * sinf(rad), w / 2, c);
  }
}

static void thickLine(TFT_eSPI& g, int x0, int y0, int x1, int y1, int w, uint16_t c) {
  int steps = max(abs(x1 - x0), abs(y1 - y0)) / 2 + 1;
  for (int i = 0; i <= steps; i++) {
    g.fillCircle(x0 + (x1 - x0) * i / steps, y0 + (y1 - y0) * i / steps, w / 2, c);
  }
}

static void heart(TFT_eSPI& g, int cx, int cy, int r, uint16_t c) {
  g.fillCircle(cx - r * 9 / 10, cy - r * 3 / 10, r, c);
  g.fillCircle(cx + r * 9 / 10, cy - r * 3 / 10, r, c);
  g.fillTriangle(cx - r * 185 / 100, cy + r / 10,
                 cx + r * 185 / 100, cy + r / 10,
                 cx, cy + r * 19 / 10, c);
}

static float eyeSquash = 1.0f;  // >1 = wider + shorter (dance landing)

static void drawEye(TFT_eSPI& g, EyeShape s, int cx, int cy, float open,
                    bool big, uint16_t col, uint16_t glow) {
  switch (s) {
    case EyeShape::Oval: {
      float scale = big ? 1.2f : 1.0f;
      int rx = (int)(EYE_W / 2 * scale * eyeSquash);
      int ry = max(3, (int)(EYE_H / 2 * scale * open / eyeSquash));
      g.fillEllipse(cx, cy, rx + GLOW_PX, ry + GLOW_PX, glow);
      g.fillEllipse(cx, cy, rx, ry, col);
      break;
    }
    case EyeShape::ClosedArc:   // ^ shape
      thickArc(g, cx, cy + 8, 16, 210, 330, LINE_W, col);
      break;
    case EyeShape::Line:
      thickLine(g, cx - 18, cy + 4, cx + 18, cy + 4, LINE_W, col);
      break;
    case EyeShape::Heart:
      heart(g, cx, cy - 6, 15, glow);
      heart(g, cx, cy - 6, 11, col);
      break;
    case EyeShape::Spiral: {  // spinning dizzy spiral
      int rot = (millis() / 3) % 360;
      for (int t = 0; t <= 720; t += 24) {
        float r = 2 + t / 40.0f;
        float a = (t + rot) * DEG_TO_RAD;
        g.fillCircle(cx + r * cosf(a), cy + r * sinf(a), 3, col);
      }
      break;
    }
  }
}

// ---------- small drawings for thought bubbles ----------
static void miniNote(TFT_eSPI& g, int x, int y, uint16_t c) {
  g.fillEllipse(x - 4, y + 6, 4, 3, c);
  g.fillRect(x - 1, y - 8, 2, 14, c);
  g.fillTriangle(x + 1, y - 8, x + 8, y - 4, x + 1, y - 2, c);
}

// ---------- app ----------
void FaceApp::enter(TFT_eSPI& tft) {
  tft.fillScreen(VISOR);

  spr = new TFT_eSprite(&tft);
  spr->setColorDepth(16);
  if (!spr->createSprite(SPR_W, SPR_H)) {
    // Not enough contiguous RAM for 16-bit — fall back to 8-bit
    spr->setColorDepth(8);
    if (!spr->createSprite(SPR_W, SPR_H)) {
      delete spr;
      spr = nullptr;  // draw straight to the screen (will flicker a bit)
    }
  }
  uint32_t now = millis();
  nextBlink = now + 1500;
  nextGlance = now + 1000;
  nextYawn = now + 20000;
  nextIdleFun = now + random(8000, 20000);
  accessory = Accessory::None;
  lastFrame = 0;
  drawnBubble = Bubble::None;
  wasConnected = Net::connected();
  seenMusicStart = MusicWatch::startedAt();
  MusicWatch::setEnabled(true);

  // React to something that happened while we were away (e.g. a game you just played)
  if (Mood::lastEventSeq() != seenMoodSeq && now - Mood::lastEventAt() < 120000) {
    seenMoodSeq = Mood::lastEventSeq();
    if (Mood::lastEvent() == Mood::HIGH_SCORE) react(Expression::Love, 3500, Bubble::Heart);
    if (Mood::lastEvent() == Mood::GAME_LOST) react(Expression::Worried, 2500);
  }
  seenMoodSeq = Mood::lastEventSeq();
}

void FaceApp::exit() {
  MusicWatch::setEnabled(false);
  if (asleep) { asleep = false; Backlight::fadeTo(255); }
  if (spr) {
    spr->deleteSprite();
    delete spr;
    spr = nullptr;
  }
}

void FaceApp::onTouch(const TouchEvent& e) {
  switch (e.type) {
    case TouchEvent::TOUCH_DOWN: gotDown = true; break;
    case TouchEvent::TAP: {
      // Is this a real poke, or part of a stroke/rub? Resistive screens often report short
      // strokes as taps, so anything that moved, lasted a while, or follows a pet = petting.
      bool moved = abs(e.dy) > 15 || abs(e.dx) > 25;
      bool slow = e.ms > 350;
      bool rubbing = millis() - lastPetAt < 2500;
      if (moved || slow || rubbing) gotPet = true;
      else { gotTap = true; tapX = e.x; tapY = e.y; }
      break;
    }
    case TouchEvent::SWIPE_UP:
    case TouchEvent::SWIPE_DOWN: gotPet = true; break;  // stroking its head
    default: break;
  }
}

void FaceApp::react(Expression e, uint32_t ms, Bubble b) {
  reactExpr = e;
  reactBubble = b;
  reactUntil = millis() + ms;
  nextReactMs = 0;
}

void FaceApp::fallAsleep(bool dark) {
  asleep = true;
  sleptInDark = dark;
  Backlight::fadeTo(dark ? 8 : 30);
}

void FaceApp::wake(uint32_t now) {
  asleep = false;
  Backlight::fadeTo(255);
  react(Expression::Surprised, 1200);
  nextReactExpr = Expression::Happy;  // ...then happy to see you
  nextReactMs = 2000;
  nextBlink = now + 1500;
}

void FaceApp::handleInput(uint32_t now) {
  if (gotDown && asleep) {
    gotDown = gotTap = gotPet = false;
    wake(now);
    return;
  }
  gotDown = false;

  if (gotTap) {
    gotTap = false;
    tapTimes[tapIdx] = now;
    tapIdx = (tapIdx + 1) % 6;
    uint32_t oldest = tapTimes[tapIdx];  // the tap 6 taps ago
    if (oldest && now - oldest < 2500) {
      Mood::event(Mood::SPAM);
      react(Expression::Angry, 2500, Bubble::Angry);
      memset(tapTimes, 0, sizeof(tapTimes));
    } else {
      Mood::event(Mood::TAP);
      // Look toward the tap and blink
      targetX = constrain((tapX - SCREEN_W / 2) * LOOK_X / (SCREEN_W / 2), -LOOK_X, LOOK_X);
      targetY = constrain((tapY - SCREEN_H / 2) * LOOK_Y / (SCREEN_H / 2), -LOOK_Y, LOOK_Y);
      nextGlance = now + 1800;
      nextBlink = now;
    }
  }

  if (gotPet) {
    gotPet = false;
    bool sameStroke = now - lastPetAt < 600;  // one rub can produce several events: count it once
    lastPetAt = now;
    memset(tapTimes, 0, sizeof(tapTimes));    // petting never adds up to "annoyed"
    if (sameStroke) return;
    petTimes[petIdx] = now;
    petIdx = (petIdx + 1) % 3;
    uint32_t oldest = petTimes[petIdx];  // the pet 3 pets ago
    if (oldest && now - oldest < 8000) {
      Mood::event(Mood::LOVE);
      react(Expression::Love, 3000, Bubble::Heart);
      memset(petTimes, 0, sizeof(petTimes));
      spawnHearts(6);  // big heart burst
    } else {
      Mood::event(Mood::PET);
      react(Expression::Content, 2000);
      spawnHearts(3);
    }
  }
}

void FaceApp::checkSensors(uint32_t now) {
  // WiFi dropped
  bool c = Net::connected();
  if (wasConnected && !c) react(Expression::Worried, 3000, Bubble::WifiOff);
  wasConnected = c;

  // A song just started
  uint32_t ms = MusicWatch::startedAt();
  if (ms != seenMusicStart) {
    seenMusicStart = ms;
    if (asleep) wake(now);
    react(Expression::Surprised, 900, Bubble::Note);
    nextReactExpr = Expression::Happy;
    nextReactMs = 2500;
  }

  // Shake (tilt ball switch rattles open/closed)
#if TILT_PIN >= 0
  if (tiltLast < 0) { pinMode(TILT_PIN, INPUT_PULLUP); tiltLast = digitalRead(TILT_PIN); }
  int t = digitalRead(TILT_PIN);
  if (t != tiltLast) {
    tiltLast = t;
    if (now - tiltWindow > 1200) { tiltWindow = now; tiltToggles = 0; }
    if (++tiltToggles >= 6) {
      tiltToggles = 0;
      Mood::event(Mood::SHAKE);
      if (asleep) wake(now);
      react(Expression::Dizzy, 3000);
    }
  }
#endif

  // Room light (LDR)
#if LIGHT_DARK_THRESHOLD >= 0 || LIGHT_DEBUG
  if (now - lastLightRead > 1000) {
    lastLightRead = now;
    int v = analogRead(LDR_PIN);
  #if LIGHT_DEBUG
    Serial.printf("[light] %d\n", v);
  #endif
  #if LIGHT_DARK_THRESHOLD >= 0
    isDark = LIGHT_DARK_WHEN_ABOVE ? v > LIGHT_DARK_THRESHOLD : v < LIGHT_DARK_THRESHOLD;
    if (isDark) {
      if (!darkSince) darkSince = now;
      if (!asleep && now - darkSince > 15000) fallAsleep(true);
    } else {
      darkSince = 0;
      if (asleep && sleptInDark) {  // lights came back on: yawn awake
        asleep = false;
        Backlight::fadeTo(255);
        react(Expression::Yawn, 2000);
        nextReactExpr = Expression::Happy;
        nextReactMs = 1500;
      }
    }
  #endif
  }
#endif

  // Ignored for too long
  if (!asleep && Mood::idleMs(now) > SLEEP_AFTER_MIN * 60000UL) fallAsleep(false);
}

Expression FaceApp::chooseExpression(uint32_t now, Bubble& b) {
  b = Bubble::None;
  if (asleep) { b = Bubble::Zzz; return Expression::Asleep; }

  // A reaction in progress wins
  if (now < reactUntil) { b = reactBubble; return reactExpr; }
  if (nextReactMs) {  // chained reaction (e.g. surprised -> happy)
    react(nextReactExpr, nextReactMs);
    return reactExpr;
  }

  bool music = MusicWatch::playing();
  if (music) b = Bubble::Note;

  struct tm t;
  bool night = false;
  if (getLocalTime(&t, 0)) {
    night = NIGHT_START_HOUR <= NIGHT_END_HOUR
              ? (t.tm_hour >= NIGHT_START_HOUR && t.tm_hour < NIGHT_END_HOUR)
              : (t.tm_hour >= NIGHT_START_HOUR || t.tm_hour < NIGHT_END_HOUR);
  }
  if (night && !music) {
    if (now > nextYawn) { yawnUntil = now + 2500; nextYawn = now + random(30000, 60000); }
    b = Bubble::Zzz;
    return now < yawnUntil ? Expression::Yawn : Expression::Sleepy;
  }

  uint32_t idle = Mood::idleMs(now);
  if (idle > 7 * 60000UL) return Expression::Sad;
  if (idle > 3 * 60000UL) return Expression::Bored;
  if (music) return Expression::Happy;

  WeatherData wd;
  int code = WeatherService::get(wd) ? wd.code : -1;
  bool rainy = (code >= 51 && code <= 67) || (code >= 80 && code <= 82) || code >= 95;
  if (rainy) { b = Bubble::Rain; return Expression::Neutral; }
  if (code == 0 || code == 1) b = Bubble::Sun;

  int h = Mood::happiness();
  if (h >= 45) return Expression::Happy;
  if (h >= 25) return Expression::Neutral;
  return Expression::Sad;
}

void FaceApp::update(TFT_eSPI& tft, uint32_t now) {
  handleInput(now);
  if (now - lastFrame < 33) return;  // ~30 fps
  lastFrame = now;
  checkSensors(now);

  idleFun(now);
  Bubble b;
  expr = chooseExpression(now, b);
  updateHearts();
  showZees = expr == Expression::Asleep || expr == Expression::Sleepy || expr == Expression::Yawn ||
             expr == Expression::Bored;
  updateZees(now);
  bubble = b;
  if (bubble != drawnBubble) drawBubble(tft, bubble);

  // Glance around every few seconds (not while asleep)
  if (!asleep && now > nextGlance) {
    targetX = random(-LOOK_X, LOOK_X + 1);
    targetY = random(-LOOK_Y, LOOK_Y + 1);
    if (random(3) == 0) targetX = targetY = 0;  // often look straight ahead
    nextGlance = now + random(1500, 4500);
  }
  if (asleep) targetX = targetY = 0;
  lookX += (targetX - lookX) * 0.25f;
  lookY += (targetY - lookY) * 0.25f;

  // Blink
  float open = 1.0f;
  if (!blinking && now > nextBlink && !asleep) { blinking = true; blinkStart = now; }
  if (blinking) {
    uint32_t t = now - blinkStart;
    if (t >= (uint32_t)BLINK_MS) {
      blinking = false;
      nextBlink = now + random(2000, 6000);
    } else {
      open = 1.0f - 0.9f * sinf(PI * t / BLINK_MS);
    }
  }

  // Movement: dance when music plays, slow breathing when asleep, gentle float otherwise
  int ox = (int)lookX, oy = (int)lookY;
  dancing = !asleep && MusicWatch::playing() && now >= reactUntil;
  if (dancing) {
    updateDance(now, ox, oy);
  } else {
    tilt = 0;
    squash = 1.0f;
    for (auto& n : notes) n.alive = false;
    if (asleep) oy += 6 + (int)(sinf(now / 1400.0f) * 3);
    else oy += (int)(sinf(now / 600.0f) * 2);
  }

  if (spr) {
    render(*spr, ox, oy, open, now);
    spr->pushSprite(SPR_X, SPR_Y);
  } else {
    tft.setViewport(SPR_X, SPR_Y, SPR_W, SPR_H);
    render(tft, ox, oy, open, now);
    tft.resetViewport();
  }
}

// ---------- idle fun: random emotes + accessories ----------
void FaceApp::idleFun(uint32_t now) {
  if (accessory != Accessory::None && now > accessoryUntil) accessory = Accessory::None;

  bool resting = !asleep && !MusicWatch::playing() && now >= reactUntil && nextReactMs == 0 &&
                 Mood::idleMs(now) < 3 * 60000UL;   // not when bored / sad / asleep
  if (!resting) {
    if (asleep) accessory = Accessory::None;
    return;
  }
  if (now < nextIdleFun) return;
  nextIdleFun = now + random(15000, 40000);

  if (random(100) < 55) {
    // Quick emote
    static const Expression EMOTES[] = {Expression::Wink, Expression::Content, Expression::Surprised,
                                        Expression::Love, Expression::Sing};
    static const uint16_t DURATION[] = {1200, 1600, 900, 1500, 1400};
    int i = random(sizeof(EMOTES) / sizeof(EMOTES[0]));
    react(EMOTES[i], DURATION[i]);
  } else if (accessory == Accessory::None) {
    // Put something on for a little while
    accessory = (Accessory)random(1, 6);
    accessoryUntil = now + random(6000, 11000);
    react(Expression::Surprised, 500);  // "ooh!"
    nextReactExpr = Expression::Happy;
    nextReactMs = 1200;
  }
}

static void star(TFT_eSPI& g, int x, int y, int r, uint16_t c) {
  g.fillTriangle(x, y - r, x - r * 3 / 10, y, x + r * 3 / 10, y, c);
  g.fillTriangle(x, y + r, x - r * 3 / 10, y, x + r * 3 / 10, y, c);
  g.fillTriangle(x - r, y, x, y - r * 3 / 10, x, y + r * 3 / 10, c);
  g.fillTriangle(x + r, y, x, y - r * 3 / 10, x, y + r * 3 / 10, c);
}

void FaceApp::drawAccessory(TFT_eSPI& g, int lx, int rx, int ey, int mx) {
  const uint16_t GOLD = 0xFEA0, GOLD_DARK = 0xC4A0, BLACK = 0x0861, WHITE = 0xFFFF, YELLOW = 0xFFE0;
  int top = ey - EYE_H / 2 - 6;  // just above the eyes

  switch (accessory) {
    case Accessory::Bow: {  // pink bow above the right eye, tilted for extra cuteness
      int bx = rx + 14, by = top - 9;
      const float a = 0.35f;   // ~20 degrees, tilted clockwise
      const float ca = cosf(a), sa = sinf(a);
      auto R = [&](int dx, int dy, int& ox2, int& oy2) {
        ox2 = bx + (int)(dx * ca - dy * sa);
        oy2 = by + (int)(dx * sa + dy * ca);
      };
      int x1, y1, x2, y2;
      R(-22, -13, x1, y1); R(-22, 13, x2, y2); g.fillTriangle(bx, by, x1, y1, x2, y2, LOVE);
      R(22, -13, x1, y1);  R(22, 13, x2, y2);  g.fillTriangle(bx, by, x1, y1, x2, y2, LOVE);
      R(-15, -7, x1, y1);  R(-15, 7, x2, y2);  g.fillTriangle(bx, by, x1, y1, x2, y2, LOVE_GLOW);
      R(15, -7, x1, y1);   R(15, 7, x2, y2);   g.fillTriangle(bx, by, x1, y1, x2, y2, LOVE_GLOW);
      g.fillCircle(bx, by, 6, LOVE);
      g.fillCircle(bx - 2, by - 2, 2, WHITE);
      break;
    }
    case Accessory::Halo: {  // glowing gold ring floating high above the head
      int hy = max(7, top - 17) + (int)(sinf(millis() / 350.0f) * 2);
      g.fillEllipse(mx, hy, 28, 7, GOLD_DARK);   // soft glow
      g.fillEllipse(mx, hy, 25, 5, GOLD);
      g.fillEllipse(mx, hy, 19, 2, VISOR);       // hollow middle
      star(g, mx + 31, hy - 4, 4, WHITE);        // sparkle
      break;
    }
    case Accessory::Sunglasses: {  // replaces the eyes (they're skipped in render)
      for (int x : {lx, rx}) {
        g.fillRoundRect(x - 26, ey - 18, 52, 34, 12, BLACK);
        g.drawWideLine(x - 16, ey - 9, x - 6, ey - 9, 3, 0x632C, BLACK);  // shine
        g.drawWideLine(x - 16, ey - 3, x - 12, ey - 3, 3, 0x632C, BLACK);
      }
      g.fillRect(lx + 26, ey - 14, rx - lx - 52, 5, BLACK);  // bridge
      break;
    }
    case Accessory::Crown: {  // little gold crown on top
      int cx = mx, base = top - 2, h = 20;
      g.fillRect(cx - 22, base - 8, 44, 8, GOLD);
      g.fillTriangle(cx - 22, base - 8, cx - 12, base - 8, cx - 22, base - h, GOLD);
      g.fillTriangle(cx - 8, base - 8, cx + 8, base - 8, cx, base - h - 2, GOLD);
      g.fillTriangle(cx + 12, base - 8, cx + 22, base - 8, cx + 22, base - h, GOLD);
      g.drawFastHLine(cx - 22, base - 1, 44, GOLD_DARK);
      g.fillCircle(cx, base - 4, 3, LOVE);
      g.fillCircle(cx - 14, base - 4, 2, EYE);
      g.fillCircle(cx + 14, base - 4, 2, EYE);
      star(g, cx + 28, base - h, 5, WHITE);  // sparkle
      break;
    }
    case Accessory::Flower: {  // flower tucked above the left eye
      int fx = lx - 16, fy = top - 8;
      for (int a = 0; a < 360; a += 72) {
        float r = a * DEG_TO_RAD;
        g.fillCircle(fx + cosf(r) * 9, fy + sinf(r) * 9, 7, LOVE);
      }
      g.fillCircle(fx, fy, 6, YELLOW);
      g.fillCircle(fx - 2, fy - 2, 2, WHITE);
      break;
    }
    default: break;
  }
}

// ---------- floating z's (asleep / sleepy / bored) ----------
void FaceApp::updateZees(uint32_t now) {
  if (showZees && now > nextZee) {
    nextZee = now + 1100;
    for (auto& z : zees) {
      if (z.alive) continue;
      z.x = SPR_W / 2 + EYE_GAP / 2 + 26;  // beside the right eye
      z.y = EYE_Y + 4;
      z.age = 0;
      z.alive = true;
      break;
    }
  }
  for (auto& z : zees) {
    if (!z.alive) continue;
    z.y -= 0.9f;
    z.x += 0.45f + sinf(z.age * 0.15f) * 0.5f;  // drift up and to the right, swaying
    if (++z.age > 75 || z.y < 8 || z.x > SPR_W - 8) z.alive = false;
  }
}

// ---------- hearts (petting) ----------
void FaceApp::spawnHearts(int count) {
  for (auto& h : hearts) {
    if (count <= 0) break;
    if (h.alive) continue;
    // pop out from around the face: left side, right side, or above
    int zone = random(3);
    if (zone == 0)      { h.x = random(10, 50);             h.y = random(40, 110); }
    else if (zone == 1) { h.x = random(SPR_W - 50, SPR_W - 10); h.y = random(40, 110); }
    else                { h.x = random(60, SPR_W - 60);     h.y = random(20, 40); }
    h.vx = (random(-10, 11)) / 20.0f;
    h.size = random(6, 10);
    h.age = 0;
    h.alive = true;
    count--;
  }
}

void FaceApp::updateHearts() {
  for (auto& h : hearts) {
    if (!h.alive) continue;
    h.y -= 1.3f;
    h.x += h.vx + sinf(h.age * 0.3f) * 0.6f;
    if (++h.age > 45 || h.y < 4) h.alive = false;
  }
}

// ---------- dancing ----------
// There's no tempo info from Spotify for new apps, so we groove at a steady ~118 BPM.
static const uint32_t BEAT_MS = 508;

void FaceApp::updateDance(uint32_t now, int& ox, int& oy) {
  uint32_t beat = now / BEAT_MS;
  float t = (now % BEAT_MS) / (float)BEAT_MS;       // 0..1 through the current beat

  // Bounce: hop up between beats, land on the beat
  oy += -(int)(sinf(PI * t) * 9) + 2;
  // Sway: one full left-right swing every 2 beats
  ox += (int)(sinf(2 * PI * (now % (BEAT_MS * 2)) / (BEAT_MS * 2.0f)) * 14);
  // Head tilt: alternate every 2 beats, eased
  float tiltTarget = ((beat / 2) % 2 == 0) ? 5.0f : -5.0f;
  tilt += (tiltTarget - tilt) * 0.2f;
  // Squash when landing on the beat
  squash = t < 0.15f ? 1.0f + (0.15f - t) * 1.6f : 1.0f;

  // Dance moves: change every 4 beats
  static const Expression MOVES[] = {Expression::Happy, Expression::Content, Expression::Sing, Expression::Happy,
                                     Expression::Wink, Expression::Content, Expression::Sing, Expression::Love};
  expr = MOVES[(beat / 4) % (sizeof(MOVES) / sizeof(MOVES[0]))];

  // Spawn a floating music note on every other beat
  if (beat != lastBeat) {
    lastBeat = beat;
    if (beat % 2 == 0) {
      for (auto& n : notes) {
        if (n.alive) continue;
        bool left = random(2);
        n.x = left ? random(8, 40) : random(SPR_W - 40, SPR_W - 8);
        n.y = SPR_H - 20;
        n.age = 0;
        n.alive = true;
        break;
      }
    }
  }
  for (auto& n : notes) {
    if (!n.alive) continue;
    n.y -= 2.2f;
    n.x += sinf((n.age + n.x) * 0.25f) * 0.8f;  // wiggle as it rises
    if (++n.age > 50 || n.y < 6) n.alive = false;
  }
}

// Thought bubble above the face (outside the sprite, so it's drawn only when it changes)
void FaceApp::drawBubble(TFT_eSPI& tft, Bubble b) {
  drawnBubble = b;
  const int bx = 238, by = 21;
  tft.fillRect(150, 0, SCREEN_W - 150, SPR_Y, VISOR);
  if (b == Bubble::None) return;

  tft.fillCircle(212, 38, 4, COL_PANEL);
  tft.fillCircle(201, 42, 2, COL_PANEL);
  tft.fillCircle(bx, by, 19, COL_PANEL);

  switch (b) {
    case Bubble::Note:
      miniNote(tft, bx - 4, by, EYE);
      miniNote(tft, bx + 7, by - 4, EYE_GLOW);
      break;
    case Bubble::Zzz:
      tft.setTextColor(EYE);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("z", bx - 7, by + 5, 2);
      tft.drawString("Z", bx + 4, by - 3, 4);
      break;
    case Bubble::Rain:
      tft.fillCircle(bx - 6, by - 2, 6, 0xBDF7);
      tft.fillCircle(bx + 4, by - 5, 8, 0xBDF7);
      tft.fillRoundRect(bx - 12, by - 3, 24, 8, 4, 0xBDF7);
      for (int i = -1; i <= 1; i++) tft.drawWideLine(bx + i * 7 + 2, by + 8, bx + i * 7, by + 13, 2, 0x3D9F, COL_PANEL);
      break;
    case Bubble::Heart:
      heart(tft, bx, by - 3, 7, LOVE);
      break;
    case Bubble::WifiOff:
      tft.fillCircle(bx, by + 8, 2, COL_DIM);
      thickArc(tft, bx, by + 8, 7, 225, 315, 3, COL_DIM);
      thickArc(tft, bx, by + 8, 13, 225, 315, 3, COL_DIM);
      tft.drawWideLine(bx - 10, by - 10, bx + 10, by + 10, 3, ANGRY, COL_PANEL);
      break;
    case Bubble::Angry:
      tft.setTextColor(ANGRY);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("!!", bx, by + 1, 4);
      break;
    case Bubble::Sun:
      for (int a = 0; a < 360; a += 45) {
        float r = a * DEG_TO_RAD;
        tft.drawWideLine(bx + cosf(r) * 9, by + sinf(r) * 9, bx + cosf(r) * 13, by + sinf(r) * 13, 2, TFT_YELLOW, COL_PANEL);
      }
      tft.fillCircle(bx, by, 6, TFT_YELLOW);
      break;
    default: break;
  }
}

void FaceApp::render(TFT_eSPI& g, int ox, int oy, float eyeOpen, uint32_t now) {
  const ExprDef& d = EXPRESSIONS[(uint8_t)expr];

  uint16_t col = EYE, glow = EYE_GLOW;
  if (d.palette == Palette::Red)  { col = ANGRY; glow = ANGRY_GLOW; }
  if (d.palette == Palette::Pink) { col = LOVE;  glow = LOVE_GLOW;  }

  g.fillRect(0, 0, SPR_W, SPR_H, VISOR);

  // Floating music notes (behind the face)
  if (dancing) {
    for (const auto& n : notes) {
      if (!n.alive) continue;
      uint16_t nc = n.age < 30 ? EYE : EYE_GLOW;  // fade as they rise
      miniNote(g, (int)n.x, (int)n.y, nc);
    }
  }

  int lx = SPR_W / 2 - EYE_GAP / 2 + ox;
  int rx = SPR_W / 2 + EYE_GAP / 2 + ox;
  int ey = EYE_Y + oy;
  int ti = (int)tilt;  // head tilt: one eye higher than the other
  float open = eyeOpen * d.openPct / 100.0f;

  bool shades = accessory == Accessory::Sunglasses && !dancing;
  eyeSquash = squash;
  if (!shades) {
    drawEye(g, d.left,  lx, ey + ti, open, d.bigEyes, col, glow);
    drawEye(g, d.right, rx, ey - ti, open, d.bigEyes, col, glow);
  }
  eyeSquash = 1.0f;

  // Brows
  int by = ey - EYE_H / 2 - 12;
  if (d.brow == Brow::Worried) {        // inner ends raised
    thickLine(g, lx - 16, by + 4, lx + 14, by - 6, LINE_W - 2, col);
    thickLine(g, rx - 14, by - 6, rx + 16, by + 4, LINE_W - 2, col);
  } else if (d.brow == Brow::Angry) {   // inner ends lowered
    thickLine(g, lx - 16, by - 6, lx + 14, by + 6, LINE_W - 2, col);
    thickLine(g, rx - 14, by + 6, rx + 16, by - 6, LINE_W - 2, col);
  }

  // Mouth (moves half as much as the eyes, which reads as depth)
  int mx = SPR_W / 2 + ox / 2;
  int my = MOUTH_Y + oy / 2;
  switch (d.mouth) {
    case Mouth::None: break;
    case Mouth::Smile:      thickArc(g, mx, my - 12, 20, 30, 150, LINE_W, col); break;
    case Mouth::SmallSmile: thickArc(g, mx, my - 8, 12, 30, 150, LINE_W - 2, col); break;
    case Mouth::Frown:      thickArc(g, mx, my + 14, 16, 210, 330, LINE_W - 2, col); break;
    case Mouth::O:
      g.fillEllipse(mx, my, 11, 13, col);
      g.fillEllipse(mx, my, 5, 7, VISOR);
      break;
    case Mouth::Flat:       thickLine(g, mx - 16, my, mx + 16, my, LINE_W - 2, col); break;
  }

  if (accessory != Accessory::None && !dancing) drawAccessory(g, lx, rx, ey, SPR_W / 2 + ox);

  // Petting hearts: pop in big, then shrink and fade as they float up
  for (const auto& h : hearts) {
    if (!h.alive) continue;
    int s = h.age < 4 ? h.size + 3 - h.age : (h.age > 32 ? max(2, h.size - (h.age - 32) / 2) : h.size);
    heart(g, (int)h.x, (int)h.y, s / 2 + 2, h.age > 32 ? LOVE_GLOW : LOVE);
  }

  // Floating z's: small "z" that grows into a big "Z" as it rises, fading out
  g.setTextDatum(MC_DATUM);
  for (const auto& z : zees) {
    if (!z.alive) continue;
    uint16_t zc = z.age < 45 ? EYE : (z.age < 62 ? EYE_GLOW : 0x1129);
    g.setTextColor(zc);
    g.drawString(z.age < 25 ? "z" : "Z", (int)z.x, (int)z.y, z.age < 50 ? 2 : 4);
  }
}
