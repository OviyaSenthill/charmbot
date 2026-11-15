# Roadmap

Each item is a good GitHub issue.

## v0.1: desk dashboard
- [x] Flash and verify display + touch
- [x] Face app with 10 expressions, blinking and glancing
- [ ] Clock (NTP)
- [ ] Weather (Open-Meteo)
- [ ] Spotify now playing + controls
- [x] Snake

## v0.1.1: app wheel + games (current)
- [ ] App wheel on long-press, with a Games folder
- [ ] Flappy Bot
- [ ] 2048
- [ ] Games pause and keep progress; best scores saved to flash

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
- [ ] Shake → dizzy via the kit's tilt ball switch (supported, see docs/mood.md)
- [ ] MPU-6050 (needs buying) on IO22/IO27 (I²C): tilt → eyes look, double-tap → wake
- [ ] LiPo + TP4056 + boost/LDO power
- [ ] Sleep after inactivity, wake on tap
- [ ] Print fit test, then case v2 (see `hardware/case/`)
- [ ] Battery + charger + boost wired per `hardware/bom.md`

## More games
- [ ] Stack (tap to drop blocks)
- [ ] Pong vs. the robot
- [ ] Simon Says with robot expressions
- [ ] Virtual pet mode tied to the mood engine
