#pragma once
#include <Arduino.h>

// WiFi + time sync. Non-blocking: call Net::loop() every loop.
// The network you pick on the WiFi screen is saved in flash and used on boot;
// config.h's WIFI_SSID / WIFI_PASSWORD are only the fallback.
namespace Net {
  void begin();
  void loop(uint32_t now);
  bool connected();

  void setPaused(bool paused);                              // stop background reconnects (WiFi screen)
  void connectTo(const String& ssid, const String& pass);   // try a network (not saved yet)
  void save();                                              // remember the current network
  void restoreSaved();                                      // go back to the saved/default network

  String currentSsid();
  String savedSsid();
  String savedPassword();
}
