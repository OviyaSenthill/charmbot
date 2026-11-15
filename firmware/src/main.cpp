// Charmbot — a pocket robot companion on the ESP32-2432S028R.
//
// Navigation:
//   swipe left/right  -> next / previous app
//   long-press        -> app wheel (robot face in the middle = home, Games folder inside)

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "settings.h"
#include "core/AppManager.h"
#include "core/Input.h"
#include "core/Net.h"
#include "core/Mood.h"
#include "core/Backlight.h"
#include "core/MusicWatch.h"
#include "core/WeatherService.h"

#include "apps/FaceApp.h"
#include "apps/ClockApp.h"
#include "apps/WeatherApp.h"
#include "apps/SpotifyApp.h"
#include "apps/SnakeApp.h"
#include "apps/FlappyApp.h"
#include "apps/Game2048App.h"
#include "apps/WifiApp.h"

TFT_eSPI tft;
Input input;
AppManager apps;

FaceApp     faceApp;     // index 0 = home
ClockApp    clockApp;
WeatherApp  weatherApp;
SpotifyApp  spotifyApp;
SnakeApp    snakeApp;
FlappyApp   flappyApp;
Game2048App game2048App;
WifiApp     wifiApp;

// What shows up on the app wheel (clockwise from the top)
App* wheelApps[]  = {&clockApp, &weatherApp, &spotifyApp, &wifiApp};
App* wheelGames[] = {&snakeApp, &flappyApp, &game2048App};

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[charmbot] booting");
  randomSeed(esp_random());

  tft.init();
  tft.setRotation(1);  // landscape
  tft.fillScreen(COL_BG);
  Backlight::begin();

  input.begin();
  Net::begin();
  Mood::begin();
  MusicWatch::begin();
  WeatherService::begin();

  apps.begin(&tft);
  apps.add(&faceApp);
  apps.add(&clockApp);
  apps.add(&weatherApp);
  apps.add(&spotifyApp);
  apps.add(&wifiApp);
  apps.add(&snakeApp);
  apps.add(&flappyApp);
  apps.add(&game2048App);

  apps.wheel().setApps(wheelApps, sizeof(wheelApps) / sizeof(wheelApps[0]));
  apps.wheel().setGames(wheelGames, sizeof(wheelGames) / sizeof(wheelGames[0]));

  apps.switchTo(0);
}

void loop() {
  uint32_t now = millis();
  Net::loop(now);
  TouchEvent e = input.poll(now);
  if (e.type != TouchEvent::NONE) Mood::interaction(now);
  apps.handle(e);
  apps.update(now);
  Mood::update(now);
  Backlight::update(now);
  delay(2);
}
