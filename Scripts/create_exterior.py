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
M_ROCK = require(eal.load_asset("/Game/Maps/Lair/M_RockDark"), "load M_RockDark")
M_WOOD = require(eal.load_asset("/Game/Maps/Lair/M_WoodDark"), "load M_WoodDark")
M_GOLD = require(eal.load_asset("/Game/Maps/Lair/M_GoldWarm"), "load M_GoldWarm")

HOARD_X = 19000.0
TOP = 5200.0  # shaft exit height from create_lair.py


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


# Peak plateau around the shaft mouth, then terraces stepping east
# down to the valley (each 1300 lower, overlapping so flight and
# walking both read as one mountainside).
box("Exterior_Plateau", CUBE, (HOARD_X, 0, TOP - 100), (14000, 14000, 200), M_ROCK)
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
        a.set_actor_rotation(unreal.Rotator(0.0, yaw, 0.0), False)
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

# --- goats in the pasture west of the village ---
goat_cls = require(unreal.load_class(None, "/Script/UnderTheMountain.GoatActor"), "load GoatActor class")
for i, (gx, gy) in enumerate(((VX - 3200, -1800), (VX - 2600, 900), (VX - 3800, 300), (VX - 2000, -400), (VX - 3000, 2100))):
    g = require(eas.spawn_actor_from_class(goat_cls, unreal.Vector(gx, gy, 170)), "spawn goat %d" % i)
    g.set_actor_label("Goat_%d" % i)
    print("GOAT_PLACED", i, gx, gy)

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("EXTERIOR_DONE")
