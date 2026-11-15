#include "core/AppManager.h"

void AppManager::add(App* app) {
  if (count < MAX_APPS) apps[count++] = app;
}

int AppManager::indexOf(App* app) const {
  for (int i = 0; i < count; i++)
    if (apps[i] == app) return i;
  return -1;
}

void AppManager::switchTo(int index) {
  if (count == 0) return;
  index = ((index % count) + count) % count;  // wrap around
  if (current >= 0) apps[current]->exit();
  current = index;
  tft->fillScreen(COL_BG);
  apps[current]->enter(*tft);
  Serial.printf("[apps] -> %s (free heap %u, largest block %u)\n", apps[current]->name(),
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
}

void AppManager::switchTo(App* app) {
  int i = indexOf(app);
  if (i >= 0) switchTo(i);
}

// Swipe to the next/previous app, skipping games
void AppManager::step(int dir) {
  for (int k = 1; k < count; k++) {
    int i = (((current + dir * k) % count) + count) % count;
    if (!apps[i]->isGame()) {
      switchTo(i);
      return;
    }
  }
}

void AppManager::closeWheelAndOpen(App* app) {
  wheel_.close();
  if (app == apps[current]) apps[current]->resume(*tft);  // same app: pick up where you left off
  else switchTo(app);
}

void AppManager::handle(const TouchEvent& e) {
  if (e.type == TouchEvent::NONE || current < 0) return;
  App* app = apps[current];

  // ---- Wheel is open: it gets every touch ----
  if (wheel_.isOpen()) {
    if (e.type == TouchEvent::LONG_PRESS) {
      closeWheelAndOpen(app);
      return;
    }
    if (e.type != TouchEvent::TAP) return;

    WheelChoice c = wheel_.tap(*tft, e.x, e.y);
    if (c.type == WheelChoice::CLOSE) closeWheelAndOpen(app);
    else if (c.type == WheelChoice::HOME) closeWheelAndOpen(apps[0]);
    else if (c.type == WheelChoice::OPEN) closeWheelAndOpen(c.app);
    return;
  }

  // ---- Normal navigation ----
  if (e.type == TouchEvent::LONG_PRESS) {
    wheel_.open(*tft, app);
    return;
  }

  if (!app->capturesSwipes()) {
    if (e.type == TouchEvent::SWIPE_LEFT)  { step(+1); return; }
    if (e.type == TouchEvent::SWIPE_RIGHT) { step(-1); return; }
  }

  app->onTouch(e);
}

void AppManager::update(uint32_t now) {
  if (current < 0) return;
  if (wheel_.isOpen()) wheel_.update(*tft, now);
  else apps[current]->update(*tft, now);
}
