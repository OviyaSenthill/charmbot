#pragma once
#include <Arduino.h>

// Fetches the weather in the background (on its own FreeRTOS task) as soon as WiFi
// connects and every 15 minutes after, so the Weather screen and the face's mood
// always have fresh data instantly, with no waiting and no frozen screen.
struct WeatherData {
  bool valid = false;
  float temp = 0, feels = 0, wind = 0, hi = 0, lo = 0;
  int humidity = 0, code = -1;
  uint32_t fetchedAt = 0;   // millis() of the last successful update
};

namespace WeatherService {
  void begin();
  bool get(WeatherData& out);  // copy of the latest data; false if none yet
  uint32_t version();          // goes up every time new data (or a new error) arrives
  bool busy();                 // a download is in progress
  String lastError();          // "" if the last attempt worked
  void refreshNow();           // ask for an update (rate-limited to every 15 s)
}
