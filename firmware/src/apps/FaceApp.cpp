#include "apps/FaceApp.h"
#include "apps/FaceStyle.h"

using namespace FaceStyle;

// ---------- building blocks ----------
enum class EyeShape : uint8_t { Oval, ClosedArc, Line, Heart };
enum class Brow     : uint8_t { None, Worried, Angry };
enum class Mouth    : uint8_t { None, Smile, SmallSmile, Frown, O, Flat };
enum class Palette  : uint8_t { Cyan, Red, Pink };

struct ExprDef {
  EyeShape left, right;
  Brow brow;
  Mouth mouth;
  Palette palette;
  bool bigEyes;
};

// One row per Expression (same order as the enum). Add your own here!
static const ExprDef EXPRESSIONS[] = {
  /* Neutral   */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::None,       Palette::Cyan, false},
  /* Happy     */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::Smile,      Palette::Cyan, false},
  /* Wink      */ {EyeShape::Oval,      EyeShape::ClosedArc, Brow::None,    Mouth::SmallSmile, Palette::Cyan, false},
  /* Worried   */ {EyeShape::Oval,      EyeShape::Oval,      Brow::Worried, Mouth::Frown,      Palette::Cyan, false},
  /* Angry     */ {EyeShape::Oval,      EyeShape::Oval,      Brow::Angry,   Mouth::Frown,      Palette::Red,  false},
  /* Love      */ {EyeShape::Heart,     EyeShape::Heart,     Brow::None,    Mouth::SmallSmile, Palette::Pink, false},
  /* Surprised */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::O,          Palette::Cyan, true },
  /* Bored     */ {EyeShape::Line,      EyeShape::Line,      Brow::None,    Mouth::Flat,       Palette::Cyan, false},
  /* Content   */ {EyeShape::ClosedArc, EyeShape::ClosedArc, Brow::None,    Mouth::SmallSmile, Palette::Cyan, false},
  /* Sad       */ {EyeShape::Oval,      EyeShape::Oval,      Brow::None,    Mouth::Frown,      Palette::Cyan, false},
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

static void drawEye(TFT_eSPI& g, EyeShape s, int cx, int cy, float open,
                    bool big, uint16_t col, uint16_t glow) {
  switch (s) {
    case EyeShape::Oval: {
      float scale = big ? 1.2f : 1.0f;
      int rx = (int)(EYE_W / 2 * scale);
      int ry = max(3, (int)(EYE_H / 2 * scale * open));
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
  }
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
  lastFrame = 0;
}

void FaceApp::exit() {
  if (spr) {
    spr->deleteSprite();
    delete spr;
    spr = nullptr;
  }
}

void FaceApp::onTouch(const TouchEvent& e) {
  // For now: tap cycles through expressions so you can preview them.
  // Later the mood engine will set these from events (music, shakes, etc.).
  if (e.type == TouchEvent::TAP) {
    expr = (Expression)(((uint8_t)expr + 1) % (uint8_t)Expression::COUNT);
    nextBlink = millis() + 1200;
  }
}

void FaceApp::update(TFT_eSPI& tft, uint32_t now) {
  if (now - lastFrame < 33) return;  // ~30 fps
  lastFrame = now;

  // Glance around every few seconds
  if (now > nextGlance) {
    targetX = random(-LOOK_X, LOOK_X + 1);
    targetY = random(-LOOK_Y, LOOK_Y + 1);
    if (random(3) == 0) targetX = targetY = 0;  // often look straight ahead
    nextGlance = now + random(1500, 4500);
  }
  lookX += (targetX - lookX) * 0.25f;
  lookY += (targetY - lookY) * 0.25f;

  // Blink
  float open = 1.0f;
  if (!blinking && now > nextBlink) { blinking = true; blinkStart = now; }
  if (blinking) {
    uint32_t t = now - blinkStart;
    if (t >= (uint32_t)BLINK_MS) {
      blinking = false;
      nextBlink = now + random(2000, 6000);
    } else {
      open = 1.0f - 0.9f * sinf(PI * t / BLINK_MS);
    }
  }

  int bob = (int)(sinf(now / 600.0f) * 2);  // gentle floating

  if (spr) {
    render(*spr, (int)lookX, (int)lookY + bob, open);
    spr->pushSprite(SPR_X, SPR_Y);
  } else {
    tft.setViewport(SPR_X, SPR_Y, SPR_W, SPR_H);
    render(tft, (int)lookX, (int)lookY + bob, open);
    tft.resetViewport();
  }
}

void FaceApp::render(TFT_eSPI& g, int ox, int oy, float eyeOpen) {
  const ExprDef& d = EXPRESSIONS[(uint8_t)expr];

  uint16_t col = EYE, glow = EYE_GLOW;
  if (d.palette == Palette::Red)  { col = ANGRY; glow = ANGRY_GLOW; }
  if (d.palette == Palette::Pink) { col = LOVE;  glow = LOVE_GLOW;  }

  g.fillRect(0, 0, SPR_W, SPR_H, VISOR);

  int lx = SPR_W / 2 - EYE_GAP / 2 + ox;
  int rx = SPR_W / 2 + EYE_GAP / 2 + ox;
  int ey = EYE_Y + oy;

  drawEye(g, d.left,  lx, ey, eyeOpen, d.bigEyes, col, glow);
  drawEye(g, d.right, rx, ey, eyeOpen, d.bigEyes, col, glow);

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
}
