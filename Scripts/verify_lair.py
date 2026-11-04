# [2026-11-03 T-066] Verify Lair.umap: fresh load, actor census, traversal
# traces for the mountain topology (great chamber + crown + east portal +
# west cave + 2 thief cracks). Same geometric method as T-012 (physics
# traces return None headless): segment-vs-AABB plus swept-sphere pads.
# The dragon/thief distinction is proven both ways: a 30 cm sphere passes
# the cracks (thief capsule r=30) while a 42 cm sphere does NOT (dragon
# capsule r=42) — the T-066 "provably dragon-impassable" requirement.
import unreal

# [2026-11-03 T-066] Headless runs swallow print(): mirror every line below
# to Saved/LairVerify/verify.log so the trace verdicts survive the
# session. Set up defensively — the verifier must never fail because
# the log file could not be written.
import os as _os
import sys as _sys


class _Tee(object):
    def __init__(self, *files):
        self._files = files

    def write(self, s):
        for f in self._files:
            try:
                f.write(s)
            except Exception:
                pass

    def flush(self):
        for f in self._files:
            try:
                f.flush()
            except Exception:
                pass


try:
    _logdir = _os.path.join(unreal.Paths.project_saved_dir(), "LairVerify")
    _os.makedirs(_logdir, exist_ok=True)
    _sys.stdout = _Tee(_sys.stdout, open(_os.path.join(_logdir, "verify.log"), "w"))
except Exception:
    pass

MAP_PATH = "/Game/Maps/Lair/Lair"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

ok = les.load_level(MAP_PATH)
print("LAIR_LOAD_OK", ok)
if not ok:
    # [2026-10-28 T-037] A verifier that only prints its failure treats
    # automation treat a missing/broken map as success. Abort before the
    # census/traces can produce misleading partial results.
    print("LAIR_VERIFY_FAILED load_level returned False for", MAP_PATH)
    raise SystemExit(1)

actors = eas.get_all_level_actors()
counts = {}
mats = {}
for a in actors:
    cls = a.get_class().get_name()
    counts[cls] = counts.get(cls, 0) + 1
    if cls == "StaticMeshActor":
        m = a.static_mesh_component.get_material(0)
        mn = m.get_name() if m else "<none>"
        mats[mn] = mats.get(mn, 0) + 1
print("LAIR_ACTOR_TOTAL", len(actors))
for k in sorted(counts):
    print("LAIR_ACTOR_CLASS", k, counts[k])
for k in sorted(mats):
    print("LAIR_MAT", k, mats[k])

# [2026-10-28 T-012] Physics traces are unusable here: probed live, every
# SystemLibrary trace variant returns None in the headless editor world
# even into a BlockAll cube floor — there is no physics scene in a
# commandlet. So traversal is verified geometrically: segment-vs-AABB
# against each StaticMeshActor's world bounds (conservative for
# yaw-rotated props, since world AABB contains the mesh). The controls
# below prove the geometry test itself can register hits.
BOXES = []  # (label, min_xyz, max_xyz)
for a in actors:
    if a.get_class().get_name() != "StaticMeshActor":
        continue
    origin, extent = a.get_actor_bounds(False)
    lo = (origin.x - extent.x, origin.y - extent.y, origin.z - extent.z)
    hi = (origin.x + extent.x, origin.y + extent.y, origin.z + extent.z)
    BOXES.append((a.get_actor_label(), lo, hi))
print("LAIR_BOXES", len(BOXES))

def seg_hits_box(a, b, lo, hi, pad=0.0):
    # Slab test: parametric segment a->b against the (padded) AABB.
    tmin, tmax = 0.0, 1.0
    for i in range(3):
        d = b[i] - a[i]
        mn, mx = lo[i] - pad, hi[i] + pad
        if abs(d) < 1e-9:
            if a[i] < mn or a[i] > mx:
                return False
            continue
        t1, t2 = (mn - a[i]) / d, (mx - a[i]) / d
        if t1 > t2:
            t1, t2 = t2, t1
        tmin, tmax = max(tmin, t1), min(tmax, t2)
        if tmin > tmax:
            return False
    return True

def first_hit(a, b, pad=0.0):
    for label, lo, hi in BOXES:
        if seg_hits_box(a, b, lo, hi, pad):
            return label
    return None

def line(name, a, b, expect_hit):
    who = first_hit(a, b)
    hit = who is not None
    verdict = "PASS" if hit == expect_hit else "FAIL"
    print("LAIR_TRACE line %-28s %s -> %s hit=%s expect_hit=%s %s%s" % (name, a, b, hit, expect_hit, verdict, (" hit_actor=" + who) if who else ""))
    return hit == expect_hit

def sphere(name, a, b, radius):
    who = first_hit(a, b, pad=radius)
    hit = who is not None
    verdict = "PASS" if not hit else "FAIL"
    print("LAIR_TRACE sphere(r=%s) %-22s %s -> %s hit=%s %s%s" % (radius, name, a, b, hit, verdict, (" hit_actor=" + who) if who else ""))
    return not hit

def sphere_must_hit(name, a, b, radius):
    # Inverse of sphere(): proves a passage is BLOCKED for this radius
    # (the dragon-proof half of the thief-only crack contract).
    who = first_hit(a, b, pad=radius)
    hit = who is not None
    verdict = "PASS" if hit else "FAIL"
    print("LAIR_TRACE blocked(r=%s) %-21s %s -> %s hit=%s %s%s" % (radius, name, a, b, hit, verdict, (" hit_actor=" + who) if who else ""))
    return hit

# Mountain topology (create_lair.py): chamber centre (9500, 0), hoard
# (12500, 0), crown hole above the centre, grand portal east x=18500,
# west cave exit x=500, north crack x=7500 (y 5000->7000), south crack
# x=11500 (y -5000->-7000).
CX = 9500.0
HOARDX = 12500.0
results = []
# Controls first: prove the trace system is live in this editor world.
results.append(line("CTRL_down_into_floor", (CX, 0, 150), (CX, 0, -200), True))
results.append(line("CTRL_east_into_wall", (CX, 0, 2500), (CX + 6000, 0, 2500), True))
results.append(line("CTRL_up_into_ceiling", (CX + 3500, 0, 150), (CX + 3500, 0, 5500), True))
# Grand portal run: tunnel leg low (inside the 800-tall bore), full run
# at cruise height over the flattened hoard mound (top z=300); chamber
# to east mouth likewise cruises over it.
results.append(line("PortalTunnel z=150", (18700, 0, 150), (14500, 0, 150), False))
results.append(line("Portal->Chamber z=450", (18700, 0, 450), (CX, 0, 450), False))
results.append(line("Chamber->WestCave z=150", (CX, 0, 150), (500, 0, 150), False))
results.append(line("Chamber->WestCave z=450", (CX, 0, 450), (500, 0, 450), False))
# Chamber centre to every mouth: east over the mound (z=600), west level,
# crack mouths low through the 220-tall gaps (z=110).
results.append(line("Chamber->EastMouth", (CX, 0, 600), (14500.0, 0.0, 600), False))
results.append(line("Chamber->WestMouth", (CX, 0, 300), (4500.0, 0.0, 300), False))
results.append(line("Chamber->NorthMouth", (CX, 0, 110), (7500.0, 5000.0, 110), False))
results.append(line("Chamber->SouthMouth", (CX, 0, 110), (11500.0, -5000.0, 110), False))
# Swept volume: radius-60 sphere (capsule radius is 42; 60 adds margin).
results.append(sphere("PortalTunnel", (18700, 0, 150), (14500, 0, 150), 60.0))
results.append(sphere("Portal->Chamber", (18700, 0, 450), (CX, 0, 450), 60.0))
results.append(sphere("Chamber->WestCave", (CX, 0, 200), (500, 0, 200), 60.0))
# Crown climb straight up through the 25 m hole (pads: thief and dragon).
results.append(line("Chamber->up_the_crown", (CX, 0, 150), (CX, 0, 6000), False))
results.append(sphere("Chamber->up_the_crown_42", (CX, 0, 200), (CX, 0, 6000), 42.0))
# Thief cracks: 30 cm sphere passes, 42 cm (dragon) does not.
results.append(sphere("CrackN_thief_30", (7500, 4900, 110), (7500, 10100, 110), 30.0))
results.append(sphere_must_hit("CrackN_dragon_42", (7500, 4900, 110), (7500, 10100, 110), 42.0))
results.append(sphere("CrackS_thief_30", (11500, -4900, 110), (11500, -10100, 110), 30.0))
results.append(sphere_must_hit("CrackS_dragon_42", (11500, -4900, 110), (11500, -10100, 110), 42.0))
# Touchdown disc: nothing but the floor within 12 m of (CX, 0) above z=50.
disc_violations = []
for label, lo, hi in BOXES:
    if label.startswith("Chamber_Floor"):
        continue
    # Horizontal distance from the AABB to the disc centre.
    dx = 0.0
    if CX < lo[0]:
        dx = lo[0] - CX
    elif CX > hi[0]:
        dx = CX - hi[0]
    dy = 0.0
    if 0.0 < lo[1]:
        dy = lo[1] - 0.0
    elif 0.0 > hi[1]:
        dy = 0.0 - hi[1]
    if hi[2] > 50.0 and (dx * dx + dy * dy) ** 0.5 < 1200.0:
        disc_violations.append(label)
if disc_violations:
    print("LAIR_TRACE touchdown_disc VIOLATIONS:", disc_violations)
results.append(len(disc_violations) == 0)
print("LAIR_TRACE touchdown_disc clear=%s" % (len(disc_violations) == 0))

# [2026-11-04 T-122] Settlement successor checks: the map that lost its
# terrain in T-066 (2.05 MB -> 454 KB, nothing detected it) gets a
# landscape assertion, and every settlement gets grounded + sunlit
# proofs from the same closed form the scripts place with
# (Scripts/terrain_profile.py - imported, not replicated).
_sys.path.insert(0, _os.path.join(unreal.Paths.project_dir(), "Scripts"))
from terrain_profile import grade as _grade
from terrain_profile import sun_horizon_deg as _sun_horizon
from terrain_profile import terrain_height as _th

_landscapes = [a for a in actors if a.get_class().get_name() == "Landscape"]
print("LAIR_LANDSCAPE_COUNT", len(_landscapes))
results.append(len(_landscapes) == 1)
if _landscapes:
    _comps = _landscapes[0].get_components_by_class(unreal.LandscapeComponent)
    print("LAIR_LANDSCAPE_COMPONENTS", len(_comps))
    results.append(len(_comps) == 16)
else:
    print("LAIR_LANDSCAPE_COMPONENTS 0 (no landscape actor)")
    results.append(False)

# Settlement groups: label prefix -> (grounded rule, horizon limit).
# settle-placed groups (castles, VillageN/E, farms) sit at terrain-60;
# the valley group sits on T-017 boxes at fixed z, so its rule is
# terrain-below-floor instead of terrain-follow.
_SETTLE_GROUPS = ("Exterior_CastleA_", "Exterior_CastleB_",
                  "Exterior_VillageN_", "Exterior_VillageE_",
                  "Exterior_Farm", "Exterior_House", "Exterior_Inn",
                  "Exterior_Tower", "Exterior_Well", "Exterior_Stall",
                  "Exterior_Cart", "Exterior_Crates", "Exterior_Barrels",
                  "Exterior_Fence")
for _prefix in _SETTLE_GROUPS:
    _members = [(label, lo, hi) for label, lo, hi in BOXES
                if label.startswith(_prefix)]
    if not _members:
        continue
    _cx = sum((lo[0] + hi[0]) / 2.0 for _, lo, hi in _members) / len(_members)
    _cy = sum((lo[1] + hi[1]) / 2.0 for _, lo, hi in _members) / len(_members)
    _horizon = _sun_horizon(_cx, _cy)
    print("LAIR_SETTLE %s n=%d horizon=%.1fdeg" % (_prefix, len(_members), _horizon))
    results.append(_horizon < 50.0)
    _is_valley = not (_prefix.startswith("Exterior_Castle")
                      or _prefix.startswith("Exterior_Village")
                      or _prefix.startswith("Exterior_Farm"))
    _bad = []
    for _label, _lo, _hi in _members:
        _gx, _gy = (_lo[0] + _hi[0]) / 2.0, (_lo[1] + _hi[1]) / 2.0
        _ground = _th(_gx, _gy)
        if _is_valley:
            # Valley boxes carry the floor (top z=0); terrain pressed
            # to -250 must stay below every member's base.
            if _ground > _lo[2] - 50.0:
                _bad.append(_label)
        else:
            # Settle-placed: base within 1.5 m of terrain-60 (sink +
            # interpolation + gentle pad slope).
            if abs(_lo[2] - (_ground - 60.0)) > 150.0:
                _bad.append(_label)
        if _grade(_gx, _gy) > 0.45:
            _bad.append(_label + ":steep")
    if _bad:
        print("LAIR_SETTLE_GROUND %s VIOLATIONS: %s" % (_prefix, _bad))
    results.append(len(_bad) == 0)
    # [2026-11-04 T-122] Overhang audit (the actual owner-reported bug:
    # T2/T3 terrace slabs roofed 7 of 8 valley buildings — a roofed
    # cave, not a sunlit village). No StaticMesh box may sit above a
    # settlement member in plan view (the valley floor beneath is
    # below, never above, so it never votes). The heightfield cannot
    # overhang by construction.
    _over = []
    for _label, _lo, _hi in _members:
        _mcx, _mcy = (_lo[0] + _hi[0]) / 2.0, (_lo[1] + _hi[1]) / 2.0
        for _bl, _blo, _bhi in BOXES:
            if _bl == _label or _bhi[2] <= _hi[2] + 50.0:
                continue
            if _blo[0] <= _mcx <= _bhi[0] and _blo[1] <= _mcy <= _bhi[1]:
                _over.append("%s under %s" % (_label, _bl))
                break
    if _over:
        print("LAIR_SETTLE_OVERHANG %s VIOLATIONS: %s" % (_prefix, _over))
    results.append(len(_over) == 0)

passed = sum(1 for r in results if r)
print("LAIR_TRACES_SUMMARY passed=%d/%d" % (passed, len(results)))
if passed != len(results):
    # [2026-10-28 T-037] Make the process result match the printed
    # verdict; callers must not need to scrape PASS/FAIL text.
    print("LAIR_VERIFY_FAILED %d trace(s) failed" % (len(results) - passed))
    raise SystemExit(1)
