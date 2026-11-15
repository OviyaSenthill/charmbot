#pragma once
#include "core/App.h"
#include "core/AppWheel.h"

// Holds the list of apps and handles navigation:
//   swipe left / right -> next / previous app (games are skipped)
//   long press         -> open the app wheel (works everywhere, even in games)
class AppManager {
public:
  void begin(TFT_eSPI* tft) { this->tft = tft; }
  void add(App* app);
  void switchTo(int index);
  void switchTo(App* app);
  void handle(const TouchEvent& e);
  void update(uint32_t now);
  App* active() { return current >= 0 ? apps[current] : nullptr; }
  AppWheel& wheel() { return wheel_; }

private:
  void step(int dir);
  int indexOf(App* app) const;
  void closeWheelAndOpen(App* app);

  static const int MAX_APPS = 12;
  TFT_eSPI* tft = nullptr;
  App* apps[MAX_APPS] = {};
  int count = 0;
  int current = -1;
  AppWheel wheel_;
};
