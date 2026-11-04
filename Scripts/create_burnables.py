# [2026-11-01 T-055] Place the burnable props: ABurnablePropActor
# with des-* intact/rubble mesh pairs at the valley village and
# the northern village. The valley's two old static placements
# (Exterior_Crates / Exterior_Barrels from create_exterior.py) are
# destroyed here and replaced by burnable versions - re-run
# create_exterior.py first and this second if both are re-run, or
# the statics come back beside the burnables.
# Re-runnable: Burnable_* actors are destroyed and respawned.
# [2026-11-03 T-106] Re-run guard (shared map_guard module): .umap
# backup before any destroy, pre-destroy census, post-spawn uniqueness
# assert before save — a mis-ordered re-run fails loudly instead of
# persisting duplicates, and the backup restores the pre-run map.
import os as _os
import sys as _sys

import unreal

_sys.path.insert(0, _os.path.join(unreal.Paths.project_dir(), "Scripts"))
import map_guard
# [2026-11-04 T-122] Heightfield single source of truth (was a local
# replica here - second copy of the T-050 closed form).
from terrain_profile import terrain_height

MAP_PATH = "/Game/Maps/Lair/Lair"
eal = unreal.EditorAssetLibrary


def require(value, action):
    if not value:
        raise RuntimeError("create_burnables failed: %s" % action)
    return value


unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Maps", "/Game/Meshy"], True, True)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_guard.backup_level("burnables")
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."),
        "verify editor world is %s" % MAP_PATH)

REPLACE_STATICS = ("Exterior_Crates", "Exterior_Barrels")
map_guard.log_census(eas, ("Burnable_",) + REPLACE_STATICS)
for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    if label.startswith("Burnable_") or label in REPLACE_STATICS:
        require(eas.destroy_actor(a), "destroy prior burnable/static %s" % label)

cls = require(unreal.load_class(None, "/Script/UnderTheMountain.BurnablePropActor"),
              "load BurnablePropActor class")
DIR = "/Game/Meshy/World/"

# (label, intact, rubble, x, y, z)
PROPS = [
    # [2026-11-04 T-122] Valley props follow the village VX move
    # (38000 -> 50000): same relative offsets, all clear of T3.
    ("Burnable_ValleyCrates", "des-crates-intact", "des-crates-rubble", 50850, 650, 60),
    ("Burnable_ValleyBarrels", "des-barrels-intact", "des-barrels-rubble", 51000, 780, 60),
    ("Burnable_ValleyCart", "des-cart-intact", "des-cart-rubble", 49100, 300, 60),
    ("Burnable_ValleyFence", "des-fence-intact", "des-fence-rubble", 48200, 1800, 60),
    # [2026-11-04 T-122] North props follow Village N
    # ((-150000, 85000) -> (-155000, 55000)): same relative offsets.
    ("Burnable_NorthCrates", "des-crates-intact", "des-crates-rubble", -154500, 55600, None),
    ("Burnable_NorthBarrels", "des-barrels-intact", "des-barrels-rubble", -155800, 54300, None),
]


for label, intact, rubble, x, y, z in PROPS:
    if z is None:
        z = terrain_height(x, y) + 10.0
    a = require(eas.spawn_actor_from_class(cls, unreal.Vector(x, y, z)), "spawn %s" % label)
    a.set_actor_label(label)
    a.set_editor_property("intact_mesh", require(unreal.load_asset(DIR + intact), "load %s" % intact))
    a.set_editor_property("rubble_mesh", require(unreal.load_asset(DIR + rubble), "load %s" % rubble))
    print("BURNABLE_PLACED", label, x, y, z)

map_guard.assert_unique_labels(eas, ("Burnable_",))
require(les.save_current_level(), "save level %s" % MAP_PATH)
print("BURNABLES_DONE", len(PROPS))
