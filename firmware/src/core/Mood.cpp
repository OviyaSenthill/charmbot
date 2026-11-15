#include "core/Mood.h"

namespace {
  float happy = 60;
  uint32_t lastTouch = 0;
  uint32_t lastDecay = 0;
  uint32_t seq = 0;
  Mood::Event lastEv = Mood::TAP;
  uint32_t lastEvAt = 0;

  void bump(float d) { happy = constrain(happy + d, 0.0f, 100.0f); }
}

namespace Mood {

void begin() {
  lastTouch = millis();
  lastDecay = millis();
}

void update(uint32_t now) {
  if (now - lastDecay < 60000) return;  // once a minute
  lastDecay = now;
  if (happy > 50) bump(-1);              // drift toward neutral
  else if (happy < 50) bump(+1);
  if (now - lastTouch > 5UL * 60 * 1000) bump(-2);  // lonely
}

void interaction(uint32_t now) { lastTouch = now; }

void event(Event e) {
  switch (e) {
    case TAP:        bump(+3);  break;
    case SPAM:       bump(-8);  break;
    case PET:        bump(+8);  break;
    case LOVE:       bump(+20); break;
    case HIGH_SCORE: bump(+20); break;
    case GAME_LOST:  bump(-4);  break;
    case SHAKE:      bump(-3);  break;
  }
  lastEv = e;
  lastEvAt = millis();
  seq++;
}

int happiness() { return (int)happy; }
uint32_t idleMs(uint32_t now) { return now - lastTouch; }
uint32_t lastEventSeq() { return seq; }
Event lastEvent() { return lastEv; }
uint32_t lastEventAt() { return lastEvAt; }

}  // namespace Mood
