#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "pins.h"

struct TouchEvent {
  enum Type {
    NONE,
    TOUCH_DOWN,   // finger just touched (instant — good for games like Flappy Bot)
    TAP,          // finger lifted without moving much
    SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN,
    LONG_PRESS    // held ~0.7 s in one spot (opens the app wheel)
  };
  Type type = NONE;
  int16_t x = 0, y = 0;   // where the touch started (screen coords)
  int16_t dx = 0, dy = 0; // how far the finger moved (on lift events)
  uint16_t ms = 0;        // how long the finger was down (on lift events)
};

// Turns raw resistive-touch readings into taps, swipes and long presses.
class Input {
public:
  void begin();
  TouchEvent poll(uint32_t now);

private:
  SPIClass spi{VSPI};
  XPT2046_Touchscreen ts{TOUCH_CS, TOUCH_IRQ};

  bool down = false;
  bool longFired = false;
  int16_t startX = 0, startY = 0, lastX = 0, lastY = 0;
  uint32_t downAt = 0;
  uint8_t pressCount = 0, releaseCount = 0;

  static const int SWIPE_MIN_PX = 50;
  static const uint32_t LONG_PRESS_MS = 700;
};
