#include "core/WeatherService.h"
#include "core/Net.h"
#include "settings.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

namespace {
  const uint32_t REFRESH_MS = 15UL * 60 * 1000;
  const uint32_t RETRY_MS = 20UL * 1000;
  const uint32_t MANUAL_MIN_MS = 15UL * 1000;

  SemaphoreHandle_t lock;
  WeatherData data;          // guarded by lock
  String error;              // guarded by lock
  volatile uint32_t ver = 0;
  volatile bool fetching = false;
  volatile bool requested = false;
  uint32_t lastAttempt = 0, lastSuccess = 0;

  String buildPath() {
    String p = "/v1/forecast?latitude=" + String(WEATHER_LAT, 4) +
               "&longitude=" + String(WEATHER_LON, 4) +
               "&current=temperature_2m,apparent_temperature,relative_humidity_2m,weather_code,wind_speed_10m"
               "&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto";
#if WEATHER_UNITS_F
    p += "&temperature_unit=fahrenheit&wind_speed_unit=mph";
#endif
    return p;
  }

  // Returns the HTTP status (200 = ok) and fills body. Plain HTTP first (fast), HTTPS as a fallback.
  int download(String& body) {
    String path = buildPath();
    {
      WiFiClient client;
      HTTPClient http;
      http.setTimeout(6000);
      if (http.begin(client, "http://api.open-meteo.com" + path)) {
        int code = http.GET();
        if (code == 200) { body = http.getString(); http.end(); return 200; }
        Serial.printf("[weather] http %d, trying https\n", code);
        http.end();
      }
    }
    WiFiClientSecure client;
    client.setInsecure();  // public weather data, no secrets involved
    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(client, "https://api.open-meteo.com" + path)) return -1;
    int code = http.GET();
    if (code == 200) body = http.getString();
    http.end();
    return code;
  }

  void fetch() {
    fetching = true;
    uint32_t t0 = millis();
    String body, err;
    WeatherData d;
    int code = download(body);

    if (code != 200) {
      if (code < 0) err = "Can't reach weather server";
      else err = "Weather server error " + String(code);
    } else {
      JsonDocument doc;
      if (deserializeJson(doc, body)) {
        err = "Couldn't read weather data";
      } else {
        JsonObject cur = doc["current"];
        d.temp = cur["temperature_2m"] | 0.0f;
        d.feels = cur["apparent_temperature"] | 0.0f;
        d.humidity = cur["relative_humidity_2m"] | 0;
        d.code = cur["weather_code"] | 0;
        d.wind = cur["wind_speed_10m"] | 0.0f;
        d.hi = doc["daily"]["temperature_2m_max"][0] | 0.0f;
        d.lo = doc["daily"]["temperature_2m_min"][0] | 0.0f;
        d.valid = true;
        d.fetchedAt = millis();
      }
    }

    xSemaphoreTake(lock, portMAX_DELAY);
    if (d.valid) { data = d; lastSuccess = millis(); }
    error = err;
    xSemaphoreGive(lock);
    ver++;
    fetching = false;
    Serial.printf("[weather] %s in %u ms\n", err.length() ? err.c_str() : "updated", (unsigned)(millis() - t0));
  }

  void task(void*) {
    for (;;) {
      uint32_t now = millis();
      bool due = lastSuccess == 0 || now - lastSuccess > REFRESH_MS;
      bool canTry = lastAttempt == 0 || now - lastAttempt > RETRY_MS;
      if (Net::connected() && ((due && canTry) || requested)) {
        requested = false;
        lastAttempt = now;
        fetch();
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}

namespace WeatherService {

void begin() {
  lock = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(task, "weather", 12288, nullptr, 1, nullptr, 0);
}

bool get(WeatherData& out) {
  xSemaphoreTake(lock, portMAX_DELAY);
  out = data;
  xSemaphoreGive(lock);
  return out.valid;
}

uint32_t version() { return ver; }
bool busy() { return fetching; }

String lastError() {
  xSemaphoreTake(lock, portMAX_DELAY);
  String e = error;
  xSemaphoreGive(lock);
  return e;
}

void refreshNow() {
  if (millis() - lastAttempt > MANUAL_MIN_MS) requested = true;
}

}  // namespace WeatherService
