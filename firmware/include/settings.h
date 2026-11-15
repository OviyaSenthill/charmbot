#pragma once
// Loads your private config.h if it exists, otherwise the example defaults.
#if __has_include("config.h")
  #include "config.h"
#else
  #warning "No include/config.h found - using config.example.h defaults. Copy it to config.h!"
  #include "config.example.h"
#endif

#define SCREEN_W 320
#define SCREEN_H 240

// ---- Mood engine options (override any of these in config.h) ----
#ifndef NIGHT_START_HOUR
  #define NIGHT_START_HOUR 0      // sleepy from midnight...
#endif
#ifndef NIGHT_END_HOUR
  #define NIGHT_END_HOUR   6      // ...until 6 am
#endif
#ifndef SLEEP_AFTER_MIN
  #define SLEEP_AFTER_MIN  10     // fall asleep after this many idle minutes
#endif
// Light sensor (LDR on IO34). Off until calibrated: see docs/mood.md
#ifndef LIGHT_DEBUG
  #define LIGHT_DEBUG 0           // 1 = print light readings to the serial monitor
#endif
#ifndef LIGHT_DARK_THRESHOLD
  #define LIGHT_DARK_THRESHOLD -1 // -1 = don't use the light sensor
#endif
#ifndef LIGHT_DARK_WHEN_ABOVE
  #define LIGHT_DARK_WHEN_ABOVE 1 // 1 = higher reading means darker
#endif
// Tilt ball switch for "shake me" detection: one leg to this pin, the other to GND. -1 = none
#ifndef TILT_PIN
  #define TILT_PIN -1
#endif
