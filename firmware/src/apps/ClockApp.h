#pragma once
#include "core/App.h"

// Big clock + date. Time comes from NTP once WiFi connects,
// then the ESP32 keeps it on its own.
class ClockApp : public App {
public:
  const char* name() const override { return "Clock"; }
  void enter(TFT_eSPI& tft) override { lastSec = -1; lastState = -1; }
  void update(TFT_eSPI& tft, uint32_t now) override;
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  int lastSec = -1;
  int lastState = -1;  // 0 = waiting for wifi, 1 = waiting for time, 2 = showing time
};
