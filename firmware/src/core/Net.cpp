#include "core/Net.h"
#include <WiFi.h>
#include <Preferences.h>
#include <time.h>
#include "settings.h"

namespace {
  bool timeConfigured = false;
  bool scanned = false;
  bool paused = false;
  uint32_t lastAttempt = 0;
  String curSsid, curPass;

  // Saved network from the WiFi screen, else the one in config.h
  void loadSaved(String& ssid, String& pass) {
    ssid = "";
    pass = "";
    Preferences prefs;
    if (prefs.begin("wifi", true)) {
      ssid = prefs.getString("ssid", "");
      pass = prefs.getString("pass", "");
      prefs.end();
    }
    if (ssid.isEmpty()) {
      ssid = WIFI_SSID;
      pass = WIFI_PASSWORD;
    }
  }

  void startConnect() {
    WiFi.disconnect();
    WiFi.begin(curSsid.c_str(), curPass.c_str());
    lastAttempt = millis();
    Serial.printf("[net] connecting to \"%s\"\n", curSsid.c_str());
  }

  const char* statusName(wl_status_t s) {
    switch (s) {
      case WL_NO_SSID_AVAIL:   return "network not found (wrong name, or it's 5 GHz only)";
      case WL_CONNECT_FAILED:  return "connect failed (wrong password?)";
      case WL_CONNECTION_LOST: return "connection lost";
      case WL_DISCONNECTED:    return "disconnected (still trying, or wrong password)";
      case WL_IDLE_STATUS:     return "idle";
      default:                 return "unknown";
    }
  }

  // Print the networks the ESP32 can actually see (it only sees 2.4 GHz)
  void printVisibleNetworks() {
    Serial.println("[net] scanning for networks the ESP32 can see...");
    WiFi.disconnect();  // a scan can't run while a connection attempt is in progress
    delay(200);
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      Serial.printf("[net]   \"%s\"  (signal %d dBm, channel %d)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
    }
    if (n == 0) Serial.println("[net]   none found");
    if (n < 0) Serial.printf("[net]   scan failed (code %d)\n", n);
    WiFi.scanDelete();
  }
}

namespace Net {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  loadSaved(curSsid, curPass);
  startConnect();
}

void loop(uint32_t now) {
  if (WiFi.status() == WL_CONNECTED) {
    if (!timeConfigured) {
      configTzTime(TZ_INFO, "pool.ntp.org", "time.nist.gov");
      timeConfigured = true;
    }
    static String announced;
    if (announced != curSsid) {
      announced = curSsid;
      Serial.printf("[net] connected to \"%s\", IP %s\n", curSsid.c_str(), WiFi.localIP().toString().c_str());
    }
    return;
  }
  if (paused) return;

  // Retry every 15 s, and explain what's wrong in the serial monitor
  if (now - lastAttempt > 15000) {
    wl_status_t s = WiFi.status();
    Serial.printf("[net] not connected: %s (status %d)\n", statusName(s), (int)s);
    if (!scanned) {  // once is enough
      scanned = true;
      printVisibleNetworks();
    }
    startConnect();
  }
}

bool connected() { return WiFi.status() == WL_CONNECTED; }

void setPaused(bool p) {
  paused = p;
  lastAttempt = millis();
}

void connectTo(const String& ssid, const String& pass) {
  curSsid = ssid;
  curPass = pass;
  startConnect();
}

void save() {
  Preferences prefs;
  prefs.begin("wifi", false);
  prefs.putString("ssid", curSsid);
  prefs.putString("pass", curPass);
  prefs.end();
  Serial.printf("[net] saved \"%s\"\n", curSsid.c_str());
}

void restoreSaved() {
  loadSaved(curSsid, curPass);
  startConnect();
}

String currentSsid() { return curSsid; }

String savedSsid() {
  String s, p;
  loadSaved(s, p);
  return s;
}

String savedPassword() {
  String s, p;
  loadSaved(s, p);
  return p;
}

}  // namespace Net
