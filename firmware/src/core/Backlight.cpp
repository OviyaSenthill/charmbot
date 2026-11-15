#include "core/Backlight.h"
#include "pins.h"

// At full brightness the pin is held steadily HIGH (no PWM at all), because PWM switching
// noise can leak into the resistive touch readings and cause phantom taps.
// PWM is only used while dimmed (e.g. when the robot is asleep).
namespace {
  const int CHANNEL = 15;     // a PWM channel nothing else uses
  const int FREQ = 20000;     // 20 kHz: above hearing, less touch interference
  int current = 255, target = 255;
  bool pwmAttached = false;
  uint32_t lastStep = 0;

  void apply(int level) {
    if (level >= 255) {
      if (pwmAttached) { ledcDetachPin(BACKLIGHT_PIN); pwmAttached = false; }
      pinMode(BACKLIGHT_PIN, OUTPUT);
      digitalWrite(BACKLIGHT_PIN, HIGH);
    } else {
      if (!pwmAttached) { ledcAttachPin(BACKLIGHT_PIN, CHANNEL); pwmAttached = true; }
      ledcWrite(CHANNEL, level);
    }
  }
}

namespace Backlight {

void begin() {
  ledcSetup(CHANNEL, FREQ, 8);
  apply(255);
}

void fadeTo(uint8_t level) { target = level; }

void update(uint32_t now) {
  if (current == target || now - lastStep < 12) return;
  lastStep = now;
  int step = target > current ? 12 : -6;  // brighten fast, dim gently
  current = (step > 0) ? min(current + step, target) : max(current + step, target);
  apply(current);
}

}  // namespace Backlight
