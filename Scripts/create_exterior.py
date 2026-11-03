# [2026-10-28 T-017] Extend Lair.umap with the exterior flight zone:
# the peak shaft (top at z=5200 in create_lair.py) opens onto a rock
# plateau, terraced slopes drop east to a valley floor, and a greybox
# village + goat pasture sit ~190m east — one map, no load screen.
# [2026-10-28 T-046] Village/rocks/pines are the owner's Meshy world
# meshes, placed at the T-017 layout positions and scaled per asset
# from their ~1m normalized imports. Terraces/plateau stay rock boxes.
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
eal = unreal.EditorAssetLibrary

# [2026-11-01 T-050] Same commandlet warm-up as create_treasures.py:
# in -run=pythonscript the Asset Registry is cold at script start and
# load_asset lies about on-disk assets until a synchronous scan.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Maps", "/Game/Meshy"], True, True)


def require(value, action):
    if not value:
        raise RuntimeError("create_exterior failed: %s" % action)
    return value


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."), "verify editor world is %s" % MAP_PATH)

for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("Exterior_"):
        require(eas.destroy_actor(a), "destroy prior exterior actor %s" % a.get_actor_label())
    if a.get_actor_label().startswith("Goat_"):
        require(eas.destroy_actor(a), "destroy prior goat %s" % a.get_actor_label())

CUBE = require(unreal.load_asset("/Engine/BasicShapes/Cube"), "load Cube")
CONE = require(unreal.load_asset("/Engine/BasicShapes/Cone"), "load Cone")
SPHERE = require(unreal.load_asset("/Engine/BasicShapes/Sphere"), "load Sphere")
M_ROCK = require(eal.load_asset("/Game/Maps/Lair/M_MountainRock"), "load M_MountainRock")
M_WOOD = require(eal.load_asset("/Game/Maps/Lair/M_MountainWood"), "load M_MountainWood")
M_GOLD = require(eal.load_asset("/Game/Maps/Lair/M_MountainGold"), "load M_MountainGold")

HOARD_X = 19000.0  # terrace/village/massif anchor (unchanged T-017/T-050 geography)


def box(label, mesh, center, size, mat, yaw=0.0, pitch=0.0):
    a = require(eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*center)), "spawn %s" % label)
    a.set_actor_label(label)
    comp = a.static_mesh_component
    comp.set_static_mesh(mesh)
    if mat is not None:
        comp.set_material(0, mat)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    if yaw or pitch:
        a.set_actor_rotation(unreal.Rotator(pitch, yaw, 0.0), False)
    return a


# Peak shelf around the new crown (clear of the 25 m hole AND the touchdown
# disc): from x=11000 (hole edge 10750 + margin) to Terrace1 (x 23000).
# Terraces/village keep their T-017/T-050 positions downstream.
box("Exterior_Plateau", CUBE, (17525, 0, 5050), (13050, 14000, 200), M_ROCK)
for i, (cx, top_z) in enumerate(((HOARD_X + 9000, 3900), (HOARD_X + 15000, 2600), (HOARD_X + 21000, 1300))):
    box("Exterior_Terrace%d" % (i + 1), CUBE, (cx, 0, top_z - 150), (10000, 16000, 300), M_ROCK)
# Valley floor: village + pasture ground (top at z=0).
box("Exterior_Valley", CUBE, (HOARD_X + 26000, 0, -150), (36000, 42000, 300), M_ROCK)
# Vista rocks + pines on the terraces (flight landmarks) — real
# Meshy meshes since T-046, scaled from their ~1m normalized import.
def place_mesh(label, asset_name, x, y, ground_z, target_h, yaw=0.0):
    # unreal.load_asset, not eal.load_asset: the latter returns None
    # for these dashed Meshy names despite listing them (proven).
    mesh = require(unreal.load_asset("/Game/Meshy/World/" + asset_name), "load %s" % asset_name)
    bounds = mesh.get_bounds()
    height = max(1.0, 2.0 * bounds.box_extent.z)
    scale = target_h / height
    a = require(eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, ground_z + target_h / 2.0)), "spawn %s" % label)
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    if yaw:
        # [2026-11-01 T-050] Keyword args are load-bearing: this
        # Python build assigns Rotator positionally as
        # (roll, pitch, yaw), so Rotator(0.0, yaw, 0.0) PITCHED
        # every yawed building by its yaw angle - the valley
        # village has stood tilted since T-046 (proven by actor
        # dumps: House0 pitch=40 yaw=0). Keywords cannot drift.
        a.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False)
    return a


ROCKS = ("nat-rock-a", "nat-rock-b", "nat-rock-c", "nat-rock-a")
for i, (x, y, z) in enumerate(((28000, -3500, 4000), (31000, 4200, 2700), (34500, -5200, 1400), (25500, 6000, 5100))):
    place_mesh("Exterior_Rock%d" % i, ROCKS[i], x, y, z - 400, 1100, yaw=i * 40)
    place_mesh("Exterior_Pine%d" % i, "nat-pine", x + 900, y + 700, z - 150, 800)

# --- village at x=38000 on the valley floor: real Meshy meshes at
# the greybox layout positions (T-046), heights in cm ---
VX = HOARD_X + 19000

place_mesh("Exterior_HouseA", "vil-house-a", VX - 1200, -900, 0, 620, yaw=15)
place_mesh("Exterior_HouseB", "vil-house-b", VX + 400, -1400, 0, 620, yaw=-10)
place_mesh("Exterior_HouseC", "vil-house-c", VX + 1500, -500, 0, 780, yaw=30)
place_mesh("Exterior_HouseD", "vil-house-a", VX - 600, 1100, 0, 560, yaw=45)
place_mesh("Exterior_HouseE", "vil-house-b", VX + 1100, 1300, 0, 680, yaw=-25)
place_mesh("Exterior_Inn", "vil-inn", VX - 200, 0, 0, 850, yaw=0)
place_mesh("Exterior_Tower", "cas-tower", VX + 2100, 400, 0, 1150)
place_mesh("Exterior_Well", "vil-well", VX + 100, -300, 0, 170)
place_mesh("Exterior_Stall", "vil-market-stall", VX + 700, -200, 0, 320, yaw=20)
place_mesh("Exterior_Cart", "vil-cart", VX - 900, 300, 0, 200, yaw=70)
place_mesh("Exterior_Crates", "des-crates-intact", VX + 850, 650, 0, 130)
place_mesh("Exterior_Barrels", "des-barrels-intact", VX + 1000, 780, 0, 130)
place_mesh("Exterior_FenceA", "vil-fences", VX - 1800, 1800, 0, 160, yaw=90)
place_mesh("Exterior_FenceB", "vil-fences", VX - 1800, -200, 0, 160, yaw=90)

# --- settlements across the T-049 terrain (T-050, 2026-11-01) ---
# Distances are flight-visible: sites sit 1.5-4 km apart over the
# 5 km landscape, each with a different mix/layout (ring village,
# road village, courtyard castles, lone farmsteads) - no copy-paste
# rows. Grounding replicates the terrain commandlet's closed-form
# height function (TerrainCommandlet.cpp TerrainHeightCm - keep the
# constants in sync; the landscape is a bilinear surface over the
# same samples, so buildings sink 60 cm to absorb interpolation).
import math


def terrain_height(x, y):
    fields = 350.0 * math.sin(x * 2.1e-5 + 1.7) * math.cos(y * 1.7e-5) \
        + 250.0 * math.sin((x + y) * 1.3e-5)
    dist = math.hypot(x - 10000.0, y)
    t = max(0.0, min(1.0, (dist - 70000.0) / 80000.0))
    range_mask = t * t * (3.0 - 2.0 * t)
    ridge = (1.0 - abs(math.sin(x * 4.5e-5 + 2.0 * math.sin(y * 3.0e-5)))) ** 3
    ranges = range_mask * (900.0 + 3400.0 * ridge)
    massif = 4600.0 * math.exp(-((x - 19000.0) ** 2 + (y + 10000.0) ** 2)
                               / (2.0 * 20000.0 ** 2))
    h = fields + ranges + massif
    cx = 1.0 - (lambda u: u * u * (3 - 2 * u))(max(0.0, min(1.0, (x - 21000.0) / 5000.0)))
    cy = 1.0 - (lambda u: u * u * (3 - 2 * u))(max(0.0, min(1.0, (abs(y) - 7000.0) / 3000.0)))
    h = h * (1.0 - cx * cy) + (-250.0) * (cx * cy)
    return max(-800.0, min(5200.0, h))


def settle(label, asset, x, y, target_h, yaw=0.0):
    return place_mesh(label, asset, x, y, terrain_height(x, y) - 60.0, target_h, yaw)


# Castle A (north-west): courtyard square - keep at the back, two
# towers on the front corners, wall runs between, gate front-centre.
AX, AY = -120000.0, -120000.0
settle("Exterior_CastleA_Keep", "cas-keep", AX, AY - 1500, 2400)
settle("Exterior_CastleA_TowerW", "cas-tower", AX - 2200, AY + 1200, 1500)
settle("Exterior_CastleA_TowerE", "cas-tower", AX + 2200, AY + 1200, 1500)
settle("Exterior_CastleA_WallW", "cas-wall", AX - 1100, AY + 1200, 700, yaw=90)
settle("Exterior_CastleA_WallE", "cas-wall", AX + 1100, AY + 1200, 700, yaw=90)
settle("Exterior_CastleA_Gate", "cas-gate", AX, AY + 1200, 900)
settle("Exterior_CastleA_WallBack", "cas-wall", AX, AY - 2800, 700)

# Castle B (south-east): linear ridge fort - keep centre, towers at
# both ends, walls stepping down the ridge, no gate (cliff approach).
BX, BY = 150000.0, 135000.0
settle("Exterior_CastleB_Keep", "cas-keep", BX, BY, 2600)
settle("Exterior_CastleB_TowerW", "cas-tower", BX - 3000, BY + 600, 1500, yaw=20)
settle("Exterior_CastleB_TowerE", "cas-tower", BX + 3000, BY - 600, 1500, yaw=-15)
settle("Exterior_CastleB_WallW", "cas-wall", BX - 1500, BY + 300, 700, yaw=80)
settle("Exterior_CastleB_WallE", "cas-wall", BX + 1500, BY - 300, 700, yaw=80)

# Village 2 (north): a ring of houses around the well - different
# mix (B/C-led) and geometry from the valley village's road layout.
NX, NY = -150000.0, 85000.0
settle("Exterior_VillageN_Well", "vil-well", NX, NY, 170)
for i, (dx, dy, asset, yaw) in enumerate((
        (-1600, -600, "vil-house-b", 40), (1500, -900, "vil-house-c", -30),
        (-700, 1500, "vil-house-c", 160), (1200, 1300, "vil-house-b", -140),
        (0, -1900, "vil-house-a", 10), (-2100, 700, "vil-house-a", 80))):
    settle("Exterior_VillageN_House%d" % i, asset, NX + dx, NY + dy, 640, yaw=yaw)
settle("Exterior_VillageN_Stall", "vil-market-stall", NX + 500, NY + 300, 320, yaw=55)

# Village 3 (east): an inn-led road bend - inn at the corner, houses
# trailing east, tower watching the road.
EX, EY = 185000.0, -65000.0
settle("Exterior_VillageE_Inn", "vil-inn", EX, EY, 880, yaw=35)
settle("Exterior_VillageE_House0", "vil-house-c", EX + 1800, EY + 700, 700, yaw=60)
settle("Exterior_VillageE_House1", "vil-house-a", EX + 3200, EY + 1500, 600, yaw=75)
settle("Exterior_VillageE_House2", "vil-house-b", EX - 400, EY + 2000, 660, yaw=-20)
settle("Exterior_VillageE_Tower", "cas-tower", EX - 1700, EY - 800, 1200)
settle("Exterior_VillageE_Cart", "vil-cart", EX + 900, EY + 200, 200, yaw=110)

# Farmsteads (4): one house + working clutter each, every one a
# different pairing - fences/crates/cart/deadtrees in rotation.
for i, (fx, fy, asset, extra) in enumerate((
        (80000.0, 155000.0, "vil-house-a", "vil-fences"),
        (-65000.0, -185000.0, "vil-house-b", "vil-cart"),
        (125000.0, -145000.0, "vil-house-c", "des-crates-intact"),
        (-185000.0, -45000.0, "vil-house-b", "vil-fences"))):
    settle("Exterior_Farm%d_House" % i, asset, fx, fy, 600, yaw=i * 35)
    settle("Exterior_Farm%d_Extra" % i, extra, fx + 1100, fy + 500, 220, yaw=20 + i * 15)
    settle("Exterior_Farm%d_Tree" % i, "nat-deadtree-a", fx - 900, fy - 700, 700)
    settle("Exterior_Farm%d_Fence" % i, "vil-fences", fx + 300, fy - 1100, 160, yaw=90)
print("SETTLEMENTS_DONE")

# --- goats in the pasture west of the village ---
goat_cls = require(unreal.load_class(None, "/Script/UnderTheMountain.GoatActor"), "load GoatActor class")
for i, (gx, gy) in enumerate(((VX - 3200, -1800), (VX - 2600, 900), (VX - 3800, 300), (VX - 2000, -400), (VX - 3000, 2100))):
    g = require(eas.spawn_actor_from_class(goat_cls, unreal.Vector(gx, gy, 170)), "spawn goat %d" % i)
    g.set_actor_label("Goat_%d" % i)
    print("GOAT_PLACED", i, gx, gy)

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("EXTERIOR_DONE")
