# [2026-11-03 T-066] Build Content/Maps/Lair/Lair.umap headlessly: the
# mountain, not rooms. Great chamber 100x100m x 50m tall at (9500, 0)
# with a 25m crown hole to the sky, grand east portal (50m tunnel to an
# exterior apron), west dragon cave (50m), two thief-only 80cm cracks
# (50m each, exiting past the mountain foot), hoard mound offset from a
# clear touchdown disc, and a 3-ring stepped rock shell the tunnels and
# cracks pierce through full-height gaps. Replaces the T-012 linear
# chain (entry->chamber 2->chamber 3->hoard->7x7m shaft). Deterministic
# (seeded rng); re-runnable: only our own Lair.umap is deleted on disk
# before the run (a stale headless asset registry makes in-editor
# delete/create collide, so materials use collision-free names and the
# level file is removed up front — see the T-066 goose-deuce entry).
# Capsule facts: dragon default r=42/hh=96 (84cm wide); thieves r=30/hh=88
# (60cm wide) per the T-066 derivation. Design turn 16m (T-075 worst
# measured); the chamber holds 6+ turns; height 50m = 40m dive column +
# flare. Touchdown disc: 12m radius, provably furniture-free (T-075: the
# course wedged twice on centre furniture).
import random
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
MAT_DIR = "/Game/Maps/Lair"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    # [2026-10-28 T-037] Unreal editor helpers often report failure by
    # returning False/None instead of raising. Convert those results
    # at the call site so a generator cannot print success after a
    # failed delete, create, load, or save.
    if not value:
        raise RuntimeError("create_lair failed: %s" % action)
    return value


# Preflight the shared engine meshes before deleting/recreating any
# project assets; a broken engine install must not cost the old map.
CUBE = require(unreal.load_asset("/Engine/BasicShapes/Cube"), "load Cube mesh")
SPHERE = require(unreal.load_asset("/Engine/BasicShapes/Sphere"), "load Sphere mesh")

# --- materials (fresh names: the headless asset registry cannot see
# on-disk assets, so delete/recreate of the old names collides; new
# names create cleanly. Same params as the T-012 originals) ---
def make_material(name, color, roughness, metallic):
    path = MAT_DIR + "/" + name
    # [2026-11-03 T-066] Load-first like create_thieves.py: headless
    # does_asset_exist is blind and delete/create collides with the
    # stale package, so reuse the on-disk asset when it loads. Params
    # are deterministic, so reuse == recreate.
    existing = eal.load_asset(path)
    if existing is not None:
        return existing
    mat = require(
        asset_tools.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew()),
        "create material %s" % path,
    )
    c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -100)
    c.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 100)
    r.set_editor_property("r", roughness)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    m = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 250)
    m.set_editor_property("r", metallic)
    mel.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(mat)
    require(eal.save_asset(path), "save material %s" % path)
    return mat

M_ROCK = make_material("M_MountainRock", (0.08, 0.07, 0.06), 0.95, 0.0)
M_GOLD = make_material("M_MountainGold", (1.0, 0.72, 0.25), 0.35, 1.0)
M_WOOD = make_material("M_MountainWood", (0.25, 0.16, 0.06), 0.85, 0.0)

# --- open the map: load the existing one if present (fresh or stale),
# else create new. Never trusts does_asset_exist (the headless registry
# lies both ways); existence is decided by trying the load, exactly
# like verify_lair.py does. A wipe of OUR actor labels follows so every
# run is deterministic without deleting the file first.
_LAID_OUT_PREFIXES = ("Chamber_", "Ceil", "CrownLip", "Wall", "Portal_", "Apron",
                      "CaveW", "CaveWFrame", "CrackN_", "CrackS_", "SlopeN", "SlopeS",
                      "SlopeE", "SlopeW", "HoardMound", "GoldPile", "GoldIngot",
                      "Crate", "RockProp", "Light_", "PeakSun", "PeakSkyLight",
                      "DragonStart")
_LOADED = False
try:
    if les.load_level(MAP_PATH):
        # Verify the load really landed: the headless registry reports
        # success for missing maps too, so check the editor world's path
        # (the create_thieves.py proven pattern) before trusting it.
        _world = ues.get_editor_world()
        if _world is not None and _world.get_path_name().startswith(MAP_PATH + "."):
            _LOADED = True
except Exception:
    _LOADED = False
if _LOADED:
    for _a in list(eas.get_all_level_actors()):
        _cls = _a.get_class().get_name()
        if _cls in ("PointLight", "DirectionalLight", "SkyLight"):
            require(eas.destroy_actor(_a), "clear prior light %s" % _a.get_actor_label())
        elif _cls == "StaticMeshActor" and _a.get_actor_label().startswith(_LAID_OUT_PREFIXES):
            require(eas.destroy_actor(_a), "clear prior mesh %s" % _a.get_actor_label())
        elif _cls == "PlayerStart" and _a.get_actor_label() == "DragonStart":
            require(eas.destroy_actor(_a), "clear prior player start")
else:
    require(les.new_level(MAP_PATH), "create level %s" % MAP_PATH)

rng = random.Random(12)
COUNTER = [0]


def box(prefix, mesh, center, size, mat, yaw=0.0):
    COUNTER[0] += 1
    actor = require(
        eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center)),
        "spawn static mesh actor %s" % prefix,
    )
    actor.set_actor_label("%s_%03d" % (prefix, COUNTER[0]))
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh)
    if mat is not None:
        comp.set_material(0, mat)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    if yaw:
        actor.set_actor_rotation(unreal.Rotator(0.0, yaw, 0.0), False)
    return actor


WALL_T = 200.0
CHH = 5000.0       # chamber clear height: 50 m dive column + flare
CX = 9500.0        # chamber centre x (y centre 0)
CH = 5000.0        # chamber half-size: 100x100 m floor
HOARD = (12500.0, 0.0)  # hoard mound: chamber centre +30 m X

# --- great chamber: floor, walls with gaps, ceiling with crown hole ---
ext = 2 * CH + 2 * WALL_T
box("Chamber_Floor", CUBE, (CX, 0, -100), (ext, ext, 200), M_ROCK)
hole = 2500.0      # 25 m crown hole above the touchdown disc
strip = (ext - hole) / 2.0
off = hole / 2.0 + strip / 2.0
box("CeilN", CUBE, (CX, off, CHH + 100), (ext, strip, 200), M_ROCK)
box("CeilS", CUBE, (CX, -off, CHH + 100), (ext, strip, 200), M_ROCK)
box("CeilE", CUBE, (CX + off, 0, CHH + 100), (strip, hole, 200), M_ROCK)
box("CeilW", CUBE, (CX - off, 0, CHH + 100), (strip, hole, 200), M_ROCK)
lip = hole / 2.0 + 300
box("CrownLipN", CUBE, (CX, lip, CHH + 250), (hole + 2400, 600, 100), M_ROCK)
box("CrownLipS", CUBE, (CX, -lip, CHH + 250), (hole + 2400, 600, 100), M_ROCK)
box("CrownLipE", CUBE, (CX + lip, 0, CHH + 250), (600, hole, 100), M_ROCK)
box("CrownLipW", CUBE, (CX - lip, 0, CHH + 250), (600, hole, 100), M_ROCK)
# West wall (x=4400): dragon-cave gap 500 wide x 600 tall at y=0.
for s in (-1, 1):
    box("WallW_Side", CUBE, (CX - CH - 100, s * (250 + 2375), 2500), (WALL_T, 4750, 5000), M_ROCK)
box("WallW_Lintel", CUBE, (CX - CH - 100, 0, (600 + 5000) / 2.0), (WALL_T, 500, 4400), M_ROCK)
# East wall (x=14600): grand portal gap 800x800 at y=0.
for s in (-1, 1):
    box("WallE_Side", CUBE, (CX + CH + 100, s * (400 + 2300), 2500), (WALL_T, 4600, 5000), M_ROCK)
box("WallE_Lintel", CUBE, (CX + CH + 100, 0, (800 + 5000) / 2.0), (WALL_T, 800, 4200), M_ROCK)
# North wall (y=5100): thief crack gap 80x220 at x=7500 (gap 7460..7540).
box("WallN_W", CUBE, (5880, CH + 100, 2500), (3160, WALL_T, 5000), M_ROCK)
box("WallN_E", CUBE, (11120, CH + 100, 2500), (7160, WALL_T, 5000), M_ROCK)
box("WallN_Lintel", CUBE, (7500, CH + 100, (220 + 5000) / 2.0), (80, WALL_T, 4780), M_ROCK)
# South wall (y=-5100): thief crack gap 80x220 at x=11500 (gap 11460..11540).
box("WallS_W", CUBE, (7880, -(CH + 100), 2500), (7160, WALL_T, 5000), M_ROCK)
box("WallS_E", CUBE, (13120, -(CH + 100), 2500), (3160, WALL_T, 5000), M_ROCK)
box("WallS_Lintel", CUBE, (11500, -(CH + 100), (220 + 5000) / 2.0), (80, WALL_T, 4780), M_ROCK)

# --- mountain shell: 3 stepped rock rings around the chamber; every
# tunnel/crack pierces every ring it crosses through a full-height gap
# (side segments + a top slab above the passage) so no passage is ever
# sealed. Ring tops 5100/3400/1700 over a -250 base.
def ring_side_segments(prefix, fixed, c0, c1, gap_c, gap_hw, thick, bottom, top_z, gap_top, mat, along_x):
    # Builds [seg_lo, seg_hi, top slab] for one side. (fixed, c)
    # coordinates: N/S sides vary x at fixed y; E/W sides vary y at fixed x.
    if along_x:
        lo0, lo1 = c0, gap_c - gap_hw
        hi0, hi1 = gap_c + gap_hw, c1
    else:
        lo0, lo1 = c0, gap_c - gap_hw
        hi0, hi1 = gap_c + gap_hw, c1
    if along_x:
        box(prefix + "_Lo", CUBE, ((lo0 + lo1) / 2.0, fixed, (bottom + top_z) / 2.0),
            (lo1 - lo0, thick, top_z - bottom), mat)
        box(prefix + "_Hi", CUBE, ((hi0 + hi1) / 2.0, fixed, (bottom + top_z) / 2.0),
            (hi1 - hi0, thick, top_z - bottom), mat)
        if gap_top < top_z:
            box(prefix + "_Top", CUBE, (gap_c, fixed, (gap_top + top_z) / 2.0),
                (2 * gap_hw, thick, top_z - gap_top), mat)
    else:
        box(prefix + "_Lo", CUBE, (fixed, (lo0 + lo1) / 2.0, (bottom + top_z) / 2.0),
            (thick, lo1 - lo0, top_z - bottom), mat)
        box(prefix + "_Hi", CUBE, (fixed, (hi0 + hi1) / 2.0, (bottom + top_z) / 2.0),
            (thick, hi1 - hi0, top_z - bottom), mat)
        if gap_top < top_z:
            box(prefix + "_Top", CUBE, (fixed, gap_c, (gap_top + top_z) / 2.0),
                (thick, 2 * gap_hw, top_z - gap_top), mat)


for s in range(3):
    top = 5100.0 - s * 1700.0
    bottom = -250.0
    half = 5300.0 + s * 1500.0 + 750.0
    span = 2 * (half + 750.0)
    c0x, c1x = CX - span / 2.0, CX + span / 2.0
    c0y, c1y = -span / 2.0, span / 2.0
    # N side (y=+half): north crack gap at x=7500, roof z=320.
    ring_side_segments("SlopeN%d" % s, half, c0x, c1x, 7500.0, 260.0, 1500.0, bottom, top, 320.0, M_ROCK, True)
    # S side (y=-half): south crack gap at x=11500.
    ring_side_segments("SlopeS%d" % s, -half, c0x, c1x, 11500.0, 260.0, 1500.0, bottom, top, 320.0, M_ROCK, True)
    # E side (x=CX+half): grand portal gap at y=0, roof z=1000.
    ring_side_segments("SlopeE%d" % s, CX + half, c0y, c1y, 0.0, 600.0, 1500.0, bottom, top, 1000.0, M_ROCK, False)
    # W side (x=CX-half): west cave gap at y=0, roof z=800.
    ring_side_segments("SlopeW%d" % s, CX - half, c0y, c1y, 0.0, 500.0, 1500.0, bottom, top, 800.0, M_ROCK, False)

# --- grand east portal: 800x800 tunnel x=14500 -> x=19500, then apron ---
box("Portal_Floor", CUBE, (17000, 0, -100), (5400, 1200, 200), M_ROCK)
box("Portal_Ceil", CUBE, (17000, 0, 900), (5400, 1200, 200), M_ROCK)
box("Portal_N", CUBE, (17000, 500, 400), (5400, WALL_T, 800), M_ROCK)
box("Portal_S", CUBE, (17000, -500, 400), (5400, WALL_T, 800), M_ROCK)
box("PortalFrame_N", CUBE, (19500, 500, 400), (WALL_T, WALL_T, 800), M_ROCK)
box("PortalFrame_S", CUBE, (19500, -500, 400), (WALL_T, WALL_T, 800), M_ROCK)
box("PortalFrame_Top", CUBE, (19500, 0, 900), (WALL_T, 1200, 200), M_ROCK)
box("Apron", CUBE, (21000, 0, -100), (3000, 1000, 200), M_ROCK)

# --- west dragon cave: 500 wide x 600 tall, x=4500 -> x=-500 ---
box("CaveW_Floor", CUBE, (2000, 0, -100), (5200, 900, 200), M_ROCK)
box("CaveW_Ceil", CUBE, (2000, 0, 700), (5200, 900, 200), M_ROCK)
box("CaveW_N", CUBE, (2000, 350, 300), (5200, WALL_T, 600), M_ROCK)
box("CaveW_S", CUBE, (2000, -350, 300), (5200, WALL_T, 600), M_ROCK)
box("CaveWFrame_N", CUBE, (-500, 350, 300), (WALL_T, WALL_T, 600), M_ROCK)
box("CaveWFrame_S", CUBE, (-500, -350, 300), (WALL_T, WALL_T, 600), M_ROCK)
box("CaveWFrame_Top", CUBE, (-500, 0, 700), (WALL_T, 900, 200), M_ROCK)

# --- thief cracks: 80 wide x 220 tall (dragon r=42 blocked, thief r=30 fits) ---
# North crack at x=7500: y=5000 (wall) -> y=10000 (past the mountain foot).
box("CrackN_Floor", CUBE, (7500, 7500, -100), (500, 5400, 200), M_ROCK)
box("CrackN_Ceil", CUBE, (7500, 7500, 270), (500, 5400, 100), M_ROCK)
box("CrackN_W", CUBE, (7500 - 140, 7500, 110), (WALL_T, 5000, 220), M_ROCK)
box("CrackN_E", CUBE, (7500 + 140, 7500, 110), (WALL_T, 5000, 220), M_ROCK)
# South crack at x=11500: y=-5000 -> y=-10000.
box("CrackS_Floor", CUBE, (11500, -7500, -100), (500, 5400, 200), M_ROCK)
box("CrackS_Ceil", CUBE, (11500, -7500, 270), (500, 5400, 100), M_ROCK)
box("CrackS_W", CUBE, (11500 - 140, -7500, 110), (WALL_T, 5000, 220), M_ROCK)
box("CrackS_E", CUBE, (11500 + 140, -7500, 110), (WALL_T, 5000, 220), M_ROCK)

# --- hoard mound at (12500, 0): low and wide so cruise lines pass over
# it (top z=300); the touchdown disc stays clear by distance, not height.
box("HoardMound", SPHERE, (HOARD[0], HOARD[1], 100), (2400, 2400, 400), M_GOLD)


def point_light(name, loc, color=(1.0, 0.62, 0.32), intensity=6000.0, radius=2800.0):
    a = require(eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*loc)), "spawn point light %s" % name)
    a.set_actor_label(name)
    comp = a.get_component_by_class(unreal.PointLightComponent)
    comp.set_intensity(intensity)
    comp.set_light_color(unreal.LinearColor(*color, 1.0))
    comp.set_editor_property("attenuation_radius", radius)
    return a


def gold_pile(x, y, scale=1.0):
    for _ in range(rng.randint(4, 7)):
        r = rng.uniform(120, 260) * scale
        ox, oy = rng.uniform(-220, 220) * scale, rng.uniform(-220, 220) * scale
        box("GoldPile", SPHERE, (x + ox, y + oy, r * 0.25), (r * 2, r * 2, r * 0.8), M_GOLD, rng.uniform(0, 180))
    for _ in range(rng.randint(2, 4)):
        s = rng.uniform(60, 120) * scale
        box("GoldIngot", CUBE, (x + rng.uniform(-250, 250) * scale, y + rng.uniform(-250, 250) * scale, s / 2),
            (s * 1.6, s, s * 0.6), M_GOLD, rng.uniform(0, 180))


def clear_of_disc(x, y):
    # Touchdown disc: 12 m radius around the chamber centre stays empty
    # above the floor (T-075: the course wedged twice on centre furniture).
    return ((x - CX) ** 2 + y ** 2) ** 0.5 >= 1200.0


_RADIALS = [  # chamber centre to each mouth: keep travel lines clear
    ((CX, 0.0), (14500.0, 0.0)),     # east portal
    ((CX, 0.0), (4500.0, 0.0)),       # west cave
    ((CX, 0.0), (7500.0, 5000.0)),    # north crack
    ((CX, 0.0), (11500.0, -5000.0)),  # south crack
]


def _dist_to_seg(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    denom = dx * dx + dy * dy
    t = 0.0 if denom < 1e-9 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / denom))
    cx, cy = ax + t * dx, ay + t * dy
    return ((px - cx) ** 2 + (py - cy) ** 2) ** 0.5


def clear_of_paths(x, y):
    # Props are decor: they must never force a detour on a travel
    # radial (walls are what the traversal traces are allowed to hit).
    for (ax, ay), (bx, by) in _RADIALS:
        if _dist_to_seg(x, y, ax, ay, bx, by) < 900.0:
            return False
    return True


def scatter_rocks(count):
    for _ in range(count):
        # Near walls only, clear of the touchdown disc, the hoard mound
        # and every travel radial (mouths included).
        y = rng.uniform(-1, 1) * rng.uniform(CH - 1300, CH - 300)
        x = CX + rng.uniform(-CH + 500, CH - 500)
        if not clear_of_disc(x, y):
            continue
        if abs(x - HOARD[0]) < 1800 and abs(y - HOARD[1]) < 1800:
            continue
        if not clear_of_paths(x, y):
            continue
        s = rng.uniform(150, 480)
        if rng.random() < 0.5:
            box("RockProp", CUBE, (x, y, s * 0.3), (s, s * rng.uniform(0.6, 1.2), s * 0.7), M_ROCK, rng.uniform(0, 180))
        else:
            box("RockProp", SPHERE, (x, y, s * 0.15), (s, s * 0.8, s * 0.6), M_ROCK, rng.uniform(0, 180))


def crates():
    for sx, sy in ((-1, -1), (1, 1)):
        x, y = CX + sx * (CH - 700), sy * (CH - 700)
        if not clear_of_disc(x, y) or not clear_of_paths(x, y):
            continue
        box("Crate", CUBE, (x, y, 60), (120, 120, 120), M_WOOD, rng.uniform(0, 45))
        box("Crate", CUBE, (x + 40, y - 30, 180), (110, 110, 110), M_WOOD, rng.uniform(0, 45))


# --- props: hoard dressing + wall rocks (disc + mouths stay clear) ---
gold_pile(HOARD[0] - 1500, 1300, 1.2)
gold_pile(HOARD[0] + 1500, -1300, 1.2)
gold_pile(HOARD[0], 1700, 1.4)
gold_pile(HOARD[0], -1700, 1.0)
gold_pile(CX - 3000, 2500, 0.8)
scatter_rocks(24)
crates()

# --- player start west of the touchdown disc + lights ---
start = require(eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(CX - 3000, 0, 150)), "spawn PlayerStart")
start.set_actor_label("DragonStart")

point_light("Light_ChamberNW", (CX - 2500, 2500, 4000))
point_light("Light_ChamberNE", (CX + 2500, 2500, 4000))
point_light("Light_ChamberSW", (CX - 2500, -2500, 4000))
point_light("Light_ChamberSE", (CX + 2500, -2500, 4000))
point_light("Light_HoardMain", (HOARD[0], HOARD[1], 4200), intensity=9000.0, radius=3600.0)
point_light("Light_HoardN", (HOARD[0], 1500, 2500), intensity=4000.0, radius=2200.0)
point_light("Light_HoardS", (HOARD[0], -1500, 2500), intensity=4000.0, radius=2200.0)
point_light("Light_Portal", (17000, 0, 500), intensity=2500.0, radius=1800.0)
point_light("Light_PortalExit", (19500, 0, 500), intensity=2500.0, radius=1800.0)
point_light("Light_CaveW", (2000, 0, 400), intensity=2500.0, radius=1800.0)
point_light("Light_CrackN", (7500, 7500, 400), intensity=1500.0, radius=1200.0)
point_light("Light_CrackS", (11500, -7500, 400), intensity=1500.0, radius=1200.0)
point_light("Light_CrownGlow", (CX, 0, 4500), color=(0.75, 0.85, 1.0), intensity=5000.0, radius=3000.0)

sun = require(
    eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(CX + 600, 400, 7500)),
    "spawn directional light",
)
sun.set_actor_label("PeakSun")
sun.set_actor_rotation(unreal.Rotator(-82.0, -30.0, 0.0), False)
sun.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(3.0)

sky = require(eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(CX, 0, 6200)), "spawn sky light")
sky.set_actor_label("PeakSkyLight")

require(les.save_current_level(), "save level %s" % MAP_PATH)
require(eal.save_directory(MAT_DIR), "save materials in %s" % MAT_DIR)
print("LAIR_CREATED", MAP_PATH, "static_mesh_actors=", COUNTER[0])
