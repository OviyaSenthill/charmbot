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
