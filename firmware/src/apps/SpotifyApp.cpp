#include "apps/SpotifyApp.h"
#include "core/Net.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TJpg_Decoder.h>
#include <base64.h>

static const uint32_t POLL_MS = 5000;

// Layout: album art on the left, text on the right, progress + controls along the bottom
static const int ART_X = 10, ART_Y = 28, ART_SIZE = 150;
static const int TEXT_X = 170, TEXT_W = SCREEN_W - TEXT_X - 6;
static const int BAR_X = 10, BAR_Y = 188, BAR_W = 300;
static const int CTRL_Y = 218;
static const size_t MAX_ART_BYTES = 100 * 1024;

static bool configured() { return strlen(SPOTIFY_REFRESH_TOKEN) > 0 && strlen(SPOTIFY_CLIENT_ID) > 0; }

static String fmtTime(uint32_t ms) {
  uint32_t s = ms / 1000;
  char buf[8];
  snprintf(buf, sizeof(buf), "%u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
  return buf;
}

// Draw text word-wrapped into maxLines lines; returns the y below the last line
static int wrapText(TFT_eSPI& tft, const String& text, int x, int y, int maxW, int font, int maxLines, int lineH) {
  String rest = text;
  for (int line = 0; line < maxLines && rest.length(); line++) {
    // Take as many words as fit
    int cut = rest.length();
    while (cut > 0 && tft.textWidth(rest.substring(0, cut), font) > maxW) {
      int sp = rest.lastIndexOf(' ', cut - 1);
      cut = sp > 0 ? sp : cut - 1;  // no space left: break mid-word
    }
    if (cut <= 0) cut = 1;
    String piece = rest.substring(0, cut);
    rest = rest.substring(cut);
    rest.trim();

    if (line == maxLines - 1 && rest.length()) {  // last line: add "..."
      while (piece.length() > 1 && tft.textWidth(piece + "...", font) > maxW) piece.remove(piece.length() - 1);
      piece += "...";
    }
    tft.drawString(piece, x, y, font);
    y += lineH;
  }
  return y;
}

// TJpg_Decoder hands us decoded blocks of pixels; push them to the screen
static TFT_eSPI* jpgTarget = nullptr;
static bool jpgOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (!jpgTarget || y >= SCREEN_H) return false;
  jpgTarget->pushImage(x, y, w, h, bitmap);
  return true;
}

// ---------- lifecycle ----------
void SpotifyApp::enter(TFT_eSPI& tft) {
  fullRedraw = true;
  lastPoll = 0;  // poll right away
}

void SpotifyApp::exit() {
  freeArt();  // give the RAM back to other apps; re-downloaded next time
  loadedArtUrl = "";
}

void SpotifyApp::onTouch(const TouchEvent& e) {
  if (e.type != TouchEvent::TAP || e.y < CTRL_Y - 30) return;
  pendingCommand = e.x < 107 ? 0 : (e.x < 213 ? 1 : 2);
}

void SpotifyApp::update(TFT_eSPI& tft, uint32_t now) {
  if (!configured()) {
    if (fullRedraw) {
      fullRedraw = false;
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
        progressMs += now - progressStamp;
        progressStamp = now;
        drawControls(tft);
      }
      if (cmd == 2) sendCommand("POST", "/v1/me/player/next");
      lastPoll = now - POLL_MS + 700;  // re-poll shortly after a command
    }

    if (lastPoll == 0 || now - lastPoll > POLL_MS) {
      lastPoll = now;
      String oldKey = trackKey, oldStatus = status;
      bool oldHas = hasTrack, oldPlaying = playing;
      pollNowPlaying();
      if (trackKey != oldKey || hasTrack != oldHas || status != oldStatus) fullRedraw = true;
      else if (playing != oldPlaying) drawControls(tft);
    }
  } else if (status != "No WiFi") {
    status = "No WiFi";
    fullRedraw = true;
  }

  if (fullRedraw) {
    fullRedraw = false;
    drawAll(tft);
  }

  // New song: fetch its album art (after the text is already on screen)
  if (hasTrack && artUrl.length() && artUrl != loadedArtUrl && Net::connected()) {
    loadedArtUrl = artUrl;  // only try once per song
    if (downloadArt(artUrl)) drawArt(tft);
  }

  if (hasTrack && playing && now - lastProgressDraw > 1000) {
    lastProgressDraw = now;
    drawProgress(tft, now);
  }
}

// ---------- Spotify API ----------
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

  // The response is big. Keep only what we need.
  JsonDocument filter;
  filter["is_playing"] = true;
  filter["progress_ms"] = true;
  filter["item"]["id"] = true;
  filter["item"]["name"] = true;
  filter["item"]["duration_ms"] = true;
  filter["item"]["artists"][0]["name"] = true;
  filter["item"]["album"]["images"][0]["url"] = true;
  filter["item"]["album"]["images"][0]["width"] = true;

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
  String id = doc["item"]["id"] | "";
  trackKey = id;
  if (trackKey.isEmpty()) trackKey = track + "|" + artist;  // local files have no id
  progressMs = doc["progress_ms"] | 0;
  durationMs = doc["item"]["duration_ms"] | 0;
  progressStamp = millis();

  // Album art comes in 640, 300 and 64 px. 300 px at half scale = our 150 px box.
  artUrl = "";
  JsonArray imgs = doc["item"]["album"]["images"];
  int bestW = 0;
  for (JsonObject img : imgs) {
    int w = img["width"] | 0;
    String url = img["url"] | "";
    if (!url.length()) continue;
    bool better = artUrl.isEmpty() || (w == 300) || (bestW != 300 && w >= 150 && (bestW < 150 || w < bestW));
    if (better) { artUrl = url; bestW = w; }
  }
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

// ---------- album art ----------
void SpotifyApp::freeArt() {
  if (artBuf) free(artBuf);
  artBuf = nullptr;
  artLen = 0;
}

bool SpotifyApp::downloadArt(const String& url) {
  freeArt();
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.useHTTP10(true);  // plain body, no chunk markers
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code != 200) {
    Serial.printf("[spotify] art HTTP %d\n", code);
    http.end();
    return false;
  }

  int len = http.getSize();
  size_t cap = len > 0 ? (size_t)len : MAX_ART_BYTES;
  if (cap > MAX_ART_BYTES) {
    http.end();
    return false;
  }
  uint8_t* buf = (uint8_t*)malloc(cap);
  if (!buf) {
    Serial.println("[spotify] not enough RAM for album art");
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  size_t got = 0;
  uint32_t t0 = millis();
  while (got < cap && millis() - t0 < 6000) {
    size_t avail = stream->available();
    if (avail) {
      got += stream->readBytes(buf + got, min(avail, cap - got));
    } else if (!http.connected()) {
      break;
    } else {
      delay(2);
    }
  }
  http.end();

  if (got < 100 || (len > 0 && got < (size_t)len)) {
    Serial.printf("[spotify] art download incomplete (%u of %d bytes)\n", (unsigned)got, len);
    free(buf);
    return false;
  }
  artBuf = buf;
  artLen = got;
  return true;
}

// ---------- drawing ----------
void SpotifyApp::drawArt(TFT_eSPI& tft) {
  if (!artBuf) {
    // Placeholder: a big music note
    tft.fillRoundRect(ART_X, ART_Y, ART_SIZE, ART_SIZE, 10, COL_PANEL);
    int cx = ART_X + ART_SIZE / 2, cy = ART_Y + ART_SIZE / 2;
    tft.fillEllipse(cx - 12, cy + 26, 18, 14, 0x39E7);
    tft.fillRect(cx + 3, cy - 36, 8, 62, 0x39E7);
    tft.fillTriangle(cx + 11, cy - 36, cx + 36, cy - 20, cx + 11, cy - 10, 0x39E7);
    return;
  }

  uint16_t w = 0, h = 0;
  TJpgDec.getJpgSize(&w, &h, artBuf, artLen);
  uint8_t scale = 1;
  while (scale < 8 && (w / scale > ART_SIZE || h / scale > ART_SIZE)) scale *= 2;

  tft.fillRect(ART_X, ART_Y, ART_SIZE, ART_SIZE, COL_BG);
  jpgTarget = &tft;
  TJpgDec.setJpgScale(scale);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(jpgOutput);
  int x = ART_X + (ART_SIZE - w / scale) / 2;
  int y = ART_Y + (ART_SIZE - h / scale) / 2;
  TJpgDec.drawJpg(x, y, artBuf, artLen);
}

void SpotifyApp::drawText(TFT_eSPI& tft) {
  tft.fillRect(TEXT_X - 2, ART_Y, SCREEN_W - TEXT_X + 2, ART_SIZE, COL_BG);
  tft.setTextDatum(TL_DATUM);

  if (!hasTrack) {
    tft.setTextColor(COL_DIM);
    wrapText(tft, status.length() ? status : String("Loading..."), TEXT_X, ART_Y + 40, TEXT_W, 4, 3, 26);
    return;
  }

  tft.setTextColor(COL_FG);
  int y = wrapText(tft, track, TEXT_X, ART_Y + 4, TEXT_W, 4, 3, 26);
  tft.setTextColor(COL_ACCENT);
  y = wrapText(tft, artist, TEXT_X, y + 4, TEXT_W, 2, 2, 17);

  if (status.length()) {
    tft.setTextColor(COL_WARN);
    wrapText(tft, status, TEXT_X, y + 4, TEXT_W, 2, 1, 17);
  }
}

void SpotifyApp::drawAll(TFT_eSPI& tft) {
  tft.fillScreen(COL_BG);
  tft.setTextPadding(0);

  // Header
  tft.setTextDatum(TL_DATUM);
  tft.fillCircle(16, 13, 6, COL_SPOTIFY);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.drawString("NOW PLAYING", 28, 6, 2);

  drawArt(tft);  // the real art if we already have it, otherwise the placeholder
  drawText(tft);
  if (hasTrack) drawProgress(tft, millis());
  drawControls(tft);
}

void SpotifyApp::drawProgress(TFT_eSPI& tft, uint32_t now) {
  uint32_t p = progressMs + (playing ? now - progressStamp : 0);
  if (durationMs && p > durationMs) p = durationMs;
  int filled = durationMs ? (int)((uint64_t)BAR_W * p / durationMs) : 0;

  tft.fillRoundRect(BAR_X, BAR_Y, BAR_W, 6, 3, 0x39E7);
  if (filled > 0) tft.fillRoundRect(BAR_X, BAR_Y, max(filled, 6), 6, 3, COL_SPOTIFY);

  // Elapsed / total, bottom of the text column
  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(COL_DIM, COL_BG);
  tft.setTextPadding(TEXT_W);
  tft.drawString(fmtTime(p) + " / " + fmtTime(durationMs), TEXT_X, ART_Y + ART_SIZE, 2);
  tft.setTextPadding(0);
}

void SpotifyApp::drawControls(TFT_eSPI& tft) {
  const int y = CTRL_Y;
  tft.fillRect(0, y - 20, SCREEN_W, SCREEN_H - (y - 20), COL_BG);

  // previous |<<
  tft.fillRect(40, y - 10, 4, 20, COL_FG);
  tft.fillTriangle(66, y - 10, 66, y + 10, 46, y, COL_FG);

  // play / pause in a circle
  tft.fillCircle(160, y, 18, COL_SPOTIFY);
  if (playing) {
    tft.fillRect(152, y - 8, 5, 16, COL_BG);
    tft.fillRect(163, y - 8, 5, 16, COL_BG);
  } else {
    tft.fillTriangle(154, y - 9, 154, y + 9, 170, y, COL_BG);
  }

  // next >>|
  tft.fillTriangle(254, y - 10, 254, y + 10, 274, y, COL_FG);
  tft.fillRect(276, y - 10, 4, 20, COL_FG);
}

void SpotifyApp::drawIcon(TFT_eSPI& tft, int cx, int cy) {
  // Music note
  tft.fillEllipse(cx - 4, cy + 9, 6, 5, COL_SPOTIFY);
  tft.fillRect(cx, cy - 12, 3, 21, COL_SPOTIFY);
  tft.fillTriangle(cx + 3, cy - 12, cx + 12, cy - 6, cx + 3, cy - 3, COL_SPOTIFY);
}
