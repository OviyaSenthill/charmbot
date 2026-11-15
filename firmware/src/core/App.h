#pragma once
#include <TFT_eSPI.h>
#include "core/Input.h"
#include "settings.h"
#include "theme.h"

// Every screen on Charmbot is an App.
// Only one app is active at a time; it should allocate big buffers in enter()
// and free them in exit(), because the ESP32 only has ~320 KB of usable RAM.
class App {
public:
  virtual ~App() {}
  virtual const char* name() const = 0;

  // Called when the app becomes visible. The screen has already been cleared.
  virtual void enter(TFT_eSPI& tft) {}

  // Called when leaving the app. Free sprites/buffers here.
  virtual void exit() {}

  // Called every loop while active. Redraw only what changed.
  virtual void update(TFT_eSPI& tft, uint32_t now) = 0;

  // Touch events not consumed by navigation.
  virtual void onTouch(const TouchEvent& e) {}

  // Return true if the app wants left/right swipes for itself (e.g. games).
  // Long-press always returns to the home app.
  virtual bool capturesSwipes() const { return false; }
};
