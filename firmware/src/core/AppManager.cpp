#include "core/AppManager.h"

void AppManager::add(App* app) {
  if (count < MAX_APPS) apps[count++] = app;
}

void AppManager::switchTo(int index) {
  if (count == 0) return;
  index = ((index % count) + count) % count;  // wrap around
  if (current >= 0) apps[current]->exit();
  current = index;
  tft->fillScreen(COL_BG);
  apps[current]->enter(*tft);
  Serial.printf("[apps] -> %s (free heap %u)\n", apps[current]->name(), ESP.getFreeHeap());
}

void AppManager::handle(const TouchEvent& e) {
  if (e.type == TouchEvent::NONE || current < 0) return;
  App* app = apps[current];

  if (e.type == TouchEvent::LONG_PRESS) {
    if (current != 0) switchTo(0);
    else app->onTouch(e);
    return;
  }

  if (!app->capturesSwipes()) {
    if (e.type == TouchEvent::SWIPE_LEFT)  { switchTo(current + 1); return; }
    if (e.type == TouchEvent::SWIPE_RIGHT) { switchTo(current - 1); return; }
  }

  app->onTouch(e);
}

void AppManager::update(uint32_t now) {
  if (current >= 0) apps[current]->update(*tft, now);
}
