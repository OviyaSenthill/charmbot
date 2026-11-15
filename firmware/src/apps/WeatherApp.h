#pragma once
#include "core/App.h"

// Current weather from Open-Meteo (free, no API key).
// Data is fetched in the background by core/WeatherService; this screen just draws it.
class WeatherApp : public App {
public:
  const char* name() const override { return "Weather"; }
  void enter(TFT_eSPI& tft) override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  void draw(TFT_eSPI& tft);
  uint32_t shownVersion = 0xFFFFFFFF;
  bool shownConnected = false;
  bool shownBusy = false;
  uint32_t shownMinute = 0;
};
