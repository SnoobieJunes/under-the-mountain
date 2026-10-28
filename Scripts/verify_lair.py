# [2026-10-28 T-012] Verify Lair.umap: fresh load, actor census, traversal traces.
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)

ok = les.load_level(MAP_PATH)
print("LAIR_LOAD_OK", ok)

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

world = ues.get_editor_world()
ksl = unreal.KismetSystemLibrary
CHAN = unreal.TraceTypeQuery.TRACE_TYPE_QUERY1  # Visibility
NONE = unreal.DrawDebugTrace.NONE

def unpack(res):
    # UE python: (return_value, OutHit) tuple, defensively handle bare HitResult.
    if isinstance(res, tuple):
        return bool(res[0]), (res[1] if len(res) > 1 else None)
    return False, res

def line(name, a, b, expect_hit):
    res = ksl.line_trace_single(world, unreal.Vector(*a), unreal.Vector(*b), CHAN, False, [], NONE, True)
    hit, hr = unpack(res)
    who = ""
    if hit and hr is not None:
        try:
            who = " hit_actor=%s at=%s" % (hr.get_editor_property("hit_actor"), hr.get_editor_property("location"))
        except Exception as e:
            who = " hit_actor=? (%s)" % e
    verdict = "PASS" if hit == expect_hit else "FAIL"
    print("LAIR_TRACE line %-28s %s -> %s hit=%s expect_hit=%s %s%s" % (name, a, b, hit, expect_hit, verdict, who))
    return hit == expect_hit

def sphere(name, a, b, radius):
    res = ksl.sphere_trace_single(world, unreal.Vector(*a), unreal.Vector(*b), radius, CHAN, False, [], NONE, True)
    hit, hr = unpack(res)
    who = ""
    if hit and hr is not None:
        try:
            who = " hit_actor=%s at=%s" % (hr.get_editor_property("hit_actor"), hr.get_editor_property("location"))
        except Exception as e:
            who = " hit_actor=? (%s)" % e
    verdict = "PASS" if not hit else "FAIL"
    print("LAIR_TRACE sphere(r=%s) %-22s %s -> %s hit=%s %s%s" % (radius, name, a, b, hit, verdict, who))
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

print("LAIR_TRACES_SUMMARY passed=%d/%d" % (sum(1 for r in results if r), len(results)))
