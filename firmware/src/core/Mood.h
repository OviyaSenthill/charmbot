#pragma once
#include <Arduino.h>

// The robot's feelings. Anything can report an event; the face reads the mood
// and picks an expression. Happiness drifts back toward neutral over time.
namespace Mood {
  enum Event { TAP, SPAM, PET, LOVE, HIGH_SCORE, GAME_LOST, SHAKE };

  void begin();
  void update(uint32_t now);
  void interaction(uint32_t now);  // any touch anywhere resets the idle timer
  void event(Event e);

  int happiness();                 // 0..100
  uint32_t idleMs(uint32_t now);

  // The latest event, for the face to react to (seq goes up by one per event)
  uint32_t lastEventSeq();
  Event lastEvent();
  uint32_t lastEventAt();
}
