#include "core/Input.h"
#include "settings.h"

void Input::begin() {
  spi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(spi);
  ts.setRotation(1);  // landscape, matches tft.setRotation(1)
}

TouchEvent Input::poll(uint32_t now) {
  TouchEvent ev;

  if (ts.touched()) {
    TS_Point p = ts.getPoint();
#if TOUCH_DEBUG
    Serial.printf("raw x=%d y=%d z=%d\n", p.x, p.y, p.z);
#endif
    int16_t x = constrain(map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W), 0, SCREEN_W - 1);
    int16_t y = constrain(map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H), 0, SCREEN_H - 1);

    if (!down) {
      down = true;
      longFired = false;
      startX = lastX = x;
      startY = lastY = y;
      downAt = now;
    } else {
      lastX = x;
      lastY = y;
    }

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

  // Finger lifted
  if (down) {
    down = false;
    if (longFired) return ev;

    int dx = lastX - startX;
    int dy = lastY - startY;
    ev.x = startX;
    ev.y = startY;

    if (abs(dx) > SWIPE_MIN_PX && abs(dx) > abs(dy)) {
      ev.type = dx < 0 ? TouchEvent::SWIPE_LEFT : TouchEvent::SWIPE_RIGHT;
    } else if (abs(dy) > SWIPE_MIN_PX) {
      ev.type = dy < 0 ? TouchEvent::SWIPE_UP : TouchEvent::SWIPE_DOWN;
    } else {
      ev.type = TouchEvent::TAP;
    }
  }
  return ev;
}
