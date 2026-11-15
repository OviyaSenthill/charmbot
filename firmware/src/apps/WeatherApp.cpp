#include "apps/WeatherApp.h"
#include "core/Net.h"
#include "core/WeatherService.h"

// The downloading happens in the background (core/WeatherService), so this screen
// only draws, and shows up instantly.

// WMO weather codes -> short description
static const char* describe(int c) {
  if (c == 0) return "Clear";
  if (c <= 2) return "Partly cloudy";
  if (c == 3) return "Overcast";
  if (c == 45 || c == 48) return "Fog";
  if (c >= 51 && c <= 57) return "Drizzle";
  if (c >= 61 && c <= 67) return "Rain";
  if (c >= 71 && c <= 77) return "Snow";
  if (c >= 80 && c <= 82) return "Showers";
  if (c >= 85 && c <= 86) return "Snow showers";
  if (c >= 95) return "Thunderstorm";
  return "Unknown";
}

// ---------- tiny vector icons ----------
static void cloud(TFT_eSPI& g, int x, int y, uint16_t c) {
  g.fillCircle(x - 18, y + 4, 14, c);
  g.fillCircle(x + 2, y - 8, 20, c);
  g.fillCircle(x + 22, y + 4, 14, c);
  g.fillRoundRect(x - 32, y + 2, 68, 18, 9, c);
}

static void sun(TFT_eSPI& g, int x, int y, int r) {
  for (int a = 0; a < 360; a += 45) {
    float rad = a * DEG_TO_RAD;
    g.drawWideLine(x + cosf(rad) * (r + 6), y + sinf(rad) * (r + 6),
                   x + cosf(rad) * (r + 14), y + sinf(rad) * (r + 14), 4, TFT_YELLOW, COL_BG);
  }
  g.fillCircle(x, y, r, TFT_YELLOW);
}

static void drawWeatherIcon(TFT_eSPI& g, int code, int x, int y) {
  const uint16_t gray = 0xBDF7;
  if (code == 0) { sun(g, x, y, 22); return; }
  if (code <= 2) { sun(g, x - 14, y - 12, 16); cloud(g, x + 6, y + 8, gray); return; }
  if (code == 45 || code == 48) {
    for (int i = 0; i < 4; i++) g.fillRoundRect(x - 34 + (i % 2) * 8, y - 18 + i * 12, 60, 6, 3, gray);
    return;
  }
  cloud(g, x, y - 6, gray);
  if (code == 3) return;
  if (code >= 95) {  // lightning
    g.fillTriangle(x - 2, y + 14, x + 10, y + 14, x - 6, y + 34, TFT_YELLOW);
    g.fillTriangle(x + 2, y + 26, x + 12, y + 26, x - 6, y + 46, TFT_YELLOW);
    return;
  }
  bool snow = (code >= 71 && code <= 77) || code == 85 || code == 86;
  for (int i = -1; i <= 1; i++) {
    if (snow) g.fillCircle(x + i * 18, y + 28 + (i & 1) * 6, 4, TFT_WHITE);
    else g.drawWideLine(x + i * 18 + 4, y + 20, x + i * 18 - 2, y + 34, 3, 0x3D9F, COL_BG);
  }
}

// ---------- app ----------
void WeatherApp::enter(TFT_eSPI& tft) {
  shownVersion = 0xFFFFFFFF;  // force a redraw
  shownBusy = false;
}

void WeatherApp::onTouch(const TouchEvent& e) {
  if (e.type == TouchEvent::TAP) WeatherService::refreshNow();  // tap to refresh
}

void WeatherApp::update(TFT_eSPI& tft, uint32_t now) {
  uint32_t v = WeatherService::version();
  bool connected = Net::connected();
  uint32_t minute = now / 60000;  // also redraw once a minute so "updated N min ago" stays right
  if (v != shownVersion || connected != shownConnected || minute != shownMinute) {
    shownVersion = v;
    shownConnected = connected;
    shownMinute = minute;
    draw(tft);
    shownBusy = false;
  }
  // little "updating" dot in the corner while a download runs
  bool busy = WeatherService::busy();
  if (busy != shownBusy) {
    shownBusy = busy;
    tft.fillCircle(SCREEN_W - 10, 10, 4, busy ? COL_ACCENT : COL_BG);
  }
}

void WeatherApp::draw(TFT_eSPI& tft) {
  WeatherData w;
  bool has = WeatherService::get(w);
  String err = WeatherService::lastError();

  tft.fillScreen(COL_BG);
  tft.setTextPadding(0);
  tft.setTextDatum(MC_DATUM);

  if (!has) {
    tft.setTextColor(COL_DIM, COL_BG);
    if (!Net::connected()) {
      tft.drawString("No WiFi", SCREEN_W / 2, SCREEN_H / 2, 4);
      tft.drawString("hold, then tap WiFi to pick a network", SCREEN_W / 2, SCREEN_H / 2 + 30, 2);
    } else if (err.length()) {
      tft.drawString("Weather unavailable", SCREEN_W / 2, SCREEN_H / 2, 4);
      tft.drawString(err + " - retrying", SCREEN_W / 2, SCREEN_H / 2 + 30, 2);
    } else {
      tft.drawString("Loading weather...", SCREEN_W / 2, SCREEN_H / 2, 4);
    }
    return;
  }

  const char* unit = WEATHER_UNITS_F ? "F" : "C";
  const char* windUnit = WEATHER_UNITS_F ? "mph" : "km/h";

  drawWeatherIcon(tft, w.code, 80, 90);

  // Big temperature (font 7 has digits only, so draw the degree symbol by hand)
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_FG, COL_BG);
  String t = String((int)roundf(w.temp));
  tft.drawString(t, 160, 52, 7);
  int tw = tft.textWidth(t, 7);
  tft.drawCircle(160 + tw + 8, 58, 5, COL_FG);
  tft.drawCircle(160 + tw + 8, 58, 4, COL_FG);
  tft.drawString(unit, 160 + tw + 16, 56, 4);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(describe(w.code), SCREEN_W / 2, 150, 4);

  char line[64];
  tft.setTextColor(COL_DIM, COL_BG);
  snprintf(line, sizeof(line), "H %d  L %d   feels %d", (int)roundf(w.hi), (int)roundf(w.lo), (int)roundf(w.feels));
  tft.drawString(line, SCREEN_W / 2, 185, 2);
  snprintf(line, sizeof(line), "wind %d %s   humidity %d%%", (int)roundf(w.wind), windUnit, w.humidity);
  tft.drawString(line, SCREEN_W / 2, 208, 2);

  // How old the data is
  uint32_t mins = (millis() - w.fetchedAt) / 60000;
  tft.setTextDatum(TL_DATUM);
  String age = "updated just now";
  if (mins > 0) age = "updated " + String(mins) + " min ago";
  tft.drawString(age, 6, 4, 2);
}

void WeatherApp::drawIcon(TFT_eSPI& tft, int cx, int cy) {
  const uint16_t gray = 0xBDF7;
  tft.fillCircle(cx - 5, cy - 6, 8, TFT_YELLOW);
  tft.fillCircle(cx - 3, cy + 4, 7, gray);
  tft.fillCircle(cx + 5, cy + 1, 8, gray);
  tft.fillRoundRect(cx - 12, cy + 3, 25, 9, 4, gray);
}
