#pragma once
#include "core/App.h"

// Now playing on Spotify + prev / play-pause / next.
// Setup: docs/spotify-setup.md. Controls need Spotify Premium.
class SpotifyApp : public App {
public:
  const char* name() const override { return "Spotify"; }
  void enter(TFT_eSPI& tft) override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;

private:
  bool refreshAccessToken();
  bool pollNowPlaying();
  bool sendCommand(const char* method, const char* path);
  void drawAll(TFT_eSPI& tft);
  void drawProgress(TFT_eSPI& tft, uint32_t now);
  void drawControls(TFT_eSPI& tft);

  String token;
  uint32_t tokenExpiresAt = 0;

  bool hasTrack = false;
  bool playing = false;
  String track, artist, status;
  uint32_t progressMs = 0, durationMs = 0, progressStamp = 0;

  uint32_t lastPoll = 0;
  uint32_t lastProgressDraw = 0;
  bool dirty = true;
  int pendingCommand = -1;  // 0 prev, 1 play/pause, 2 next
};
