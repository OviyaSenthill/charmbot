#pragma once
// Pin map for the ESP32-2432S028R ("Cheap Yellow Display"), common revision.
// Display pins live in platformio.ini (TFT_eSPI build flags).
// Verify against your board — some revisions differ. See hardware/wiring.md.

// Touch controller (XPT2046) — its own SPI bus (VSPI peripheral, custom pins)
#define TOUCH_CLK   25
#define TOUCH_MOSI  32
#define TOUCH_MISO  39
#define TOUCH_CS    33
#define TOUCH_IRQ   36

// microSD slot (not used yet — see docs/roadmap.md about the VSPI conflict)
#define SD_CS       5
#define SD_SCK      18
#define SD_MISO     19
#define SD_MOSI     23

// On-board RGB LED (active LOW)
#define LED_R       4
#define LED_G       16
#define LED_B       17

#define LDR_PIN     34   // light sensor (analog)
#define SPEAKER_PIN 26   // speaker connector (via on-board amp)
#define BACKLIGHT_PIN 21

// Free pins on the CN1 / P3 headers — your expansion budget
#define EXP_IO22    22   // good for I2C SDA/SCL
#define EXP_IO27    27   // good for I2C SDA/SCL
#define EXP_IO35    35   // INPUT ONLY
