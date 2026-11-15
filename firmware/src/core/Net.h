#pragma once
#include <Arduino.h>

// WiFi + time sync. Non-blocking: call Net::loop() every loop.
namespace Net {
  void begin();
  void loop(uint32_t now);
  bool connected();
}
