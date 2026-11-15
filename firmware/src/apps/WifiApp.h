#pragma once
#include "core/App.h"

// WiFi picker: scan, tap a network, type the password on an on-screen keyboard.
// A network that connects is saved and used automatically on every boot.
class WifiApp : public App {
public:
  const char* name() const override { return "WiFi"; }
  void enter(TFT_eSPI& tft) override;
  void exit() override;
  void resume(TFT_eSPI& tft) override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;
  bool capturesSwipes() const override { return state == KEYBOARD; }  // no accidental app switching while typing
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  enum State { SCANNING, LIST, KEYBOARD, CONNECTING, RESULT };
  State state = SCANNING;

  struct AP { String ssid; int rssi; bool locked; };
  static const int MAX_APS = 16;
  static const int ROWS_VISIBLE = 6;
  AP aps[MAX_APS];
  int apCount = 0;
  int scroll = 0;
  int scanRetries = 0;

  // Keyboard
  struct Key { int16_t x, y, w, h; char c; uint8_t kind; };
  static const int MAX_KEYS = 50;
  Key keys[MAX_KEYS];
  int keyCount = 0;
  int layer = 0;            // 0 lower, 1 upper, 2 symbols
  bool shiftOnce = false;
  int flashKey = -1;
  uint32_t flashUntil = 0;

  String ssid, pass;
  bool resultOk = false;
  String resultMsg;
  uint32_t stateAt = 0, lastAnim = 0;
  int anim = 0;

  // Touches are queued in onTouch() and handled in update(), where we can draw
  bool hasTap = false;
  int tapX = 0, tapY = 0;
  int pendingScroll = 0;

  TFT_eSPI* g = nullptr;

  void startScan();
  void collectScan(int n);
  void choose(int idx);
  void connect();

  void redrawAll();
  void drawHeader();
  void drawList();
  void drawScanning();
  void drawConnecting();
  void drawResult();

  void buildKeys();
  void drawField();
  void drawKeyboard();
  void drawKey(int i, bool pressed);
  void pressKey(int i);

  void handleTap(int x, int y);
};
