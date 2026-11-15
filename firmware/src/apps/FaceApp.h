#pragma once
#include "core/App.h"

// The robot face. Expressions are built from a few simple parts
// (eye shape, brows, mouth, color) so they can be animated:
// blinking, glancing around, bobbing to music.
// Which expression shows is decided by the mood engine (see docs/mood.md).
enum class Expression : uint8_t {
  Neutral, Happy, Wink, Worried, Angry,
  Love, Surprised, Bored, Content, Sad,
  Sleepy, Asleep, Yawn, Dizzy, Sing,
  COUNT
};

class FaceApp : public App {
public:
  const char* name() const override { return "Face"; }
  void enter(TFT_eSPI& tft) override;
  void exit() override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;

private:
  enum class Bubble : uint8_t { None, Note, Zzz, Rain, Heart, WifiOff, Angry, Sun };

  TFT_eSprite* spr = nullptr;
  Expression expr = Expression::Happy;
  Bubble bubble = Bubble::None, drawnBubble = Bubble::None;

  // Animation
  uint32_t lastFrame = 0, nextBlink = 0, blinkStart = 0, nextGlance = 0;
  bool blinking = false;
  float lookX = 0, lookY = 0, targetX = 0, targetY = 0;

  // Reactions: show an expression (and bubble) for a while, optionally followed by another
  Expression reactExpr = Expression::Happy, nextReactExpr = Expression::Happy;
  Bubble reactBubble = Bubble::None;
  uint32_t reactUntil = 0, nextReactMs = 0;

  // Touch input, queued by onTouch() and handled in update()
  bool gotTap = false, gotPet = false, gotDown = false;
  int16_t tapX = 0, tapY = 0;
  uint32_t lastPetAt = 0;
  uint32_t tapTimes[6] = {};
  int tapIdx = 0;
  uint32_t petTimes[3] = {};
  int petIdx = 0;

  // Mood inputs
  bool asleep = false, sleptInDark = false;
  bool wasConnected = false;
  uint32_t seenMoodSeq = 0, seenMusicStart = 0;
  uint32_t lastLightRead = 0, darkSince = 0;
  bool isDark = false;
  uint32_t nextYawn = 0, yawnUntil = 0;
  int tiltLast = -1, tiltToggles = 0;
  uint32_t tiltWindow = 0;

  void react(Expression e, uint32_t ms, Bubble b = Bubble::None);
  void fallAsleep(bool dark);
  void wake(uint32_t now);
  void handleInput(uint32_t now);
  void checkSensors(uint32_t now);
  Expression chooseExpression(uint32_t now, Bubble& b);
  void drawBubble(TFT_eSPI& tft, Bubble b);
  // Dancing (while music plays)
  struct Note { float x, y; uint8_t age; bool alive; };
  static const int MAX_NOTES = 5;
  Note notes[MAX_NOTES] = {};
  uint32_t lastBeat = 0;
  float tilt = 0, squash = 1.0f;
  bool dancing = false;
  void updateDance(uint32_t now, int& ox, int& oy);

  // Floating "z z Z" while asleep / sleepy / bored
  struct Zee { float x, y; uint8_t age; bool alive; };
  static const int MAX_ZEES = 4;
  Zee zees[MAX_ZEES] = {};
  uint32_t nextZee = 0;
  bool showZees = false;
  void updateZees(uint32_t now);

  // Hearts that pop up when you pet it
  struct Heart { float x, y, vx; uint8_t age, size; bool alive; };
  static const int MAX_HEARTS = 8;
  Heart hearts[MAX_HEARTS] = {};
  void spawnHearts(int count);
  void updateHearts();

  // Idle fun: random emotes and accessories every now and then
  enum class Accessory : uint8_t { None, Bow, Halo, Sunglasses, Crown, Flower };
  Accessory accessory = Accessory::None;
  uint32_t accessoryUntil = 0, nextIdleFun = 0;
  void idleFun(uint32_t now);
  void drawAccessory(TFT_eSPI& g, int lx, int rx, int ey, int mx);

  void render(TFT_eSPI& g, int ox, int oy, float eyeOpen, uint32_t now);
};
