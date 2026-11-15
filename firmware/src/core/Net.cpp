#include "core/Net.h"
#include <WiFi.h>
#include <time.h>
#include "settings.h"

namespace {
  bool timeConfigured = false;
  uint32_t lastAttempt = 0;
}

namespace Net {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastAttempt = millis();
  Serial.printf("[net] connecting to %s\n", WIFI_SSID);
}

void loop(uint32_t now) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!timeConfigured) {
      configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
      timeConfigured = true;
      Serial.printf("[net] connected, IP %s\n", WiFi.localIP().toString().c_str());
    }
    return;
  }
  // Retry every 15 s if auto-reconnect hasn't worked
  if (now - lastAttempt > 15000) {
    lastAttempt = now;
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

bool connected() { return WiFi.status() == WL_CONNECTED; }

}  // namespace Net
