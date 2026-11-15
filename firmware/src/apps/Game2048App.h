#pragma once
#include "core/App.h"

// 2048: swipe to slide the tiles; matching tiles merge.
// Your game is kept when you leave, so you can come back to it later.
class Game2048App : public App {
public:
  const char* name() const override { return "2048"; }
  bool isGame() const override { return true; }
  bool capturesSwipes() const override { return true; }
  void enter(TFT_eSPI& tft) override;
  void resume(TFT_eSPI& tft) override { enter(tft); }
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  uint16_t grid[4][4] = {};
  int score = 0, best = 0;
  bool started = false, over = false, won = false, showWin = false;
  int pendingMove = -1;      // 0 up, 1 right, 2 down, 3 left
  bool needsNewGame = false;
  bool needsRedraw = false;
  int lastSpawnR = -1, lastSpawnC = -1;

  void newGame();
  bool move(int dir);
  void spawn();
  bool canMove() const;

  void drawAll(TFT_eSPI& tft);
  void drawTiles(TFT_eSPI& tft);
  void drawTile(TFT_eSPI& tft, int r, int c);
  void drawPanel(TFT_eSPI& tft);
  void drawBanner(TFT_eSPI& tft, const char* big, const char* small);
};
