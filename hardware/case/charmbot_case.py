#!/usr/bin/env python3
"""
Charmbot case v2: a cute robot-helmet shell for the ESP32-2432S028R (CYD).

  - squishy rounded body, pillowy back and front edges
  - big round "headphone" ears with a button in the middle
  - antenna on top with a ball-loop for a keyring / lobster clasp
  - little blush-cheek recesses under the screen (paint them or add stickers)
  - a heart on the back, tiny feet on the bottom
  - room for a LiPo battery + charger + boost board, with a charge port and power switch

Parts (all print WITHOUT supports):
  case_back.stl   - shell the board screws into. Print BACK-FACE DOWN (antenna loop flat on the bed).
  case_front.stl  - visor bezel. Print FRONT-FACE DOWN.
  fit_test.stl    - thin frame to check screen window + holes against your board (~10 min).

Usage:   pip install cadquery && python3 charmbot_case.py
Every dimension is below. Measure your board and edit.
"""
import os
import cadquery as cq

# ============================================================
#  BOARD (measure yours!)
# ============================================================
PCB_W = 86.0          # board length (mm)
PCB_H = 50.0          # board width
PCB_T = 1.6
HOLE_INSET = 3.2      # center of each corner hole from the two nearest edges
HOLE_D = 3.2
DISPLAY_T = 4.2       # screen module height above the board
BACK_PARTS_H = 5.5    # tallest part on the BACK of the board

SCREEN_W = 60.0       # visor window (lit area is ~57.6 x 43.2)
SCREEN_H = 46.0
SCREEN_R = 5.0        # rounded visor corners
SCREEN_DX = 0.0       # shift window right(+)/left(-) to center it on the lit pixels
SCREEN_DY = 0.0       # shift up(+)/down(-)

USB_SIDE = +1         # board's USB port: +1 = right short side, -1 = left
USB_DY = 0.0
USB_W, USB_H = 13.0, 8.0

# ============================================================
#  POWER (set BATTERY_SPACE = 0 for a USB-powered desk version)
# ============================================================
BATTERY_SPACE = 6.0   # room under the board for a flat LiPo + charger/boost boards
CHARGE_PORT = True    # opening for the charger's USB-C (opposite side from the board's USB)
CHARGE_W, CHARGE_H = 10.0, 4.6
SWITCH_SLOT = True    # slot on the bottom edge for a mini slide switch (SS12D00)
SWITCH_W, SWITCH_H = 9.5, 4.5

# ============================================================
#  SHAPE + CUTE STUFF
# ============================================================
CLEAR = 0.4
WALL = 5.0            # thick walls = room for big round corners
BACK_T = 2.0
FRONT_T = 2.8
OUTER_R = 13.0        # big rounded corners (squishy look)
INNER_R = 1.2
BACK_ROUND = 4.0      # pillowy back edge
FRONT_ROUND = 2.2     # pillowy front edge
SCREEN_GAP = 0.3
SCREW_PILOT_D = 2.6   # M3 self-tapping (4.0 for heat-set inserts)
SCREW_CLEAR_D = 3.4
SCREW_HEAD_D = 6.4
SCREW_HEAD_H = 1.8   # deep enough for M3 hex-socket heads to sit flush

EAR_R_BASE = 11.5     # big round headphone ears
EAR_R_TIP = 5.5       # 45-degree taper, prints without supports
EAR_LEN = 6.0
EAR_BUTTON = True     # small raised button on each ear

ANTENNA_STALK_W = 6.0
ANTENNA_STALK_L = 5.0
ANTENNA_BALL_R = 8.0
ANTENNA_HOLE_D = 5.5  # fits a standard split ring / lobster clasp ring
ANTENNA_THICK = 5.0   # keep >= 4 for strength

# Stylus holder: a "headband" tube along the top edge. Measure your stylus!
STYLUS = True
STYLUS_D = 5.0        # stylus diameter at its THICKEST point (mm)
STYLUS_FIT = 0.35     # extra room in the hole (bigger = looser). Friction holds it in.
STYLUS_WALL = 2.0     # tube wall thickness
STYLUS_OPEN_SIDE = +1 # which end the stylus slides in: +1 right, -1 left

CHEEKS = True         # blush recesses under the visor (0.6 mm deep)
HEART = True          # heart on the back
FEET = True           # two little feet on the bottom edge
GRILLE_SLOTS = 3      # speaker grille on the back (0 = none)

# ============================================================
#  derived
# ============================================================
IN_W, IN_H = PCB_W + 2 * CLEAR, PCB_H + 2 * CLEAR
OUT_W, OUT_H = IN_W + 2 * WALL, IN_H + 2 * WALL
Z_PCB_BOTTOM = BACK_T + BATTERY_SPACE + BACK_PARTS_H
Z_PCB_TOP = Z_PCB_BOTTOM + PCB_T
Z_RIM = Z_PCB_TOP + DISPLAY_T + SCREEN_GAP
BOSS_H = Z_RIM - Z_PCB_TOP
TOTAL_T = Z_RIM + FRONT_T
HOLES = [(sx * (PCB_W / 2 - HOLE_INSET), sy * (PCB_H / 2 - HOLE_INSET)) for sx in (-1, 1) for sy in (-1, 1)]


def rounded_rect(w, h, r, depth):
    return cq.Workplane("XY").rect(w, h).extrude(depth).edges("|Z").fillet(r)


def heart_shape(size, depth):
    r = size * 0.27
    left = cq.Workplane("XY").center(-r * 0.9, r * 0.35).circle(r).extrude(depth)
    right = cq.Workplane("XY").center(r * 0.9, r * 0.35).circle(r).extrude(depth)
    tip = (cq.Workplane("XY").polyline([(-r * 1.83, r * 0.05), (r * 1.83, r * 0.05), (0, -size * 0.55)])
           .close().extrude(depth))
    return left.union(right).union(tip)


def side_opening(side, dy, z, w, h, depth):
    """A rounded rectangular hole through a short side wall (side = +1 right, -1 left)."""
    hole = cq.Workplane("YZ").center(dy, z).rect(w, h).extrude(depth).edges("|X").fillet(min(w, h) / 2 - 0.2)
    if side > 0:
        return hole.translate((IN_W / 2 - 1, 0, 0))
    return hole.mirror("YZ").translate((-(IN_W / 2 - 1), 0, 0))


def make_back():
    shell = rounded_rect(OUT_W, OUT_H, OUTER_R, Z_RIM)
    shell = shell.faces("<Z").edges().fillet(BACK_ROUND)
    shell = shell.cut(rounded_rect(IN_W, IN_H, INNER_R, Z_RIM).translate((0, 0, BACK_T)))

    # Standoffs for the board
    for (x, y) in HOLES:
        post = cq.Workplane("XY").center(x, y).circle(3.3).extrude(Z_PCB_BOTTOM - BACK_T).translate((0, 0, BACK_T))
        pilot = (cq.Workplane("XY").center(x, y).circle(SCREW_PILOT_D / 2)
                 .extrude(Z_PCB_BOTTOM).translate((0, 0, BACK_T + 0.6)))
        shell = shell.union(post).cut(pilot)

    # Headphone ears (trimmed flush with the back and the front so they print flat)
    z_mid = TOTAL_T / 2
    keep = cq.Workplane("XY").box(OUT_W + 2 * EAR_LEN + 10, OUT_H, TOTAL_T, centered=(True, True, False))
    for side in (-1, 1):
        ear = cq.Workplane("YZ").circle(EAR_R_BASE).workplane(offset=EAR_LEN).circle(EAR_R_TIP).loft()
        if EAR_BUTTON:
            btn = (cq.Workplane("YZ").workplane(offset=EAR_LEN).circle(EAR_R_TIP - 1.5)
                   .workplane(offset=1.5).circle(EAR_R_TIP - 3.0).loft())
            ear = ear.union(btn)
        ear = ear.translate((0, 0, z_mid))
        if side < 0:
            ear = ear.mirror("YZ")
        ear = ear.translate((side * (OUT_W / 2 - 1.0), 0, 0))
        shell = shell.union(ear.intersect(keep))

    # Board USB port, charge port, switch slot
    shell = shell.cut(side_opening(USB_SIDE, USB_DY, Z_PCB_BOTTOM - 0.5, USB_W, USB_H, WALL + EAR_LEN + 6))
    batt_z = BACK_T + BATTERY_SPACE / 2 + 0.3
    if CHARGE_PORT and BATTERY_SPACE > 0:
        shell = shell.cut(side_opening(-USB_SIDE, 0, batt_z, CHARGE_W, CHARGE_H, WALL + EAR_LEN + 6))
    if SWITCH_SLOT and BATTERY_SPACE > 0:
        slot = (cq.Workplane("XZ").center(0, batt_z).rect(SWITCH_W, SWITCH_H).extrude(-(WALL + 4))
                .translate((0, -(IN_H / 2 - 1), 0)))
        shell = shell.cut(slot)

    # Feet
    if FEET:
        for fx in (-22, 22):
            foot = (cq.Workplane("XY").center(fx, -OUT_H / 2 - 1.0).slot2D(14, 9).extrude(7)
                    .faces(">Z").edges().fillet(2.5))
            shell = shell.union(foot)

    # Stylus holder tube along the top edge (lies on the bed; teardrop hole prints without supports)
    top = OUT_H / 2
    if STYLUS:
        bore_r = (STYLUS_D + STYLUS_FIT) / 2
        tube_r = bore_r + STYLUS_WALL
        ty = top + tube_r - 2.5            # overlaps the wall so it's one solid piece
        tz = tube_r
        tube_len = OUT_W - 2
        tube = (cq.Workplane("YZ").center(ty, tz).circle(tube_r).extrude(tube_len)
                .translate((-tube_len / 2, 0, 0)))
        tube = tube.intersect(cq.Workplane("XY").box(OUT_W + 20, 200, 200, centered=(True, True, False)))  # flat bottom
        try:
            tube = tube.edges("%CIRCLE").fillet(min(1.5, tube_r - 0.5))  # rounded ends
        except Exception:
            pass
        shell = shell.union(tube)
        # Teardrop-shaped bore: circle + a 45-degree point on top
        bore = (cq.Workplane("YZ").center(ty, tz).circle(bore_r).extrude(tube_len - 3)
                .union(cq.Workplane("YZ").polyline([(ty - bore_r * 0.707, tz + bore_r * 0.707),
                                                    (ty + bore_r * 0.707, tz + bore_r * 0.707),
                                                    (ty + bore_r * 0.3, tz + bore_r * 1.15),
                                                    (ty - bore_r * 0.3, tz + bore_r * 1.15)]).close().extrude(tube_len - 3)))
        # open at one end, closed (stop) at the other
        if STYLUS_OPEN_SIDE > 0:
            bore = bore.translate((-tube_len / 2 + 3, 0, 0))
        else:
            bore = bore.translate((-tube_len / 2, 0, 0))
        shell = shell.cut(bore)
        # Thumb notch at the open end so you can pinch the stylus out
        nx = STYLUS_OPEN_SIDE * (tube_len / 2 - 4)
        notch = cq.Workplane("XY").center(nx, ty).circle(4.5).extrude(20).translate((0, 0, tz))
        shell = shell.cut(notch)
        top = ty + tube_r - 1.5  # antenna sits on top of the tube

    # Antenna with a ball loop (lies flat on the back = strong when printed back-down)
    ball_y = top + ANTENNA_STALK_L + ANTENNA_BALL_R - 2
    stalk = cq.Workplane("XY").center(0, top + ANTENNA_STALK_L / 2 - 1.5).rect(ANTENNA_STALK_W, ANTENNA_STALK_L + 3).extrude(ANTENNA_THICK)
    ball = cq.Workplane("XY").center(0, ball_y).circle(ANTENNA_BALL_R).extrude(ANTENNA_THICK)
    antenna = stalk.union(ball)
    try:
        antenna = antenna.faces(">Z").edges().fillet(1.6)
    except Exception:
        pass
    shell = shell.union(antenna)
    shell = shell.cut(cq.Workplane("XY").center(0, ball_y).circle(ANTENNA_HOLE_D / 2).extrude(ANTENNA_THICK + 2).translate((0, 0, -1)))

    # Back decorations (recessed into the back face)
    if HEART:
        shell = shell.cut(heart_shape(16, 0.8).translate((18, 2, -0.2)))
    for i in range(GRILLE_SLOTS):
        slot = (cq.Workplane("XY").center(-20, (i - (GRILLE_SLOTS - 1) / 2) * 5)
                .slot2D(16, 2.2).extrude(BACK_T + 1).translate((0, 0, -0.5)))
        shell = shell.cut(slot)
    return shell


def make_front():
    plate = rounded_rect(OUT_W, OUT_H, OUTER_R, FRONT_T)
    plate = plate.faces(">Z").edges().fillet(FRONT_ROUND)       # front faces UP while building

    win = (cq.Workplane("XY").center(SCREEN_DX, SCREEN_DY).rect(SCREEN_W, SCREEN_H)
           .extrude(FRONT_T + 2).translate((0, 0, -1)).edges("|Z").fillet(SCREEN_R))
    plate = plate.cut(win)

    if CHEEKS:  # little oval blush spots below the visor corners
        band = (OUT_H / 2 - (SCREEN_H / 2 - SCREEN_DY)) / 2
        cy = -(SCREEN_H / 2 - SCREEN_DY) - band
        for cx in (-SCREEN_W / 2 + 4, SCREEN_W / 2 - 4):
            cheek = (cq.Workplane("XY").center(cx + SCREEN_DX, cy).ellipse(5.5, min(2.6, band - 1.2))
                     .extrude(1).translate((0, 0, FRONT_T - 0.6)))
            plate = plate.cut(cheek)

    for (x, y) in HOLES:
        boss = cq.Workplane("XY").center(x, y).circle(3.2).extrude(BOSS_H).translate((0, 0, -BOSS_H))
        thru = (cq.Workplane("XY").center(x, y).circle(SCREW_CLEAR_D / 2)
                .extrude(FRONT_T + BOSS_H + 2).translate((0, 0, -BOSS_H - 1)))
        cbore = (cq.Workplane("XY").center(x, y).circle(SCREW_HEAD_D / 2)
                 .extrude(SCREW_HEAD_H + 1).translate((0, 0, FRONT_T - SCREW_HEAD_H)))
        plate = plate.union(boss).cut(thru).cut(cbore)

    return plate.mirror("XY").translate((0, 0, FRONT_T))  # front face down for printing


def make_fit_test():
    t = 1.2
    frame = cq.Workplane("XY").rect(PCB_W, PCB_H).extrude(t)
    frame = frame.cut(cq.Workplane("XY").center(SCREEN_DX, SCREEN_DY).rect(SCREEN_W, SCREEN_H)
                      .extrude(t + 1).translate((0, 0, -0.5)).edges("|Z").fillet(SCREEN_R))
    for (x, y) in HOLES:
        frame = frame.cut(cq.Workplane("XY").center(x, y).circle(HOLE_D / 2).extrude(t + 1).translate((0, 0, -0.5)))
    notch = cq.Workplane("XY").center(USB_SIDE * PCB_W / 2, USB_DY).rect(4, USB_W).extrude(t + 1).translate((0, 0, -0.5))
    return frame.cut(notch)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    os.makedirs(os.path.join(here, "stl"), exist_ok=True)
    os.makedirs(os.path.join(here, "cad"), exist_ok=True)
    parts = {"case_back": make_back(), "case_front": make_front(), "fit_test": make_fit_test()}
    for name, part in parts.items():
        cq.exporters.export(part, os.path.join(here, "stl", f"{name}.stl"), tolerance=0.02, angularTolerance=0.1)
        cq.exporters.export(part, os.path.join(here, "cad", f"{name}.step"))
        bb = part.val().BoundingBox()
        print(f"{name:11s} {bb.xlen:6.1f} x {bb.ylen:6.1f} x {bb.zlen:5.1f} mm")
    print(f"\nScrews: 4x M3 x {round(FRONT_T + BOSS_H + PCB_T + 6)} mm self-tapping")
    print(f"Assembled thickness: {TOTAL_T:.1f} mm   body: {OUT_W:.1f} x {OUT_H:.1f} mm")
    print(f"Space under the board for battery + boards: ~{IN_W - 14:.0f} x {IN_H:.0f} x {BATTERY_SPACE:.0f} mm")
