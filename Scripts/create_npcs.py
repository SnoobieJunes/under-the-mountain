# [2026-11-01 T-053] Populate the settlements: villagers in two of
# them and goat herds at the farmsteads.
# Villagers (AVillagerActor) go to the valley village (the original
# settlement, flat valley floor) and Village N (the northern ring,
# terrain-grounded). Farm goats are AGoatActor at each of the four
# T-050 farmsteads, two apiece, wandering their own home pasture.
# Re-runnable: prior Villager_*/FarmGoat_* actors are destroyed and
# respawned at the same deterministic spots (offsets are fixed
# tables, never RNG, so evidence counts are stable).
import math
import unreal

MAP_PATH = "/Game/Maps/Lair/Lair"
eal = unreal.EditorAssetLibrary


def require(value, action):
    if not value:
        raise RuntimeError("create_npcs failed: %s" % action)
    return value


# Cold-registry warm-up for -run=pythonscript (see T-050).
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Maps"], True, True)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."),
        "verify editor world is %s" % MAP_PATH)

for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    if label.startswith("Villager_") or label.startswith("FarmGoat_"):
        require(eas.destroy_actor(a), "destroy prior npc %s" % label)


# Terrain height replica (kept in sync with TerrainCommandlet.cpp
# and create_exterior.py - same closed form, same constants).
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


villager_cls = require(
    unreal.load_class(None, "/Script/UnderTheMountain.VillagerActor"),
    "load VillagerActor class")
goat_cls = require(
    unreal.load_class(None, "/Script/UnderTheMountain.GoatActor"),
    "load GoatActor class")

# (settlement centre, ground z or None for terrain, offsets)
VILLAGES = [
    ("Valley", (38000.0, 0.0), 30.0,
     [(-800, -500), (600, 800), (-300, 1200), (1100, -900), (-1400, 300), (200, -1400)]),
    ("North", (-150000.0, 85000.0), None,
     [(-900, -400), (800, 600), (-200, 1000), (1200, -700), (-1300, 500)]),
]
count = 0
for name, (cx, cy), ground_z, offsets in VILLAGES:
    for i, (dx, dy) in enumerate(offsets):
        x, y = cx + dx, cy + dy
        z = ground_z if ground_z is not None else terrain_height(x, y)
        v = require(eas.spawn_actor_from_class(
            villager_cls, unreal.Vector(x, y, z)), "spawn villager %s %d" % (name, i))
        v.set_actor_label("Villager_%s_%d" % (name, i))
        v.set_home_location(unreal.Vector(x, y, z))
        count += 1
print("VILLAGERS_PLACED", count)

# Farm goats: the four T-050 farmstead centres, two goats each on
# offset pasture points (their wander home is the spawn point).
FARMS = [(80000.0, 155000.0), (-65000.0, -185000.0),
         (125000.0, -145000.0), (-185000.0, -45000.0)]
goats = 0
for i, (fx, fy) in enumerate(FARMS):
    for j, (dx, dy) in enumerate(((-500, -300), (700, 400))):
        x, y = fx + dx, fy + dy
        g = require(eas.spawn_actor_from_class(
            goat_cls, unreal.Vector(x, y, terrain_height(x, y) + 120.0)),
            "spawn farm goat %d %d" % (i, j))
        g.set_actor_label("FarmGoat_%d_%d" % (i, j))
        goats += 1
print("FARM_GOATS_PLACED", goats)

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("NPCS_DONE")
