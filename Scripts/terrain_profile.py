# [2026-11-04 T-122] Single source of truth for the world heightfield.
#
# Why this file exists: the closed-form terrain function used to live in
# three copy-pasted replicas (create_exterior.py, create_npcs.py,
# create_burnables.py) plus a C++ twin (TerrainCommandlet.cpp), kept in
# sync by a comment rule (T-050) that already drifted once — the valley
# press (T-122) landed in only one of the three Python copies. Four
# copies of load-bearing constants is a matter of when, not if. Every
# map script imports from here now; the C++ commandlet cannot import
# Python, so it stays a documented twin (its header names this file,
# and the T-122 close ritual numerically diffed the two at every
# settle() call site: max pairwise diff 0.0).
#
# MIRROR RULE (T-050, still standing): change this function and change
# TerrainCommandlet.cpp TerrainHeightCm identically, or buildings float
# or bury. The landscape is a bilinear surface over the same samples,
# so scripts sink buildings 60 cm to absorb interpolation.
import math


def _smoothstep(edge0, edge1, x):
    # Hermite blend: 0 at edge0, 1 at edge1. Matches the C++ SmoothStep.
    t = max(0.0, min(1.0, (x - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def terrain_height(x, y):
    # [2026-11-04 T-122] The world height function in cm, closed form.
    # Rolling fields + distance-ridged ranges + the lair massif
    # (46 m peak south-east of the peak-shaft exit: the mountain
    # backdrop the dragon lives in), pressed to -250 cm under the
    # lair chamber chain AND the T-017 flight-zone boxes.
    # Valley press bounds are load-bearing (proven numerically in the
    # T-122 close: zero box-top excesses on a 250 cm grid over the
    # whole flight zone): west blends 23000->27000 so Terrace1's south
    # edge (massif north lobe hits 43 m raw at (25000, -8000)) and the
    # valley floor's west edge (x=27000) both sit on fully pressed
    # ground; east blends back 66000->90000 so the valley exits through
    # a pass, not a canyon; |y| blends 21000->26000. Combined with
    # max() so either mask fully presses and shoulders never stack
    # past 1. Lair zone x<23000 is corridor-only, bit-identical to the
    # pre-T-122 function.
    fields = 350.0 * math.sin(x * 2.1e-5 + 1.7) * math.cos(y * 1.7e-5) \
        + 250.0 * math.sin((x + y) * 1.3e-5)
    dist = math.hypot(x - 10000.0, y)
    range_mask = _smoothstep(70000.0, 150000.0, dist)
    ridge = (1.0 - abs(math.sin(x * 4.5e-5 + 2.0 * math.sin(y * 3.0e-5)))) ** 3
    ranges = range_mask * (900.0 + 3400.0 * ridge)
    massif = 4600.0 * math.exp(-((x - 19000.0) ** 2 + (y + 10000.0) ** 2)
                               / (2.0 * 20000.0 ** 2))
    h = fields + ranges + massif
    corridor = (1.0 - _smoothstep(21000.0, 26000.0, x)) \
        * (1.0 - _smoothstep(7000.0, 10000.0, abs(y)))
    valley_x = _smoothstep(23000.0, 27000.0, x) \
        * (1.0 - _smoothstep(66000.0, 90000.0, x))
    valley_y = 1.0 - _smoothstep(21000.0, 26000.0, abs(y))
    press = max(corridor, valley_x * valley_y)
    h = h * (1.0 - press) + (-250.0) * press
    return max(-800.0, min(5200.0, h))


# Shipped sun (create_sky.py: PeakSun pitch -52, yaw -35). The light
# travels down-forward; toward-sun on the ground is (-X, +Y), unit:
_SUN_TX, _SUN_TY = -0.819, 0.574
# Sun elevation above the horizon, degrees. Settlement rock: no terrain
# may subtend more than this from any building pad (checked with
# margin: the verifier asserts < 50).
SUN_ELEVATION_DEG = 52.0


def sun_horizon_deg(x, y, reach=200000.0, step=1000.0):
    # [2026-11-04 T-122] Highest terrain elevation angle seen from
    # (x, y) looking toward the sun, over `reach` cm in `step`
    # increments. Below SUN_ELEVATION_DEG means the shipped sun
    # reaches the pad (no terrain self-shadow). March starts 10 m out
    # so the pad's own ground never votes.
    h0 = terrain_height(x, y)
    worst = -90.0
    d = 1000.0
    while d <= reach:
        px, py = x + _SUN_TX * d, y + _SUN_TY * d
        if abs(px) > 252000.0 or abs(py) > 252000.0:
            break
        worst = max(worst, math.degrees(math.atan2(terrain_height(px, py) - h0, d)))
        d += step
    return worst


def grade(x, y, radius=2000.0):
    # [2026-11-04 T-122] Max heightfield grade over `radius` cm in the
    # four cardinals. Grounding rule of thumb: settle() sinks 60 cm,
    # so grade * footprint should stay well under a metre (0.1 over a
    # 6 m house wall = 60 cm: the sink absorbs it, barely).
    h0 = terrain_height(x, y)
    return max(abs(terrain_height(x + radius, y) - h0) / radius,
               abs(terrain_height(x - radius, y) - h0) / radius,
               abs(terrain_height(x, y + radius) - h0) / radius,
               abs(terrain_height(x, y - radius) - h0) / radius)
