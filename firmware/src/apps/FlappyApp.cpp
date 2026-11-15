#include "apps/FlappyApp.h"
#include "core/Scores.h"
#include "core/Mood.h"

// ---- Tuning knobs (per 33 ms tick) ----
static const float GRAVITY   = 0.38f;
static const float FLAP_VY   = -5.6f;
static const float SPEED     = 2.6f;   // pipe scroll speed
static const int   PIPE_W    = 42;
static const int   GAP       = 82;     // vertical opening
static const int   SPACING   = 160;    // distance between pipes
static const int   GROUND_Y  = 222;
static const int   BOT_X     = 80;
static const int   BOT_HW    = 11, BOT_HH = 9;  // hitbox half-size
static const uint32_t TICK_MS = 33;

// ---- Colors ----
static const uint16_t SKY     = 0x1108;  // #102040
static const uint16_t STAR    = 0x8410;
static const uint16_t PIPE    = 0x25B5;  // #25B5A8
static const uint16_t PIPE_LIP = COL_EYE;
static const uint16_t PIPE_SHADE = COL_GLOW;
static const uint16_t GROUND  = 0x2968;
static const uint16_t BOT_BODY = 0xEF7D;
static const uint16_t FLAME   = 0xFD20;

static int randomGapY() { return random(40 + GAP / 2, GROUND_Y - 30 - GAP / 2); }

void FlappyApp::reset() {
  state = READY;
  y = 110;
  vy = 0;
  score = 0;
  newBest = false;
  flame = 0;
  for (int i = 0; i < NUM_PIPES; i++) pipes[i] = {(float)(SCREEN_W + 60 + i * SPACING), randomGapY(), false};
}

void FlappyApp::enter(TFT_eSPI& tft) {
  if (!initialized) {
    for (int i = 0; i < NUM_STARS; i++) stars[i] = {(float)random(SCREEN_W), (uint8_t)random(GROUND_Y - 10)};
    reset();
    initialized = true;
  }
  best = Scores::best("flappy");

  // Full-screen 8-bit sprite = 77 KB. Fall back to drawing directly if RAM is tight.
  spr = new TFT_eSprite(&tft);
  spr->setColorDepth(8);
  if (!spr->createSprite(SCREEN_W, SCREEN_H)) {
    delete spr;
    spr = nullptr;
    Serial.println("[flappy] not enough RAM for sprite, drawing directly");
  }

  if (state == PLAYING) state = PAUSED;
  if (state == DEAD) reset();
  lastTick = millis();
  present(tft);
}

void FlappyApp::exit() {
  if (spr) {
    spr->deleteSprite();
    delete spr;
    spr = nullptr;
  }
}

void FlappyApp::resume(TFT_eSPI& tft) {
  // Back from the app wheel: the sprite is still allocated, just pause and redraw
  if (state == PLAYING) state = PAUSED;
  lastTick = millis();
  present(tft);
}

void FlappyApp::onTouch(const TouchEvent& e) {
  if (e.type != TouchEvent::TOUCH_DOWN) return;  // react the instant you touch
  switch (state) {
    case READY:
    case PAUSED:
      state = PLAYING;
      flapRequested = true;
      break;
    case PLAYING:
      flapRequested = true;
      break;
    case DEAD:
      if (millis() - deadAt > 600) reset();  // short delay so a panic-tap doesn't restart
      break;
  }
}

void FlappyApp::die() {
  state = DEAD;
  deadAt = millis();
  if (Scores::submit("flappy", score)) {
    best = score;
    newBest = true;
    Mood::event(Mood::HIGH_SCORE);
  } else {
    Mood::event(Mood::GAME_LOST);
  }
}

void FlappyApp::tick() {
  frame++;

  // Stars drift slowly (parallax)
  if (state != PAUSED) {
    for (int i = 0; i < NUM_STARS; i++) {
      stars[i].x -= 0.4f;
      if (stars[i].x < 0) stars[i].x += SCREEN_W;
    }
  }

  if (state == READY) {
    y = 110 + sinf(frame * 0.12f) * 6;  // hover
    groundOff = fmodf(groundOff + SPEED, 16);
    return;
  }
  if (state == PAUSED) return;

  if (state == DEAD) {  // tumble to the ground
    if (y + BOT_HH < GROUND_Y) {
      vy += GRAVITY;
      y = fminf(y + vy, (float)(GROUND_Y - BOT_HH));
    }
    return;
  }

  // PLAYING
  if (flapRequested) {
    flapRequested = false;
    vy = FLAP_VY;
    flame = 6;
  }
  vy += GRAVITY;
  y += vy;
  if (y < BOT_HH) { y = BOT_HH; vy = 0; }
  if (flame > 0) flame--;
  groundOff = fmodf(groundOff + SPEED, 16);

  for (int i = 0; i < NUM_PIPES; i++) {
    Pipe& p = pipes[i];
    p.x -= SPEED;
    if (p.x < -PIPE_W) {  // recycle to the back of the line
      p.x += NUM_PIPES * SPACING;
      p.gapY = randomGapY();
      p.scored = false;
    }
    if (!p.scored && p.x + PIPE_W < BOT_X - BOT_HW) {
      p.scored = true;
      score++;
    }
    // Collision
    bool overlapX = BOT_X + BOT_HW > p.x && BOT_X - BOT_HW < p.x + PIPE_W;
    bool outsideGap = y - BOT_HH < p.gapY - GAP / 2 || y + BOT_HH > p.gapY + GAP / 2;
    if (overlapX && outsideGap) { die(); return; }
  }

  if (y + BOT_HH >= GROUND_Y) {
    y = GROUND_Y - BOT_HH;
    die();
  }
}

void FlappyApp::update(TFT_eSPI& tft, uint32_t now) {
  if (now - lastTick > 250) lastTick = now - TICK_MS;  // don't fast-forward after a hiccup
  bool ticked = false;
  while (now - lastTick >= TICK_MS) {
    tick();
    lastTick += TICK_MS;
    ticked = true;
  }
  if (ticked) present(tft);
}

void FlappyApp::present(TFT_eSPI& tft) {
  if (spr) {
    render(*spr);
    spr->pushSprite(0, 0);
  } else {
    render(tft);
  }
}

// ---------- drawing ----------
void FlappyApp::drawBot(TFT_eSPI& g, int x, int y, bool dead, bool thrust) {
  // Jetpack flame
  if (thrust) {
    int len = 8 + (frame % 3) * 2;
    g.fillTriangle(x - 6, y + BOT_HH, x + 6, y + BOT_HH, x, y + BOT_HH + len, FLAME);
    g.fillTriangle(x - 3, y + BOT_HH, x + 3, y + BOT_HH, x, y + BOT_HH + len / 2, TFT_YELLOW);
  }
  // Antenna
  g.drawLine(x, y - 10, x, y - 15, 0xBDF7);
  g.fillCircle(x, y - 16, 2, dead ? STAR : COL_PINK);
  // Ears
  g.fillRect(x - 14, y - 4, 3, 8, 0xBDF7);
  g.fillRect(x + 12, y - 4, 3, 8, 0xBDF7);
  // Head + visor
  g.fillRoundRect(x - 12, y - 10, 24, 20, 6, BOT_BODY);
  g.fillRoundRect(x - 9, y - 7, 18, 13, 4, COL_VISOR);
  // Eyes
  if (dead) {
    g.drawLine(x - 7, y - 4, x - 3, y + 1, COL_PINK);
    g.drawLine(x - 7, y + 1, x - 3, y - 4, COL_PINK);
    g.drawLine(x + 3, y - 4, x + 7, y + 1, COL_PINK);
    g.drawLine(x + 3, y + 1, x + 7, y - 4, COL_PINK);
  } else {
    int look = vy < -1 ? -1 : (vy > 3 ? 1 : 0);  // eyes glance up/down with motion
    g.fillRoundRect(x - 6, y - 4 + look, 4, 6, 1, COL_EYE);
    g.fillRoundRect(x + 2, y - 4 + look, 4, 6, 1, COL_EYE);
  }
}

void FlappyApp::render(TFT_eSPI& g) {
  g.fillRect(0, 0, SCREEN_W, GROUND_Y, SKY);

  for (int i = 0; i < NUM_STARS; i++) g.fillRect((int)stars[i].x, stars[i].y, (i % 4 == 0) ? 2 : 1, (i % 4 == 0) ? 2 : 1, STAR);

  // Pipes
  for (int i = 0; i < NUM_PIPES; i++) {
    const Pipe& p = pipes[i];
    int x = (int)p.x;
    if (x > SCREEN_W || x + PIPE_W < -4) continue;
    int top = p.gapY - GAP / 2, bot = p.gapY + GAP / 2;
    g.fillRect(x, 0, PIPE_W, top, PIPE);
    g.fillRect(x + 6, 0, 5, top, PIPE_SHADE);
    g.fillRect(x - 3, top - 10, PIPE_W + 6, 10, PIPE_LIP);
    g.fillRect(x, bot, PIPE_W, GROUND_Y - bot, PIPE);
    g.fillRect(x + 6, bot, 5, GROUND_Y - bot, PIPE_SHADE);
    g.fillRect(x - 3, bot, PIPE_W + 6, 10, PIPE_LIP);
  }

  // Ground
  g.fillRect(0, GROUND_Y, SCREEN_W, SCREEN_H - GROUND_Y, GROUND);
  g.drawFastHLine(0, GROUND_Y, SCREEN_W, COL_GLOW);
  for (int x = -(int)groundOff; x < SCREEN_W; x += 16) g.fillRect(x, GROUND_Y + 8, 8, 3, 0x4A69);

  drawBot(g, BOT_X, (int)y, state == DEAD, flame > 0 && state == PLAYING);

  // Text overlays
  g.setTextDatum(TC_DATUM);
  g.setTextColor(COL_FG);
  if (state == PLAYING || state == PAUSED) g.drawNumber(score, SCREEN_W / 2, 10, 4);

  if (state == READY) {
    g.setTextColor(COL_EYE);
    g.drawString("Flappy Bot", SCREEN_W / 2 + 30, 60, 4);
    g.setTextColor(COL_FG);
    g.drawString("tap to fly", SCREEN_W / 2 + 30, 92, 2);
    g.setTextColor(COL_DIM);
    g.drawString("best " + String(best) + "  -  hold for menu", SCREEN_W / 2 + 30, 112, 2);
  } else if (state == PAUSED) {
    g.drawString("Paused - tap to continue", SCREEN_W / 2, 44, 2);
  } else if (state == DEAD) {
    g.fillRoundRect(80, 60, 160, 100, 10, COL_VISOR);
    g.drawRoundRect(80, 60, 160, 100, 10, COL_EYE);
    g.setTextColor(COL_FG);
    g.drawString("Game over", SCREEN_W / 2, 70, 4);
    g.setTextColor(newBest ? COL_PINK : COL_DIM);
    g.drawString(newBest ? "NEW BEST!" : ("best " + String(best)).c_str(), SCREEN_W / 2, 100, 2);
    g.setTextColor(COL_FG);
    g.drawString("score " + String(score), SCREEN_W / 2, 118, 2);
    if (millis() - deadAt > 600) {
      g.setTextColor(COL_DIM);
      g.drawString("tap to retry", SCREEN_W / 2, 138, 2);
    }
  }
}

void FlappyApp::drawIcon(TFT_eSPI& tft, int cx, int cy) {
  tft.fillTriangle(cx - 4, cy + 8, cx + 4, cy + 8, cx, cy + 16, FLAME);
  tft.fillRoundRect(cx - 11, cy - 10, 22, 18, 5, BOT_BODY);
  tft.fillRoundRect(cx - 8, cy - 7, 16, 11, 3, COL_VISOR);
  tft.fillRect(cx - 5, cy - 4, 3, 5, COL_EYE);
  tft.fillRect(cx + 2, cy - 4, 3, 5, COL_EYE);
  tft.drawLine(cx, cy - 10, cx, cy - 14, 0xBDF7);
  tft.fillCircle(cx, cy - 15, 2, COL_PINK);
}
