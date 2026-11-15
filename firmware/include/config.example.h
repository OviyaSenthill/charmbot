#pragma once
// Copy this file to config.h (same folder) and fill in your values.
// config.h is gitignored so your WiFi password and Spotify tokens stay private.

// ---- WiFi ----
#define WIFI_SSID      "your-wifi-name"
#define WIFI_PASSWORD  "your-wifi-password"

// ---- Time ----
// POSIX timezone string. Pacific time:
#define TZ_INFO        "PST8PDT,M3.2.0,M11.1.0"
#define CLOCK_24H      0

// ---- Weather (Open-Meteo, no API key needed) ----
#define WEATHER_LAT     32.7157   // San Diego
#define WEATHER_LON   -117.1611
#define WEATHER_UNITS_F 1         // 1 = °F / mph, 0 = °C / km/h

// ---- Spotify (see docs/spotify-setup.md) ----
#define SPOTIFY_CLIENT_ID      ""
#define SPOTIFY_CLIENT_SECRET  ""
#define SPOTIFY_REFRESH_TOKEN  ""

// ---- Touch calibration ----
// Set TOUCH_DEBUG to 1, open the serial monitor, tap the corners,
// and update these with the raw min/max you see.
#define TOUCH_DEBUG  0
#define TOUCH_X_MIN  200
#define TOUCH_X_MAX  3700
#define TOUCH_Y_MIN  240
#define TOUCH_Y_MAX  3800
