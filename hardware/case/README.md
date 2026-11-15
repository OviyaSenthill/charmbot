# Cartridge case

Put CAD source files in `cad/` (Fusion `.f3d`, `.step`, Onshape links...) and printable
files in `stl/`.

## Design concept

A robot helmet / cartridge. The screen is the dark visor, and rounded "ear" pods on each
side hide the screws. One ear (or a top antenna) holds a reinforced keyring loop.

## Measure your board first

Board revisions vary, so measure with calipers and record the numbers here:

| Measurement | Approx. | Yours |
|---|---|---|
| PCB width × height | 86 × 50 mm | |
| PCB thickness | 1.6 mm | |
| Total depth (PCB + display + tallest part on the back) | ~12 mm | |
| Visible screen area | ~58 × 43 mm | |
| Mounting hole positions | 4 corners, M3 | |
| USB port position + size | | |
| Reset / BOOT button positions | | |
| LDR (light sensor) position | | |

## Openings checklist

- [ ] Screen window, with ~0.5 mm clearance around the display edges. A resistive
      screen registers false touches if the bezel presses on it.
- [ ] USB port
- [ ] Reset button (a pinhole is fine) + BOOT button access
- [ ] Light sensor window
- [ ] Speaker grille (later)
- [ ] Power switch (once a battery is added)

## Size budget with a battery

~92 × 58 × 18–22 mm. Leave room behind the board for a flat LiPo (e.g. 503040,
~500 mAh) plus the charger board.

## Print tips

- **PETG** over PLA: PLA warps in a hot car.
- M2 heat-set inserts + screws, not snap-fits. A bag charm gets thrown around.
- Make the keyring loop thick (≥ 3 mm walls) and print it so the layer lines run
  *around* the loop, not across it.
- Optional: a recessed label area on the back for your own sticker art.
- A separate desk stand that the cartridge clips into turns it into the dashboard.
