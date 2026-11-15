# Roadmap

Each item is a good GitHub issue.

## v0.1: desk dashboard (current)
- [ ] Flash and verify display + touch (calibrate `TOUCH_*` in config.h)
- [ ] Face app with 10 expressions, blinking and glancing
- [ ] Clock (NTP)
- [ ] Weather (Open-Meteo)
- [ ] Spotify now playing + controls
- [ ] Snake
- [ ] Page dots / app name toast when switching apps

## v0.2: personality
- [ ] Mood engine: events (music started, tapped a lot, ignored, time of day) → expressions
- [ ] Face reacts to Spotify (bobbing while music plays)
- [ ] Look toward the tap point
- [ ] Sleepy at night, auto-dim using the light sensor (LDR on IO34)
- [ ] Move network calls to a background FreeRTOS task so the UI never freezes during HTTPS requests

## v0.3: phone uploads + photos
- [ ] Web page served by the ESP32 (`web/`) at `http://charmbot.local`
- [ ] Send text from the phone to a Notes screen
- [ ] Photo upload: the phone browser resizes to 320×240 JPEG before sending
- [ ] Photo frame app (SD card + TJpg_Decoder)
  - Heads-up: the touch controller and the SD card both want the VSPI peripheral.
    Common fix: run touch over software SPI (e.g. an XPT2046 bit-bang library) and give VSPI to the SD card.
- [ ] Access-point mode (`Charmbot` WiFi) for uploads away from home

## v0.4: charm-ify
- [ ] MPU-6050 on IO22/IO27 (I²C): tilt → eyes look, shake → dizzy, double-tap → wake
- [ ] LiPo + TP4056 + boost/LDO power
- [ ] Sleep after inactivity, wake on tap
- [ ] Cartridge case v1 (see `hardware/case/`)

## More games
- [ ] Pong vs. the robot
- [ ] Flappy
- [ ] Virtual pet mode tied to the mood engine
