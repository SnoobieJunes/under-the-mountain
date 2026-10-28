# [2026-10-28 T-012] Verify Lair.umap: fresh load, actor census, traversal traces.
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

ok = les.load_level(MAP_PATH)
print("LAIR_LOAD_OK", ok)
if not ok:
    # [2026-10-28 T-037] A verifier that only prints its failure lets
    # automation treat a missing/broken map as success. Abort before
    # the census/traces can produce misleading partial results.
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
# SystemLibrary trace variant (channel/profile/objects) returns None in the
# headless editor world even straight into a BlockAll cube floor — there is
# no physics scene in a commandlet. So traversal is verified geometrically:
# segment-vs-AABB against each StaticMeshActor's world bounds (conservative
# for yaw-rotated props, since world AABB contains the mesh). The controls
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

C1, C2, C3, HOARD = 0.0, 6000.0, 12000.0, 19000.0
results = []
# Controls first: prove the trace system is live in this editor world.
results.append(line("CTRL_down_into_floor", (C1, 0, 150), (C1, 0, -200), True))
results.append(line("CTRL_south_into_wall", (C1, 0, 150), (C1, -2500, 150), True))
results.append(line("CTRL_up_into_ceiling", (C2, 0, 150), (C2, 0, 2500), True))
# Traversal at capsule-centre height (z=150) and flight height (z=450, tunnels 600 tall).
for z in (150.0, 450.0):
    results.append(line("Entry->Chamber2 z=%g" % z, (C1, 0, z), (C2, 0, z), False))
    results.append(line("Chamber2->Chamber3 z=%g" % z, (C2, 0, z), (C3, 0, z), False))
    results.append(line("Chamber3->Hoard z=%g" % z, (C3, 0, z), (HOARD, 0, z), False))
results.append(line("Hoard->up_the_shaft", (HOARD, 0, 150), (HOARD, 0, 5600), False))
# Swept volume: radius-60 sphere (capsule radius is 42; 60 adds margin).
results.append(sphere("Entry->Chamber2", (C1, 0, 200), (C2, 0, 200), 60.0))
results.append(sphere("Chamber2->Chamber3", (C2, 0, 200), (C3, 0, 200), 60.0))
results.append(sphere("Chamber3->Hoard", (C3, 0, 200), (HOARD, 0, 200), 60.0))
results.append(sphere("Hoard->up_the_shaft", (HOARD, 0, 200), (HOARD, 0, 5600), 60.0))

passed = sum(1 for r in results if r)
print("LAIR_TRACES_SUMMARY passed=%d/%d" % (passed, len(results)))
if passed != len(results):
    # [2026-10-28 T-037] Make the process result match the printed
    # verdict; callers must not need to scrape PASS/FAIL text.
    print("LAIR_VERIFY_FAILED %d trace(s) failed" % (len(results) - passed))
    raise SystemExit(1)
