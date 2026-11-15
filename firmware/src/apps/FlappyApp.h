#pragma once
#include "core/App.h"

// Flappy Bot: tap anywhere to fire the jetpack. Fly through the gaps.
// Drawn into an off-screen 8-bit sprite for smooth, flicker-free motion.
class FlappyApp : public App {
public:
  const char* name() const override { return "Flappy Bot"; }
  bool isGame() const override { return true; }
  bool capturesSwipes() const override { return true; }
  void enter(TFT_eSPI& tft) override;
  void exit() override;
  void resume(TFT_eSPI& tft) override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  enum State { READY, PLAYING, PAUSED, DEAD };
  State state = READY;

  TFT_eSprite* spr = nullptr;

  struct Pipe { float x; int gapY; bool scored; };
  struct Star { float x; uint8_t y; };
  static const int NUM_PIPES = 3;
  static const int NUM_STARS = 24;
  Pipe pipes[NUM_PIPES];
  Star stars[NUM_STARS];

  float y = 110, vy = 0;
  float groundOff = 0;
  int flame = 0;
  int score = 0, best = 0;
  bool newBest = false;
  bool flapRequested = false;
  bool initialized = false;
  uint32_t lastTick = 0, deadAt = 0, frame = 0;

  void reset();
  void tick();
  void die();
  void render(TFT_eSPI& g);
  void drawBot(TFT_eSPI& g, int x, int y, bool dead, bool thrust);
  void present(TFT_eSPI& tft);
};
