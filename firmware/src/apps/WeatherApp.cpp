#include "apps/WeatherApp.h"
#include "core/Net.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static const uint32_t REFRESH_MS = 15UL * 60 * 1000;
static const uint32_t RETRY_MS = 30UL * 1000;

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

static void drawIcon(TFT_eSPI& g, int code, int x, int y) {
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
void WeatherApp::onTouch(const TouchEvent& e) {
  if (e.type == TouchEvent::TAP) forceRefresh = true;  // tap to refresh
}

void WeatherApp::update(TFT_eSPI& tft, uint32_t now) {
  bool due = !hasData || now - lastFetch > REFRESH_MS || forceRefresh;
  if (due && Net::connected() && (now - lastAttempt > RETRY_MS || lastAttempt == 0 || forceRefresh)) {
    lastAttempt = now;
    forceRefresh = false;
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString("updating...", SCREEN_W - 6, 6, 2);
    if (fetch()) lastFetch = now;
    dirty = true;
  }
  if (dirty) {
    dirty = false;
    draw(tft);
  }
}

bool WeatherApp::fetch() {
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(WEATHER_LAT, 4) +
               "&longitude=" + String(WEATHER_LON, 4) +
               "&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m"
               "&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto";
#if WEATHER_UNITS_F
  url += "&temperature_unit=fahrenheit&wind_speed_unit=mph";
#endif

  WiFiClientSecure client;
  client.setInsecure();  // skip cert check — fine for public weather data
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  int status = http.GET();
  if (status != 200) {
    Serial.printf("[weather] HTTP %d\n", status);
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;

  JsonObject cur = doc["current"];
  temp = cur["temperature_2m"] | 0.0f;
  feels = cur["apparent_temperature"] | 0.0f;
  humidity = cur["relative_humidity_2m"] | 0;
  code = cur["weather_code"] | 0;
  wind = cur["wind_speed_10m"] | 0.0f;
  hi = doc["daily"]["temperature_2m_max"][0] | 0.0f;
  lo = doc["daily"]["temperature_2m_min"][0] | 0.0f;
  hasData = true;
  return true;
}

void WeatherApp::draw(TFT_eSPI& tft) {
  tft.fillScreen(COL_BG);
  tft.setTextPadding(0);

  if (!hasData) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString(Net::connected() ? "Loading weather..." : "No WiFi", SCREEN_W / 2, SCREEN_H / 2, 4);
    return;
  }

  const char* unit = WEATHER_UNITS_F ? "F" : "C";
  const char* windUnit = WEATHER_UNITS_F ? "mph" : "km/h";

  drawIcon(tft, code, 80, 90);

  // Big temperature (font 7 has digits only, so draw the degree symbol by hand)
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_FG, COL_BG);
  String t = String((int)roundf(temp));
  tft.drawString(t, 160, 52, 7);
  int w = tft.textWidth(t, 7);
  tft.drawCircle(160 + w + 8, 58, 5, COL_FG);
  tft.drawCircle(160 + w + 8, 58, 4, COL_FG);
  tft.drawString(unit, 160 + w + 16, 56, 4);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(describe(code), SCREEN_W / 2, 150, 4);

  char line[64];
  tft.setTextColor(COL_DIM, COL_BG);
  snprintf(line, sizeof(line), "H %d  L %d   feels %d", (int)roundf(hi), (int)roundf(lo), (int)roundf(feels));
  tft.drawString(line, SCREEN_W / 2, 185, 2);
  snprintf(line, sizeof(line), "wind %d %s   humidity %d%%", (int)roundf(wind), windUnit, humidity);
  tft.drawString(line, SCREEN_W / 2, 208, 2);
}
