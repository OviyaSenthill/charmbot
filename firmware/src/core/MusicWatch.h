#pragma once
#include <Arduino.h>

// Quietly checks Spotify in the background (every ~12 s) while the face is showing,
// so the robot can react when music starts. Runs on its own FreeRTOS task so the
// face animation never freezes during the network request.
namespace MusicWatch {
  void begin();
  void setEnabled(bool on);
  bool playing();
  uint32_t startedAt();  // millis() when a song last started; 0 = never
}
