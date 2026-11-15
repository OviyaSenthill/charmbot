#include "apps/ClockApp.h"
#include "core/Net.h"
#include <time.h>

void ClockApp::update(TFT_eSPI& tft, uint32_t now) {
  struct tm t;
  if (!getLocalTime(&t, 0)) {
    int state = Net::connected() ? 1 : 0;
    if (state != lastState) {
      lastState = state;
      tft.fillScreen(COL_BG);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(COL_DIM, COL_BG);
      tft.drawString(state ? "Syncing time..." : "Connecting to WiFi...", SCREEN_W / 2, SCREEN_H / 2, 4);
    }
    return;
  }

  if (lastState != 2) {
    lastState = 2;
    lastSec = -1;
    tft.fillScreen(COL_BG);
  }
  if (t.tm_sec == lastSec) return;
  lastSec = t.tm_sec;

  char timeBuf[8], secBuf[4], ampm[3], dateBuf[32];
#if CLOCK_24H
  strftime(timeBuf, sizeof(timeBuf), "%H:%M", &t);
  ampm[0] = 0;
#else
  int h = t.tm_hour % 12;
  if (h == 0) h = 12;
  snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", h, t.tm_min);
  strcpy(ampm, t.tm_hour < 12 ? "AM" : "PM");
#endif
  snprintf(secBuf, sizeof(secBuf), "%02d", t.tm_sec);
  strftime(dateBuf, sizeof(dateBuf), "%A, %b %e", &t);

  // Time (font 8 = big 7-segment style digits)
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_FG, COL_BG);
  tft.setTextPadding(290);
  tft.drawString(timeBuf, SCREEN_W / 2 - 14, 100, 8);

  // Seconds + AM/PM in the corner
  tft.setTextPadding(40);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(secBuf, 286, 70, 4);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString(ampm, 288, 100, 2);

  // Date
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(300);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(dateBuf, SCREEN_W / 2, 185, 4);
  tft.setTextPadding(0);
}
