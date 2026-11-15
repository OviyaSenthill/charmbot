#pragma once
#include "core/App.h"

// Current weather from Open-Meteo (free, no API key).
// Data is kept between visits; refreshes every 15 minutes.
class WeatherApp : public App {
public:
  const char* name() const override { return "Weather"; }
  void enter(TFT_eSPI& tft) override { dirty = true; }
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;

private:
  bool fetch();
  void draw(TFT_eSPI& tft);

  bool hasData = false;
  bool dirty = true;
  bool forceRefresh = false;
  uint32_t lastFetch = 0;
  uint32_t lastAttempt = 0;

  float temp = 0, feels = 0, wind = 0, hi = 0, lo = 0;
  int humidity = 0, code = 0;
};
