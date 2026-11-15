#pragma once
#include "core/App.h"

// Snake. Swipe to turn, or tap beside the snake's head to turn toward the tap.
// Long-press opens the app wheel; the game pauses and waits for you.
class SnakeApp : public App {
public:
  const char* name() const override { return "Snake"; }
  bool isGame() const override { return true; }
  bool capturesSwipes() const override { return true; }
  void enter(TFT_eSPI& tft) override;
  void resume(TFT_eSPI& tft) override { enter(tft); }
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  static const int CELL = 16, COLS = 20, ROWS = 14, TOP = 16;
  static const int MAX_LEN = COLS * ROWS;
  struct P { int8_t x, y; };

  P body[MAX_LEN];
  int len = 0;
  int dir = 1, nextDir = 1;  // 0 up, 1 right, 2 down, 3 left
  P food{0, 0};
  bool started = false;
  bool over = false;
  bool paused = false;
  bool needsReset = false;
  bool needsRedraw = false;
  int score = 0, best = 0;
  uint32_t lastStep = 0;
  TFT_eSPI* g = nullptr;

  void reset();
  void step();
  void placeFood();
  void drawBoard();
  void drawFood();
  void cell(P p, uint16_t c);
  void drawScore();
  void banner(const char* big, const char* small);
  void turn(int d);
};
