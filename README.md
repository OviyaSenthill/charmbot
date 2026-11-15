# Charmbot 🤖

A pocket robot companion built on the **ESP32-2432S028R** ("Cheap Yellow Display").
Desk dashboard today, 3D-printed bag charm tomorrow.

![expressions](docs/face-preview.png)

## What it does

Swipe left/right between apps; **long-press anywhere to go home**.

| App | What it shows |
|---|---|
| 🤖 **Face** (home) | Animated robot face with 10 expressions. Blinks, glances around, floats. Tap to cycle expressions. |
| 🕐 **Clock** | Big time + date, synced over WiFi |
| ⛅ **Weather** | Current conditions from Open-Meteo (no API key). Tap to refresh. |
| 🎵 **Spotify** | Now playing + progress, prev / play-pause / next |
| 🐍 **Snake** | Swipe, or tap beside the snake's head, to turn |

## Hardware

- ESP32-2432S028R (2.8" ILI9341 320×240 + XPT2046 resistive touch)
- Later: MPU-6050 (from the Elegoo kit), LiPo + TP4056, 3D-printed cartridge case

Pin map: [`hardware/wiring.md`](hardware/wiring.md) · Case notes: [`hardware/case/`](hardware/case/README.md)

## Quick start

1. Install [VS Code](https://code.visualstudio.com/) + the **PlatformIO** extension.
2. Open the `firmware/` folder in VS Code.
3. Copy `firmware/include/config.example.h` → `firmware/include/config.h` and fill in your
   WiFi, location, and (optionally) Spotify details.
4. Plug in the board and click **Upload** (→) in the PlatformIO toolbar.
   If the upload hangs on "Connecting...", hold the **BOOT** button until it starts.
5. Open the **Serial Monitor** (115200 baud) to see logs.

> **Status:** the firmware hasn't been compiled or flashed yet. Expect a few fixes on
> first build. Open an issue with the error output.

## Project layout

```
firmware/                 PlatformIO project
  include/                pins.h, config.example.h, theme.h
  src/core/               App base class, AppManager (navigation), Input (touch gestures), Net (WiFi/NTP)
  src/apps/               FaceApp (+ FaceStyle.h), ClockApp, WeatherApp, SpotifyApp, SnakeApp
docs/                     face design guide + Canva template, Spotify setup, roadmap
hardware/                 wiring, case CAD/STL
tools/                    face_preview.py, spotify_auth.py
web/                      phone upload page (planned)
```

## Adding an app

1. Make a class that extends `App` (see `src/core/App.h`) in `src/apps/`.
2. Allocate big buffers in `enter()` and free them in `exit()`, since RAM is tight.
3. Register it in `main.cpp` with `apps.add(&myApp);`.

## Designing the face

See [`docs/face-design.md`](docs/face-design.md): Canva setup, shape rules, and how to map
your design onto `FaceStyle.h`. Preview without flashing:

```bash
pip install pillow && python3 tools/face_preview.py
```

## Troubleshooting

| Symptom | Fix |
|---|---|
| White or blank screen | Your board may have an ST7789 display. In `platformio.ini`, swap `ILI9341_2_DRIVER` for `ST7789_DRIVER`. |
| Colors inverted | Add `-DTFT_INVERSION_ON` (or `_OFF`) to `build_flags` |
| Touch is offset or mirrored | Set `TOUCH_DEBUG 1`, tap the corners, update the `TOUCH_*` min/max values |
| Upload fails | Hold BOOT while uploading; install the CH340 USB driver if the port doesn't show up |
| Spotify says "not set up" | Follow [`docs/spotify-setup.md`](docs/spotify-setup.md) |

## Roadmap

Mood engine, phone photo/text uploads, motion reactions, battery, and the case:
see [`docs/roadmap.md`](docs/roadmap.md).
