#pragma once
#include <TFT_eSPI.h>

// Shared UI colors (RGB565). Change these to restyle every app at once.
#define COL_BG      TFT_BLACK
#define COL_FG      TFT_WHITE
#define COL_DIM     0x7BEF   // mid gray
#define COL_ACCENT  0x07FF   // cyan
#define COL_WARN    0xFD20   // orange
#define COL_GOOD    0x07E0   // green
#define COL_SPOTIFY 0x1EA9   // music green

// Robot palette (matches the face)
#define COL_VISOR   0x08A6   // #0B1530 dark navy
#define COL_PANEL   0x1949   // #1C2A4A raised navy (wheel buttons, game panels)
#define COL_EYE     0x3F3E   // #3FE6F5 glowing cyan
#define COL_GLOW    0x1B50   // #1A6A80 cyan halo
#define COL_PINK    0xFA6D   // #FF4D6D
