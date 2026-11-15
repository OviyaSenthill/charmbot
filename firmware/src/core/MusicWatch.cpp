#include "core/MusicWatch.h"
#include "core/Net.h"
#include "settings.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

namespace {
  volatile bool enabled = false;
  volatile bool primed = false;      // first result after enabling just sets the baseline
  volatile bool isPlaying = false;
  volatile uint32_t startedMs = 0;
  volatile uint32_t lastPollAt = 0;

  String token;
  uint32_t tokenExp = 0;
  String lastId;

  bool configured() { return strlen(SPOTIFY_REFRESH_TOKEN) > 0 && strlen(SPOTIFY_CLIENT_ID) > 0; }

  bool refresh() {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.begin(client, "https://accounts.spotify.com/api/token");
    http.addHeader("Authorization", "Basic " + base64::encode(String(SPOTIFY_CLIENT_ID) + ":" + SPOTIFY_CLIENT_SECRET));
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int code = http.POST(String("grant_type=refresh_token&refresh_token=") + SPOTIFY_REFRESH_TOKEN);
    if (code != 200) { http.end(); return false; }
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    http.end();
    token = doc["access_token"].as<String>();
    tokenExp = millis() + (uint32_t)((doc["expires_in"] | 3600) - 60) * 1000;
    return token.length() > 0;
  }

  void poll() {
    if (token.isEmpty() || (int32_t)(millis() - tokenExp) > 0) {
      if (!refresh()) return;
    }
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.useHTTP10(true);
    http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing");
    http.addHeader("Authorization", "Bearer " + token);
    int code = http.GET();

    bool p = false;
    String id;
    if (code == 200) {
      JsonDocument filter;
      filter["is_playing"] = true;
      filter["item"]["id"] = true;
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
      http.end();
      if (err) return;
      p = doc["is_playing"] | false;
      id = doc["item"]["id"] | "";
    } else {
      http.end();
      if (code == 401) token = "";
      if (code != 204) return;  // 204 = nothing playing
    }

    if (!primed) {
      primed = true;  // don't "react" to a song that was already playing when the face appeared
    } else if (p && (!isPlaying || id != lastId)) {
      startedMs = millis();
    }
    isPlaying = p;
    lastId = id;
  }

  void task(void*) {
    for (;;) {
      if (enabled && configured() && Net::connected() &&
          (lastPollAt == 0 || millis() - lastPollAt > 12000)) {
        lastPollAt = millis();
        poll();
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

namespace MusicWatch {

void begin() { xTaskCreatePinnedToCore(task, "music", 12288, nullptr, 1, nullptr, 0); }

void setEnabled(bool on) {
  if (on && !enabled) {
    primed = false;
    lastPollAt = 0;  // check right away
  }
  enabled = on;
}

bool playing() { return isPlaying; }
uint32_t startedAt() { return startedMs; }

}  // namespace MusicWatch
