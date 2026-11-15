#pragma once

// High scores saved in the ESP32's flash (NVS), so they survive reboots.
namespace Scores {
  int best(const char* game);
  // Saves the score if it beats the best. Returns true on a new record.
  bool submit(const char* game, int score);
}
