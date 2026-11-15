#pragma once
#include "core/App.h"

// What the user picked on the wheel.
struct WheelChoice {
  enum Type { NONE, CLOSE, HOME, OPEN };
  Type type = NONE;
  App* app = nullptr;
};

// The app wheel: long-press anywhere to open it.
// The robot face sits in the center (tap = home), apps orbit around it,
// and a Games folder opens a second ring with the games.
class AppWheel {
public:
  void setApps(App** items, int n);
  void setGames(App** items, int n);

  bool isOpen() const { return open_; }
  void open(TFT_eSPI& tft, App* current);
  void close() { open_ = false; }

  WheelChoice tap(TFT_eSPI& tft, int x, int y);
  void update(TFT_eSPI& tft, uint32_t now);

private:
  static const int MAX_ITEMS = 8;
  App* mainItems[MAX_ITEMS] = {};
  App* gameItems[MAX_ITEMS] = {};
  int mainCount = 0, gameCount = 0;

  bool open_ = false;
  bool inGames = false;
  App* current = nullptr;

  uint32_t nextBlink = 0, blinkUntil = 0;
  bool blinking = false;

  int slotCount() const;
  void slotPos(int i, int& x, int& y) const;
  void drawRing(TFT_eSPI& tft, bool animate);
  void drawSlot(TFT_eSPI& tft, int i);
  void drawCenter(TFT_eSPI& tft, bool blink);
};
