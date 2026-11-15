#include "apps/SpotifyApp.h"
#include "core/Net.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

static const uint32_t POLL_MS = 5000;
static const int BAR_Y = 150, BAR_X = 20, BAR_W = 280;
static const int CTRL_Y = 200;

static bool configured() { return strlen(SPOTIFY_REFRESH_TOKEN) > 0 && strlen(SPOTIFY_CLIENT_ID) > 0; }

static String fmtTime(uint32_t ms) {
  uint32_t s = ms / 1000;
  char buf[8];
  snprintf(buf, sizeof(buf), "%u:%02u", s / 60, s % 60);
  return buf;
}

// Shorten text with "..." so it fits in maxW pixels
static String fit(TFT_eSPI& tft, String s, int maxW, int font) {
  if (tft.textWidth(s, font) <= maxW) return s;
  while (s.length() > 1 && tft.textWidth(s + "...", font) > maxW) s.remove(s.length() - 1);
  return s + "...";
}

void SpotifyApp::enter(TFT_eSPI& tft) {
  dirty = true;
  lastPoll = 0;  // poll right away
}

void SpotifyApp::onTouch(const TouchEvent& e) {
  if (e.type != TouchEvent::TAP || e.y < CTRL_Y - 30) return;
  pendingCommand = e.x < 107 ? 0 : (e.x < 213 ? 1 : 2);
}

void SpotifyApp::update(TFT_eSPI& tft, uint32_t now) {
  if (!configured()) {
    if (dirty) {
      dirty = false;
      tft.fillScreen(COL_BG);
      tft.setTextDatum(MC_DATUM);
      tft.setTextColor(COL_SPOTIFY, COL_BG);
      tft.drawString("Spotify not set up", SCREEN_W / 2, 100, 4);
      tft.setTextColor(COL_DIM, COL_BG);
      tft.drawString("See docs/spotify-setup.md", SCREEN_W / 2, 135, 2);
    }
    return;
  }

  if (Net::connected()) {
    if (pendingCommand >= 0) {
      int cmd = pendingCommand;
      pendingCommand = -1;
      if (cmd == 0) sendCommand("POST", "/v1/me/player/previous");
      if (cmd == 1) {
        sendCommand("PUT", playing ? "/v1/me/player/pause" : "/v1/me/player/play");
        playing = !playing;  // optimistic update
        drawControls(tft);
      }
      if (cmd == 2) sendCommand("POST", "/v1/me/player/next");
      lastPoll = now - POLL_MS + 700;  // re-poll shortly after a command
    }
    if (lastPoll == 0 || now - lastPoll > POLL_MS) {
      lastPoll = now;
      pollNowPlaying();
      dirty = true;
    }
  } else if (status != "No WiFi") {
    status = "No WiFi";
    dirty = true;
  }

  if (dirty) {
    dirty = false;
    drawAll(tft);
  }
  if (hasTrack && playing && now - lastProgressDraw > 1000) {
    lastProgressDraw = now;
    drawProgress(tft, now);
  }
}

bool SpotifyApp::refreshAccessToken() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, "https://accounts.spotify.com/api/token");
  http.addHeader("Authorization",
                 "Basic " + base64::encode(String(SPOTIFY_CLIENT_ID) + ":" + SPOTIFY_CLIENT_SECRET));
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int code = http.POST(String("grant_type=refresh_token&refresh_token=") + SPOTIFY_REFRESH_TOKEN);
  if (code != 200) {
    status = "Spotify auth failed (" + String(code) + ")";
    Serial.printf("[spotify] token refresh HTTP %d: %s\n", code, http.getString().c_str());
    http.end();
    return false;
  }
  JsonDocument doc;
  deserializeJson(doc, http.getString());
  http.end();
  token = doc["access_token"].as<String>();
  int expiresIn = doc["expires_in"] | 3600;
  tokenExpiresAt = millis() + (uint32_t)(expiresIn - 60) * 1000;
  return token.length() > 0;
}

bool SpotifyApp::pollNowPlaying() {
  if (token.isEmpty() || (int32_t)(millis() - tokenExpiresAt) > 0) {
    if (!refreshAccessToken()) return false;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);  // no chunked encoding, so we can stream-parse
  http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + token);
  int code = http.GET();

  if (code == 204) {  // nothing playing
    hasTrack = false;
    status = "Nothing playing";
    http.end();
    return true;
  }
  if (code == 401) {  // token expired early
    token = "";
    http.end();
    return false;
  }
  if (code != 200) {
    status = "Spotify error " + String(code);
    http.end();
    return false;
  }

  // The response is big (album art URLs, markets...). Keep only what we need.
  JsonDocument filter;
  filter["is_playing"] = true;
  filter["progress_ms"] = true;
  filter["item"]["name"] = true;
  filter["item"]["duration_ms"] = true;
  filter["item"]["artists"][0]["name"] = true;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();
  if (err) {
    status = "Parse error";
    return false;
  }

  if (doc["item"].isNull()) {  // ads, or some podcasts
    hasTrack = false;
    status = "Ad / unknown item";
    return true;
  }
  hasTrack = true;
  status = "";
  playing = doc["is_playing"] | false;
  track = doc["item"]["name"].as<String>();
  artist = doc["item"]["artists"][0]["name"].as<String>();
  progressMs = doc["progress_ms"] | 0;
  durationMs = doc["item"]["duration_ms"] | 0;
  progressStamp = millis();
  return true;
}

bool SpotifyApp::sendCommand(const char* method, const char* path) {
  if (token.isEmpty() && !refreshAccessToken()) return false;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, String("https://api.spotify.com") + path);
  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("Content-Length", "0");  // Spotify rejects empty bodies without it
  int code = http.sendRequest(method, (uint8_t*)nullptr, 0);
  http.end();
  Serial.printf("[spotify] %s %s -> %d\n", method, path, code);
  if (code == 403) status = "Controls need Premium";
  return code >= 200 && code < 300;
}

// ---------- drawing ----------
void SpotifyApp::drawAll(TFT_eSPI& tft) {
  tft.fillScreen(COL_BG);
  tft.setTextPadding(0);

  // Header
  tft.setTextDatum(TL_DATUM);
  tft.fillCircle(16, 14, 6, COL_SPOTIFY);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("NOW PLAYING", 28, 7, 2);

  if (!hasTrack) {
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(COL_DIM, COL_BG);
    tft.drawString(status.length() ? status : String("Loading..."), SCREEN_W / 2, 100, 4);
    drawControls(tft);
    return;
  }

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_FG, COL_BG);
  tft.drawString(fit(tft, track, SCREEN_W - 40, 4), BAR_X, 55, 4);
  tft.setTextColor(COL_ACCENT, COL_BG);
  tft.drawString(fit(tft, artist, SCREEN_W - 40, 2), BAR_X, 92, 2);

  if (status.length()) {
    tft.setTextColor(COL_WARN, COL_BG);
    tft.drawString(status, BAR_X, 115, 2);
  }

  drawProgress(tft, millis());
  drawControls(tft);
}

void SpotifyApp::drawProgress(TFT_eSPI& tft, uint32_t now) {
  uint32_t p = progressMs + (playing ? now - progressStamp : 0);
  if (durationMs && p > durationMs) p = durationMs;
  int filled = durationMs ? (int)((uint64_t)BAR_W * p / durationMs) : 0;

  tft.fillRoundRect(BAR_X, BAR_Y, BAR_W, 6, 3, 0x39E7);
  if (filled > 0) tft.fillRoundRect(BAR_X, BAR_Y, max(filled, 6), 6, 3, COL_SPOTIFY);

  tft.setTextPadding(50);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(fmtTime(p), BAR_X, BAR_Y + 12, 2);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(fmtTime(durationMs), BAR_X + BAR_W, BAR_Y + 12, 2);
  tft.setTextPadding(0);
}

void SpotifyApp::drawControls(TFT_eSPI& tft) {
  const int y = CTRL_Y + 10;
  tft.fillRect(0, CTRL_Y - 12, SCREEN_W, SCREEN_H - CTRL_Y + 12, COL_BG);

  // previous |<<
  tft.fillRect(40, y - 10, 4, 20, COL_FG);
  tft.fillTriangle(66, y - 10, 66, y + 10, 46, y, COL_FG);

  // play / pause in a circle
  tft.fillCircle(160, y, 20, COL_SPOTIFY);
  if (playing) {
    tft.fillRect(152, y - 9, 5, 18, COL_BG);
    tft.fillRect(163, y - 9, 5, 18, COL_BG);
  } else {
    tft.fillTriangle(154, y - 10, 154, y + 10, 171, y, COL_BG);
  }

  // next >>|
  tft.fillTriangle(254, y - 10, 254, y + 10, 274, y, COL_FG);
  tft.fillRect(276, y - 10, 4, 20, COL_FG);
}
