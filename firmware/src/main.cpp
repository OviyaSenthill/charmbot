// Charmbot — a pocket robot companion on the ESP32-2432S028R.
//
// Navigation: swipe left/right to change apps, long-press to go home.

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "settings.h"
#include "core/AppManager.h"
#include "core/Input.h"
#include "core/Net.h"

#include "apps/FaceApp.h"
#include "apps/ClockApp.h"
#include "apps/WeatherApp.h"
#include "apps/SpotifyApp.h"
#include "apps/SnakeApp.h"

TFT_eSPI tft;
Input input;
AppManager apps;

FaceApp    faceApp;     // index 0 = home
ClockApp   clockApp;
WeatherApp weatherApp;
SpotifyApp spotifyApp;
SnakeApp   snakeApp;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[charmbot] booting");

  tft.init();
  tft.setRotation(1);  // landscape, USB port on the right on most boards
  tft.fillScreen(COL_BG);

  input.begin();
  Net::begin();

  apps.begin(&tft);
  apps.add(&faceApp);
  apps.add(&clockApp);
  apps.add(&weatherApp);
  apps.add(&spotifyApp);
  apps.add(&snakeApp);
  apps.switchTo(0);
}

void loop() {
  uint32_t now = millis();
  Net::loop(now);
  apps.handle(input.poll(now));
  apps.update(now);
  delay(2);
}
