# [2026-11-01 T-053] Populate the settlements: villagers in two of
# them and goat herds at the farmsteads.
# Villagers (AVillagerActor) go to the valley village (the original
# settlement, flat valley floor) and Village N (the northern ring,
# terrain-grounded). Farm goats are AGoatActor at each of the four
# T-050 farmsteads, two apiece, wandering their own home pasture.
# Re-runnable: prior Villager_*/FarmGoat_* actors are destroyed and
# respawned at the same deterministic spots (offsets are fixed
# tables, never RNG, so evidence counts are stable).
import os as _os
import sys as _sys

import unreal

_sys.path.insert(0, _os.path.join(unreal.Paths.project_dir(), "Scripts"))
import map_guard
# [2026-11-04 T-122] Heightfield single source of truth (was a local
# replica here - third copy of the T-050 closed form).
from terrain_profile import terrain_height

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
map_guard.backup_level("npcs")
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."),
        "verify editor world is %s" % MAP_PATH)

map_guard.log_census(eas, ("Villager_", "FarmGoat_"))
for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    if label.startswith("Villager_") or label.startswith("FarmGoat_"):
        require(eas.destroy_actor(a), "destroy prior npc %s" % label)


# Terrain height: Scripts/terrain_profile.py (single source of truth
# since T-122; the C++ commandlet is its documented twin).


villager_cls = require(
    unreal.load_class(None, "/Script/UnderTheMountain.VillagerActor"),
    "load VillagerActor class")
goat_cls = require(
    unreal.load_class(None, "/Script/UnderTheMountain.GoatActor"),
    "load GoatActor class")

# (settlement centre, ground z or None for terrain, offsets)
VILLAGES = [
    # [2026-11-04 T-122] Valley centre follows create_exterior.py VX
    # (38000 -> 50000: old site under the terrace slabs).
    ("Valley", (50000.0, 0.0), 30.0,
     [(-800, -500), (600, 800), (-300, 1200), (1100, -900), (-1400, 300), (200, -1400)]),
    # [2026-11-04 T-122] North centre follows create_exterior.py
    # ((-150000, 85000) -> (-155000, 55000): old ring on a slope).
    ("North", (-155000.0, 55000.0), None,
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

map_guard.assert_unique_labels(eas, ("Villager_", "FarmGoat_"))
require(les.save_current_level(), "save level %s" % MAP_PATH)
print("NPCS_DONE")
