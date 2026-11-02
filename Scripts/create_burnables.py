# [2026-11-01 T-055] Place the burnable props: ABurnablePropActor
# with des-* intact/rubble mesh pairs at the valley village and
# the northern village. The valley's two old static placements
# (Exterior_Crates / Exterior_Barrels from create_exterior.py) are
# destroyed here and replaced by burnable versions - re-run
# create_exterior.py first and this second if both are re-run, or
# the statics come back beside the burnables.
# Re-runnable: Burnable_* actors are destroyed and respawned.
import unreal

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
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."),
        "verify editor world is %s" % MAP_PATH)

REPLACE_STATICS = ("Exterior_Crates", "Exterior_Barrels")
for a in list(eas.get_all_level_actors()):
    label = a.get_actor_label()
    if label.startswith("Burnable_") or label in REPLACE_STATICS:
        require(eas.destroy_actor(a), "destroy prior burnable/static %s" % label)

cls = require(unreal.load_class(None, "/Script/UnderTheMountain.BurnablePropActor"),
              "load BurnablePropActor class")
DIR = "/Game/Meshy/World/"

# (label, intact, rubble, x, y, z)
PROPS = [
    ("Burnable_ValleyCrates", "des-crates-intact", "des-crates-rubble", 38850, 650, 60),
    ("Burnable_ValleyBarrels", "des-barrels-intact", "des-barrels-rubble", 39000, 780, 60),
    ("Burnable_ValleyCart", "des-cart-intact", "des-cart-rubble", 37100, 300, 60),
    ("Burnable_ValleyFence", "des-fence-intact", "des-fence-rubble", 36200, 1800, 60),
    ("Burnable_NorthCrates", "des-crates-intact", "des-crates-rubble", -149500, 85600, None),
    ("Burnable_NorthBarrels", "des-barrels-intact", "des-barrels-rubble", -150800, 84300, None),
]

import math


def terrain_height(x, y):
    # Replica of the terrain closed form (see create_exterior.py).
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


for label, intact, rubble, x, y, z in PROPS:
    if z is None:
        z = terrain_height(x, y) + 10.0
    a = require(eas.spawn_actor_from_class(cls, unreal.Vector(x, y, z)), "spawn %s" % label)
    a.set_actor_label(label)
    a.set_editor_property("intact_mesh", require(unreal.load_asset(DIR + intact), "load %s" % intact))
    a.set_editor_property("rubble_mesh", require(unreal.load_asset(DIR + rubble), "load %s" % rubble))
    print("BURNABLE_PLACED", label, x, y, z)

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("BURNABLES_DONE", len(PROPS))
