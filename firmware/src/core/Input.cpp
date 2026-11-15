#include "core/Input.h"
#include "settings.h"

#ifndef TOUCH_MIN_Z
  #define TOUCH_MIN_Z 500   // minimum pressure to count as a touch (raise if you get phantom taps)
#endif
static const uint8_t PRESS_SAMPLES = 3;    // readings in a row needed to start a touch
static const uint8_t RELEASE_SAMPLES = 4;  // readings in a row needed to end one

void Input::begin() {
  spi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(spi);
  ts.setRotation(1);  // landscape, matches tft.setRotation(1)
}

TouchEvent Input::poll(uint32_t now) {
  TouchEvent ev;

  // ---- read + filter: resistive panels produce noisy single readings ----
  bool raw = false;
  int16_t x = lastX, y = lastY;
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
#if TOUCH_DEBUG
    Serial.printf("raw x=%d y=%d z=%d\n", p.x, p.y, p.z);
#endif
    if (p.z >= TOUCH_MIN_Z) {
      raw = true;
      x = constrain(map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W), 0, SCREEN_W - 1);
      y = constrain(map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H), 0, SCREEN_H - 1);
    }
  }
  if (raw) { if (pressCount < 255) pressCount++; releaseCount = 0; }
  else     { if (releaseCount < 255) releaseCount++; pressCount = 0; }

  // ---- finger down ----
  if (!down) {
    if (pressCount >= PRESS_SAMPLES) {
      down = true;
      longFired = false;
      startX = lastX = x;
      startY = lastY = y;
      downAt = now;
      ev.type = TouchEvent::TOUCH_DOWN;
      ev.x = x;
      ev.y = y;
    }
    return ev;
  }

  // ---- finger held ----
  if (releaseCount < RELEASE_SAMPLES) {
    if (raw) { lastX = x; lastY = y; }
    // Long press: held in roughly one spot
    if (!longFired && now - downAt > LONG_PRESS_MS &&
        abs(lastX - startX) < SWIPE_MIN_PX / 2 && abs(lastY - startY) < SWIPE_MIN_PX / 2) {
      longFired = true;
      ev.type = TouchEvent::LONG_PRESS;
      ev.x = startX;
      ev.y = startY;
    }
    return ev;
  }

  // ---- finger lifted ----
  down = false;
  if (longFired) return ev;

  int dx = lastX - startX;
  int dy = lastY - startY;
  ev.x = startX;
  ev.y = startY;
  ev.dx = dx;
  ev.dy = dy;
  uint32_t held = now - downAt;
  ev.ms = held > 65535 ? 65535 : (uint16_t)held;
  if (abs(dx) > SWIPE_MIN_PX && abs(dx) > abs(dy)) {
    ev.type = dx < 0 ? TouchEvent::SWIPE_LEFT : TouchEvent::SWIPE_RIGHT;
  } else if (abs(dy) > SWIPE_MIN_PX) {
    ev.type = dy < 0 ? TouchEvent::SWIPE_UP : TouchEvent::SWIPE_DOWN;
  } else {
    ev.type = TouchEvent::TAP;
  }
  return ev;
}
