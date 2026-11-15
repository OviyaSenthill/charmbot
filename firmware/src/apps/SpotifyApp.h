#pragma once
#include "core/App.h"

// Now playing on Spotify: album art, title, artist, progress, prev / play-pause / next.
// Setup: docs/spotify-setup.md. Needs Spotify Premium.
class SpotifyApp : public App {
public:
  const char* name() const override { return "Spotify"; }
  void enter(TFT_eSPI& tft) override;
  void exit() override;
  void update(TFT_eSPI& tft, uint32_t now) override;
  void onTouch(const TouchEvent& e) override;
  void drawIcon(TFT_eSPI& tft, int cx, int cy) override;

private:
  bool refreshAccessToken();
  bool pollNowPlaying();
  bool sendCommand(const char* method, const char* path);

  bool downloadArt(const String& url);
  void freeArt();

  void drawAll(TFT_eSPI& tft);
  void drawArt(TFT_eSPI& tft);
  void drawText(TFT_eSPI& tft);
  void drawProgress(TFT_eSPI& tft, uint32_t now);
  void drawControls(TFT_eSPI& tft);

  String token;
  uint32_t tokenExpiresAt = 0;

  bool hasTrack = false;
  bool playing = false;
  String trackKey, track, artist, status;
  String artUrl, loadedArtUrl;
  uint32_t progressMs = 0, durationMs = 0, progressStamp = 0;

  uint8_t* artBuf = nullptr;   // album art JPEG, kept while the app is open
  size_t artLen = 0;

  uint32_t lastPoll = 0;
  uint32_t lastProgressDraw = 0;
  bool fullRedraw = true;
  int pendingCommand = -1;  // 0 prev, 1 play/pause, 2 next
};
