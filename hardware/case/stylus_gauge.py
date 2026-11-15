#!/usr/bin/env python3
"""Stylus hole tester: a small bar with holes from 4.0 to 7.0 mm (labels engraved).
Push your stylus into each hole; the snuggest one that still slides is your size.
Then set STYLUS_D in charmbot_case.py to that number (the case adds its own fit clearance)."""
import os
import cadquery as cq

SIZES = [4.0, 4.5, 5.0, 5.5, 6.0, 6.5, 7.0]
PITCH, T = 11.0, 6.0
bar = cq.Workplane("XY").box(PITCH * len(SIZES) + 6, 20, T, centered=(True, True, False)).edges("|Z").fillet(3)
x0 = -PITCH * (len(SIZES) - 1) / 2
for i, d in enumerate(SIZES):
    x = x0 + i * PITCH
    bar = bar.cut(cq.Workplane("XY").center(x, 3).circle((d + 0.35) / 2).extrude(T + 2).translate((0, 0, -1)))
    label = cq.Workplane("XY").text(f"{d:g}", 4.2, 0.6, kind="bold").translate((x, -5.5, T - 0.6))
    bar = bar.cut(label)
here = os.path.dirname(os.path.abspath(__file__))
cq.exporters.export(bar, os.path.join(here, "stl", "stylus_gauge.stl"), tolerance=0.02)
print("wrote stl/stylus_gauge.stl")
