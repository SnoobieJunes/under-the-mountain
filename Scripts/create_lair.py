# [2026-10-28 T-012] Build Content/Maps/Lair/Lair.umap headlessly.
# Entry -> tunnel -> Chamber2 -> tunnel -> Chamber3 -> tunnel -> Hoard (largest)
# + vertical peak shaft out of the hoard ceiling. GreyboxLair untouched.
# Dragon capsule: ACharacter default r=42 hh=96 (no InitCapsuleSize in
# DragonCharacter.cpp) -> 800w x 600t doors and 700x700 shaft clear it easily.
import random
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
MAT_DIR = "/Game/Maps/Lair"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
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

# --- materials (recreate deterministically; only ever our own 3 assets) ---
def make_material(name, color, roughness, metallic):
    path = MAT_DIR + "/" + name
    if eal.does_asset_exist(path):
        require(eal.delete_asset(path), "delete existing material %s" % path)
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

M_ROCK = make_material("M_RockDark", (0.08, 0.07, 0.06), 0.95, 0.0)
M_GOLD = make_material("M_GoldWarm", (1.0, 0.72, 0.25), 0.35, 1.0)
M_WOOD = make_material("M_WoodDark", (0.25, 0.16, 0.09), 0.85, 0.0)

# --- fresh level (delete only our own Lair.umap if a prior run left one) ---
if eal.does_asset_exist(MAP_PATH):
    require(eal.delete_asset(MAP_PATH), "delete existing level %s" % MAP_PATH)
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
CEIL = 1200.0
DOOR_W = 800.0
DOOR_H = 600.0

def door_wall(prefix, cx, cy, half):
    # Wall plane at x=cx spanning y in [cy-span/2, cy+span/2], door gap at cy.
    span = 2 * half + 2 * WALL_T
    side_len = (span - DOOR_W) / 2.0
    for s in (-1, 1):
        y = cy + s * (DOOR_W / 2.0 + side_len / 2.0)
        box(prefix + "_Side", CUBE, (cx, y, CEIL / 2.0), (WALL_T, side_len, CEIL), M_ROCK)
    # Wood lintel above the gap.
    box(prefix + "_Lintel", CUBE, (cx, cy, (DOOR_H + CEIL) / 2.0), (WALL_T, DOOR_W, CEIL - DOOR_H), M_WOOD)

def solid_wall_x(prefix, cx, cy, half):
    box(prefix, CUBE, (cx, cy, CEIL / 2.0), (WALL_T, 2 * half + 2 * WALL_T, CEIL), M_ROCK)

def solid_wall_y(prefix, cx, cy, half):
    box(prefix, CUBE, (cx, cy, CEIL / 2.0), (2 * half + 2 * WALL_T, WALL_T, CEIL), M_ROCK)

def chamber(name, cx, half, door_west, door_east, shaft_hole=False):
    ext = 2 * half + 2 * WALL_T
    box(name + "_Floor", CUBE, (cx, 0, -100), (ext, ext, 200), M_ROCK)
    if not shaft_hole:
        box(name + "_Ceiling", CUBE, (cx, 0, CEIL + 100), (ext, ext, 200), M_ROCK)
    else:
        # 700x700 hole at centre: N/S strips full width, E/W strips between.
        hole = 700.0
        strip = (ext - hole) / 2.0
        off = hole / 2.0 + strip / 2.0
        box(name + "_CeilN", CUBE, (cx, off, CEIL + 100), (ext, strip, 200), M_ROCK)
        box(name + "_CeilS", CUBE, (cx, -off, CEIL + 100), (ext, strip, 200), M_ROCK)
        box(name + "_CeilE", CUBE, (cx + off, 0, CEIL + 100), (strip, hole, 200), M_ROCK)
        box(name + "_CeilW", CUBE, (cx - off, 0, CEIL + 100), (strip, hole, 200), M_ROCK)
    (door_wall if door_west else solid_wall_x)(name + "_WallW", cx - half - WALL_T / 2.0, 0, half)
    (door_wall if door_east else solid_wall_x)(name + "_WallE", cx + half + WALL_T / 2.0, 0, half)
    solid_wall_y(name + "_WallN", cx, half + WALL_T / 2.0, half)
    solid_wall_y(name + "_WallS", cx, -(half + WALL_T / 2.0), half)

def tunnel(name, x1, x2):
    cx = (x1 + x2) / 2.0
    length = (x2 - x1) + 400.0  # overlap into chamber walls
    box(name + "_Floor", CUBE, (cx, 0, -100), (length, DOOR_W + 400, 200), M_ROCK)
    box(name + "_Ceiling", CUBE, (cx, 0, DOOR_H + 100), (length, DOOR_W + 400, 200), M_ROCK)
    box(name + "_WallN", CUBE, (cx, DOOR_W / 2.0 + 100, DOOR_H / 2.0), (length, WALL_T, DOOR_H), M_ROCK)
    box(name + "_WallS", CUBE, (cx, -(DOOR_W / 2.0 + 100), DOOR_H / 2.0), (length, WALL_T, DOOR_H), M_ROCK)
    # Wood support beams across the top just outside the walkway width.
    for bx in (x1 + 300, cx, x2 - 300):
        box(name + "_Beam", CUBE, (bx, 0, DOOR_H - 25), (80, DOOR_W + 400, 50), M_WOOD)

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

def scatter_rocks(cx, half, count):
    for _ in range(count):
        # Near walls only: keeps the y=0 traversal corridor (and doors) clear.
        y = rng.choice((-1, 1)) * rng.uniform(half - 650, half - 200)
        x = cx + rng.uniform(-half + 250, half - 250)
        s = rng.uniform(150, 480)
        if rng.random() < 0.5:
            box("RockProp", CUBE, (x, y, s * 0.3), (s, s * rng.uniform(0.6, 1.2), s * 0.7), M_ROCK, rng.uniform(0, 180))
        else:
            box("RockProp", SPHERE, (x, y, s * 0.15), (s, s * 0.8, s * 0.6), M_ROCK, rng.uniform(0, 180))

def crates(cx, half):
    for sx, sy in ((-1, -1), (1, 1)):
        x, y = cx + sx * (half - 350), sy * (half - 350)
        box("Crate", CUBE, (x, y, 60), (120, 120, 120), M_WOOD, rng.uniform(0, 45))
        box("Crate", CUBE, (x + 40, y - 30, 180), (110, 110, 110), M_WOOD, rng.uniform(0, 45))

# --- layout: chambers on +X, centreline y=0 ---
C1, C2, C3, HOARD = 0.0, 6000.0, 12000.0, 19000.0
H = 1500.0       # chamber half-size (3000x3000)
HH = 2000.0      # hoard half-size (4000x4000, largest)

chamber("Entry", C1, H, door_west=False, door_east=True)
chamber("Chamber2", C2, H, door_west=True, door_east=True)
chamber("Chamber3", C3, H, door_west=True, door_east=True)
chamber("Hoard", HOARD, HH, door_west=True, door_east=False, shaft_hole=True)

tunnel("Tunnel1", C1 + H, C2 - H)
tunnel("Tunnel2", C2 + H, C3 - H)
tunnel("Tunnel3", C3 + H, HOARD - HH)

# --- peak shaft: 700x700 chimney from hoard ceiling (1200) up 4000 to 5200 ---
SHAFT_TOP = CEIL + 4000.0
sz = (CEIL + SHAFT_TOP) / 2.0
box("ShaftWallE", CUBE, (HOARD + 450, 0, sz), (WALL_T, 1100, 4000), M_ROCK)
box("ShaftWallW", CUBE, (HOARD - 450, 0, sz), (WALL_T, 1100, 4000), M_ROCK)
box("ShaftWallN", CUBE, (HOARD, 450, sz), (700, WALL_T, 4000), M_ROCK)
box("ShaftWallS", CUBE, (HOARD, -450, sz), (700, WALL_T, 4000), M_ROCK)
# Crater lip at the top opening (annulus slabs, centre stays open).
lip_off = 350 + 300
box("PeakLipN", CUBE, (HOARD, lip_off, SHAFT_TOP + 50), (1900, 600, 100), M_ROCK)
box("PeakLipS", CUBE, (HOARD, -lip_off, SHAFT_TOP + 50), (1900, 600, 100), M_ROCK)
box("PeakLipE", CUBE, (HOARD + lip_off, 0, SHAFT_TOP + 50), (600, 700, 100), M_ROCK)
box("PeakLipW", CUBE, (HOARD - lip_off, 0, SHAFT_TOP + 50), (600, 700, 100), M_ROCK)
for _ in range(5):
    ang = rng.uniform(0, 6.283)
    import math
    s = rng.uniform(200, 450)
    box("PeakRock", SPHERE, (HOARD + math.cos(ang) * 800, math.sin(ang) * 800, SHAFT_TOP + 100),
        (s, s * 0.8, s * 0.6), M_ROCK, rng.uniform(0, 180))

# --- props ---
scatter_rocks(C1, H, 7)
scatter_rocks(C2, H, 8)
scatter_rocks(C3, H, 8)
scatter_rocks(HOARD, HH, 8)
crates(C1, H)
crates(C2, H)
crates(C3, H)
gold_pile(C2 + 800, 1000, 0.6)
gold_pile(C3 - 700, -1050, 0.8)
for gx, gy, gs in ((HOARD - 1200, 1300, 1.2), (HOARD + 1200, 1300, 1.4),
                   (HOARD - 1200, -1300, 1.4), (HOARD + 1200, -1300, 1.2),
                   (HOARD + 1500, 0, 1.1), (HOARD, 1550, 1.0), (HOARD, -1550, 1.0)):
    gold_pile(gx, gy, gs)

# --- player start + lights ---
start = require(eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(C1, 0, 150)), "spawn PlayerStart")
start.set_actor_label("DragonStart")

point_light("Light_Entry", (C1, 0, 900))
point_light("Light_Chamber2", (C2, 0, 900))
point_light("Light_Chamber3", (C3, 0, 900))
point_light("Light_HoardMain", (HOARD, 0, 950), intensity=9000.0, radius=3600.0)
point_light("Light_HoardN", (HOARD, 1400, 700), intensity=4000.0, radius=2200.0)
point_light("Light_HoardS", (HOARD, -1400, 700), intensity=4000.0, radius=2200.0)
point_light("Light_Tunnel1", ((C1 + C2) / 2.0, 0, 450), intensity=2500.0, radius=1800.0)
point_light("Light_Tunnel2", ((C2 + C3) / 2.0, 0, 450), intensity=2500.0, radius=1800.0)
point_light("Light_Tunnel3", ((C3 + HOARD) / 2.0, 0, 450), intensity=2500.0, radius=1800.0)
point_light("Light_ShaftGlow", (HOARD, 0, 3000), color=(0.75, 0.85, 1.0), intensity=5000.0, radius=3000.0)

sun = require(
    eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(HOARD + 600, 400, 7500)),
    "spawn directional light",
)
sun.set_actor_label("PeakSun")
sun.set_actor_rotation(unreal.Rotator(-82.0, -30.0, 0.0), False)
sun.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(3.0)

sky = require(eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(HOARD, 0, 6200)), "spawn sky light")
sky.set_actor_label("PeakSkyLight")

require(les.save_current_level(), "save level %s" % MAP_PATH)
require(eal.save_directory(MAT_DIR), "save materials in %s" % MAT_DIR)
print("LAIR_CREATED", MAP_PATH, "static_mesh_actors=", COUNTER[0])
