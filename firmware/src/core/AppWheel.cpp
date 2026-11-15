#include "core/AppWheel.h"

// Layout
static const int CX = SCREEN_W / 2, CY = SCREEN_H / 2;
static const int RING_R = 84;     // distance from center to each app button
static const int ITEM_R = 27;     // app button radius
static const int CENTER_R = 36;   // robot face button radius

// Small thick arc made of dots (screen angles: 0 = right, 90 = down)
static void dotArc(TFT_eSPI& g, int cx, int cy, int r, int a0, int a1, int w, uint16_t c) {
  for (int a = a0; a <= a1; a += 8) {
    float rad = a * DEG_TO_RAD;
    g.fillCircle(cx + r * cosf(rad), cy + r * sinf(rad), w, c);
  }
}

void AppWheel::setApps(App** items, int n) {
  mainCount = min(n, MAX_ITEMS - 1);  // leave room for the Games folder
  for (int i = 0; i < mainCount; i++) mainItems[i] = items[i];
}

void AppWheel::setGames(App** items, int n) {
  gameCount = min(n, MAX_ITEMS - 1);  // leave room for the Back button
  for (int i = 0; i < gameCount; i++) gameItems[i] = items[i];
}

// Main ring: apps + Games folder.  Games ring: games + Back.
int AppWheel::slotCount() const {
  if (inGames) return gameCount + 1;
  return mainCount + (gameCount > 0 ? 1 : 0);
}

void AppWheel::slotPos(int i, int& x, int& y) const {
  float a = (-90.0f + 360.0f * i / slotCount()) * DEG_TO_RAD;  // start at the top, go clockwise
  x = CX + (int)(RING_R * cosf(a));
  y = CY + (int)(RING_R * sinf(a));
}

void AppWheel::open(TFT_eSPI& tft, App* cur) {
  open_ = true;
  current = cur;
  inGames = cur && cur->isGame();  // open straight into the games ring from a game
  drawRing(tft, true);
  nextBlink = millis() + 2500;
  blinking = false;
}

void AppWheel::drawRing(TFT_eSPI& tft, bool animate) {
  tft.fillScreen(COL_VISOR);

  // Faint orbit line
  tft.drawCircle(CX, CY, RING_R, COL_PANEL);

  // Corner labels
  tft.setTextColor(COL_DIM);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(inGames ? "GAMES" : "APPS", 8, 8, 2);
  tft.setTextDatum(BR_DATUM);
  tft.drawString("hold to close", SCREEN_W - 6, SCREEN_H - 6, 2);

  drawCenter(tft, false);
  for (int i = 0; i < slotCount(); i++) {
    drawSlot(tft, i);
    if (animate) delay(35);  // buttons pop in one by one
  }
}

void AppWheel::drawSlot(TFT_eSPI& tft, int i) {
  int x, y;
  slotPos(i, x, y);

  bool highlight = false;
  if (inGames) {
    if (i < gameCount) highlight = (gameItems[i] == current);
  } else {
    if (i < mainCount) highlight = (mainItems[i] == current);
    else highlight = current && current->isGame();  // you're "inside" the Games folder
  }

  tft.fillCircle(x, y, ITEM_R, COL_PANEL);
  if (highlight) {
    tft.drawCircle(x, y, ITEM_R + 3, COL_EYE);
    tft.drawCircle(x, y, ITEM_R + 4, COL_EYE);
  }

  if (inGames) {
    if (i < gameCount) {
      gameItems[i]->drawIcon(tft, x, y);
    } else {
      // Back arrow
      tft.drawWideLine(x + 6, y - 10, x - 6, y, 5, COL_FG, COL_PANEL);
      tft.drawWideLine(x - 6, y, x + 6, y + 10, 5, COL_FG, COL_PANEL);
    }
  } else {
    if (i < mainCount) {
      mainItems[i]->drawIcon(tft, x, y);
    } else {
      // Games folder: a little gamepad
      tft.fillRoundRect(x - 17, y - 10, 34, 20, 9, 0xC618);
      tft.fillRect(x - 12, y - 2, 10, 4, COL_VISOR);   // d-pad
      tft.fillRect(x - 9, y - 5, 4, 10, COL_VISOR);
      tft.fillCircle(x + 6, y + 2, 3, COL_PINK);        // buttons
      tft.fillCircle(x + 11, y - 3, 3, COL_EYE);
    }
  }
}

void AppWheel::drawCenter(TFT_eSPI& tft, bool blink) {
  // Glowing ring + visor
  tft.fillCircle(CX, CY, CENTER_R, COL_GLOW);
  tft.fillCircle(CX, CY, CENTER_R - 3, COL_VISOR);
  // Mini robot face
  if (blink) {
    tft.fillRoundRect(CX - 17, CY - 7, 10, 3, 1, COL_EYE);
    tft.fillRoundRect(CX + 7, CY - 7, 10, 3, 1, COL_EYE);
  } else {
    tft.fillEllipse(CX - 12, CY - 6, 5, 7, COL_EYE);
    tft.fillEllipse(CX + 12, CY - 6, 5, 7, COL_EYE);
  }
  dotArc(tft, CX, CY + 3, 9, 30, 150, 2, COL_EYE);
}

WheelChoice AppWheel::tap(TFT_eSPI& tft, int x, int y) {
  WheelChoice c;
  int dx = x - CX, dy = y - CY;
  if (dx * dx + dy * dy <= (CENTER_R + 4) * (CENTER_R + 4)) {
    c.type = WheelChoice::HOME;
    return c;
  }

  for (int i = 0; i < slotCount(); i++) {
    int sx, sy;
    slotPos(i, sx, sy);
    int ddx = x - sx, ddy = y - sy;
    if (ddx * ddx + ddy * ddy > (ITEM_R + 8) * (ITEM_R + 8)) continue;

    if (inGames) {
      if (i < gameCount) { c.type = WheelChoice::OPEN; c.app = gameItems[i]; return c; }
      inGames = false;              // Back
    } else {
      if (i < mainCount) { c.type = WheelChoice::OPEN; c.app = mainItems[i]; return c; }
      inGames = true;               // Games folder
    }
    drawRing(tft, true);
    return c;                       // NONE: stay open, ring changed
  }

  c.type = WheelChoice::CLOSE;      // tapped empty space
  return c;
}

void AppWheel::update(TFT_eSPI& tft, uint32_t now) {
  // The little face in the middle blinks every few seconds
  if (!blinking && now > nextBlink) {
    blinking = true;
    blinkUntil = now + 150;
    drawCenter(tft, true);
  } else if (blinking && now > blinkUntil) {
    blinking = false;
    nextBlink = now + random(2000, 5000);
    drawCenter(tft, false);
  }
}
