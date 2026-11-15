#pragma once
#include <stdint.h>

// ============================================================
//  FACE DESIGN KNOBS — tweak these to match your Canva mockup.
//  Screen is 320 x 240. See docs/face-design.md for the spec.
// ============================================================
namespace FaceStyle {
  // Colors (RGB565). Hex equivalents for Canva in the comments.
  constexpr uint16_t VISOR      = 0x08A6;  // #0B1530 dark navy visor
  constexpr uint16_t EYE        = 0x3F3E;  // #3FE6F5 glowing cyan
  constexpr uint16_t EYE_GLOW   = 0x1B50;  // #1A6A80 cyan halo
  constexpr uint16_t ANGRY      = 0xF9CB;  // #FF3B5C
  constexpr uint16_t ANGRY_GLOW = 0x78C5;  // #7A1A2A
  constexpr uint16_t LOVE       = 0xFA6D;  // #FF4D6D
  constexpr uint16_t LOVE_GLOW  = 0x7926;  // #7A2535

  // The face is drawn into an off-screen sprite (no flicker) placed here:
  constexpr int SPR_X = 40, SPR_Y = 45, SPR_W = 240, SPR_H = 150;

  // Geometry, in sprite coordinates (add SPR_X / SPR_Y for screen coords)
  constexpr int EYE_W   = 36;   // oval eye width
  constexpr int EYE_H   = 48;   // oval eye height
  constexpr int EYE_GAP = 100;  // center-to-center distance between eyes
  constexpr int EYE_Y   = 55;   // eye center height
  constexpr int MOUTH_Y = 112;  // mouth center height
  constexpr int LINE_W  = 8;    // stroke thickness for arcs / lines / brows
  constexpr int GLOW_PX = 5;    // how far the halo extends past the eye

  // Motion
  constexpr int LOOK_X = 14;    // max eye wander left/right (px)
  constexpr int LOOK_Y = 8;     // max eye wander up/down (px)
  constexpr int BLINK_MS = 160;
}
