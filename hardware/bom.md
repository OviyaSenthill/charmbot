# Parts list (BOM)

## Already have
| Part | Notes |
|---|---|
| ESP32-2432S028R (CYD) | the brain + screen |
| Tilt ball switch (Elegoo kit) | optional "shake me" sensor → dizzy face. See `docs/mood.md` |
| Jumper wires (Elegoo kit) | for prototyping |

## To buy: battery power (makes it a real charm)
| Part | Example / spec | ~Price | Why |
|---|---|---|---|
| **LiPo battery** | 3.7 V, **503450** (5 × 34 × 50 mm, ~1000 mAh) or **503040** (5 × 30 × 40 mm, ~500 mAh), with a JST-PH 2.0 plug and built-in protection | $6–10 | Power. Must be ≤ 5–6 mm thick to fit under the board |
| **Charger board** | **TP4056 with USB-C and protection** (the version with 6 pads: IN+/IN−, B+/B−, OUT+/OUT−) | $1–2 | Charges the battery. Its USB-C port lines up with the case's charge opening |
| **5 V boost converter** | **MT3608** (adjustable) or a fixed-5 V mini boost board | $1–2 | The CYD needs 5 V; the battery gives 3.7 V |
| **Mini slide switch** | **SS12D00** (or any ~8 × 4 mm slide switch) | $0.50 | On/off. Fits the slot on the bottom edge |
| Thin wire | 26–28 AWG silicone wire | $5 | Connections |

> **Easier option:** a single "LiPo charge + 5 V boost" combo board (often sold as a
> "Type-C 5V 1A charge/discharge module"). It replaces the TP4056 + MT3608, means less
> wiring, and is about the same size.

**Battery life (rough):** the CYD draws about 115 mA with the screen on. Through a boost converter
that's roughly 180 mA from the battery: **~5 h on 1000 mAh, ~2.5 h on 500 mAh** with the screen
at full brightness. Longer when the robot sleeps and dims itself.

## To buy: case hardware
| Part | Notes |
|---|---|
| 4× **M3 × 15 mm** self-tapping screws (or machine screws + M3 heat-set inserts) | hold the sandwich together |
| Split ring + **lobster clasp** (or a bag-charm hook) | goes through the 5.5 mm antenna loop |
| Double-sided foam tape / Kapton tape | stick the battery + boards down, insulate them from the board above |
| PETG filament (or PLA) | PETG survives hot cars |

## Optional extras
| Part | Why |
|---|---|
| **Mini speaker** 8 Ω, 0.5–1 W, ~20 mm, with a **1.25 mm JST** 2-pin plug | robot beeps and chirps. Plugs into the CYD's speaker connector; sits behind the back grille |
| **MPU-6050 (GY-521)** | real tilt sensing (eyes slide downhill). I²C on IO22/IO27 |
| **INMP441** I²S microphone | voice memos later (needs 3 pins; see `docs/roadmap.md`) |
| JST 1.25 mm 4-pin cables | for the CYD's CN1 / P3 connectors (often included with the board) |

## Power wiring

```
LiPo (+/-) ──► TP4056  B+ / B-
               TP4056  OUT+ ──► slide switch ──► MT3608 IN+
               TP4056  OUT- ───────────────────► MT3608 IN-
                                                 MT3608 OUT+ ──► CYD 5V  (VIN pin on the 4-pin P1 connector)
                                                 MT3608 OUT- ──► CYD GND
```

1. **Set the MT3608 to 5.0 V before connecting it to the CYD.** Power it from the battery, measure
   the output with a multimeter, and turn its little screw until it reads 5.0 V. Out of the
   box they're often set much higher, which can kill the board.
2. Charge through the **TP4056's USB-C** (the opening on the case's left side, opposite the board's USB).
3. **Flip the switch OFF before plugging the CYD into your computer** to upload code, so the
   boost converter and the USB port aren't both powering the board at the same time.
4. Insulate everything under the board with Kapton or foam tape so nothing shorts against the
   board's back.

Check your board's labels: the P1 connector is usually marked **VIN / TX / RX / GND**.
If yours differs, send a photo before wiring.

## Layout under the board (6 mm battery bay, ~73 × 51 mm)

```
 ┌──────────────── case (looking from the front, board removed) ───────────────┐
 │ [TP4056]◄ charge port   ┌──────────── LiPo 503450 ────────────┐               │
 │  (rotated)              │                                    │               │
 │                         └────────────────────────────────────┘    board USB ►│
 │                         [ MT3608 boost ]                                       │
 └──────────────────────────────── [switch] ──────────────────────────────────────┘
```
