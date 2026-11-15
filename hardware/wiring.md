# Wiring & pin map: ESP32-2432S028R (CYD)

Values are for the common revision. **Check yours.** Some boards use an ST7789 display
or a slightly different pinout (see the troubleshooting section in the README).

## On-board (already wired)

| Function | Pins |
|---|---|
| Display ILI9341 (HSPI) | MOSI 13, MISO 12, SCK 14, CS 15, DC 2, backlight 21 |
| Touch XPT2046 (VSPI, custom pins) | CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36 |
| microSD | CS 5, SCK 18, MISO 19, MOSI 23 |
| RGB LED (active LOW) | R 4, G 16, B 17 |
| Light sensor (LDR) | 34 (analog in) |
| Speaker connector | 26 (through on-board amp) |

## Free pins: your expansion budget

| Pin | Header | Notes |
|---|---|---|
| IO22 | CN1 / P3 | general purpose, good for I²C |
| IO27 | CN1 | general purpose, good for I²C (ADC2: no analog reads while WiFi is on) |
| IO35 | P3 | **input only**, ADC1 analog OK |

Planned use:
- **MPU-6050 (from the Elegoo kit):** SDA → IO27, SCL → IO22, VCC → 3.3 V, GND → GND
  (the GY-521 board works at 3.3 V)

## Power (for the charm version)

Don't feed a 3.7 V LiPo straight into the 5 V pin. The on-board 3.3 V regulator needs
more headroom than that. Options:
- LiPo → TP4056 charger → small 5 V boost converter → 5 V pin
- LiPo → TP4056 → low-dropout 3.3 V regulator (e.g. HT7333) → 3.3 V rail
