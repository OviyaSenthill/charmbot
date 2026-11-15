#pragma once
#include "core/App.h"

// Holds the list of apps and handles navigation:
//   swipe left / right -> next / previous app
//   long press         -> back to the home app (index 0)
class AppManager {
public:
  void begin(TFT_eSPI* tft) { this->tft = tft; }
  void add(App* app);
  void switchTo(int index);
  void handle(const TouchEvent& e);
  void update(uint32_t now);
  App* active() { return current >= 0 ? apps[current] : nullptr; }

private:
  static const int MAX_APPS = 10;
  TFT_eSPI* tft = nullptr;
  App* apps[MAX_APPS] = {};
  int count = 0;
  int current = -1;
};
