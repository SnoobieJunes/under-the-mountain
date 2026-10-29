# [2026-10-28 T-017] Extend Lair.umap with the exterior flight zone:
# the peak shaft (top at z=5200 in create_lair.py) opens onto a rock
# plateau, terraced slopes drop east to a valley floor, and a greybox
# village + goat pasture sit ~190m east — one map, no load screen.
# Meshy world meshes (docs/WORLD-ASSETS.md, approval-gated) replace
# these primitives later; layout positions are the contract.
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
# Vista rocks + dead pines on the terraces (flight landmarks).
for i, (x, y, z) in enumerate(((28000, -3500, 4000), (31000, 4200, 2700), (34500, -5200, 1400), (25500, 6000, 5100))):
    box("Exterior_Rock%d" % i, SPHERE, (x, y, z), (900, 700, 1100), M_ROCK, yaw=i * 40)
    box("Exterior_Pine%d" % i, CONE, (x + 900, y + 700, z + 250), (260, 260, 900), M_WOOD)

# --- greybox village at x=38000 on the valley floor ---
VX = HOARD_X + 19000


def house(label, x, y, yaw, w=900, d=700, h=450):
    box(label + "_Walls", CUBE, (x, y, h / 2), (w, d, h), M_WOOD, yaw=yaw)
    box(label + "_Roof", CUBE, (x, y, h + 110), (w * 1.05, d * 0.62, 300), M_ROCK, yaw=yaw, pitch=0)
    box(label + "_RoofPeak", CUBE, (x, y, h + 200), (w * 1.05, 90, 90), M_WOOD, yaw=yaw)


house("Exterior_HouseA", VX - 1200, -900, 15)
house("Exterior_HouseB", VX + 400, -1400, -10)
house("Exterior_HouseC", VX + 1500, -500, 30, w=1100, d=800, h=520)
house("Exterior_HouseD", VX - 600, 1100, 45)
house("Exterior_HouseE", VX + 1100, 1300, -25)
house("Exterior_Inn", VX - 200, 0, 0, w=1600, d=1000, h=650)
box("Exterior_Tower", CUBE, (VX + 2100, 400, 550), (500, 500, 1100), M_ROCK)
box("Exterior_TowerTop", CONE, (VX + 2100, 400, 1350), (620, 620, 520), M_WOOD)
box("Exterior_Well", CUBE, (VX + 100, -300, 60), (220, 220, 120), M_ROCK)

# --- goats in the pasture west of the village ---
goat_cls = require(unreal.load_class(None, "/Script/UnderTheMountain.GoatActor"), "load GoatActor class")
for i, (gx, gy) in enumerate(((VX - 3200, -1800), (VX - 2600, 900), (VX - 3800, 300), (VX - 2000, -400), (VX - 3000, 2100))):
    g = require(eas.spawn_actor_from_class(goat_cls, unreal.Vector(gx, gy, 170)), "spawn goat %d" % i)
    g.set_actor_label("Goat_%d" % i)
    print("GOAT_PLACED", i, gx, gy)

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("EXTERIOR_DONE")
