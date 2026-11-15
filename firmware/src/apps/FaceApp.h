#pragma once
#include "core/App.h"

// The robot face. Expressions are built from a few simple parts
// (eye shape, brows, mouth, color) so they can be animated:
// blinking, glancing around, and a gentle bob.
enum class Expression : uint8_t {
  Neutral, Happy, Wink, Worried, Angry,
  Love, Surprised, Bored, Content, Sad,
  COUNT
};

class FaceApp : public App {
public:
  const char* name() const override { return "Face"; }
  void enter(TFT_eSPI& tft) override;
  void exit() override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;

  void setExpression(Expression e) { expr = e; }
  Expression expression() const { return expr; }

private:
  TFT_eSprite* spr = nullptr;
  Expression expr = Expression::Happy;

  uint32_t lastFrame = 0, nextBlink = 0, blinkStart = 0, nextGlance = 0;
  bool blinking = false;
  float lookX = 0, lookY = 0, targetX = 0, targetY = 0;

  void render(TFT_eSPI& g, int ox, int oy, float eyeOpen);
};
