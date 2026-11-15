#pragma once
#include <Arduino.h>

// Screen brightness (PWM on the backlight pin) with smooth fades.
namespace Backlight {
  void begin();
  void fadeTo(uint8_t level);  // 0 = off, 255 = full
  void update(uint32_t now);
}
