# [2026-10-28 T-014] Author Content/Gameplay/Thieves/ data assets (numbers from
# docs/GAMEPLAY.md section 5) and place one thief of each type in Lair.umap,
# each in the same chamber as a treasure so steering has a clear run.
# [2026-10-28 T-037, applied on the T-014 resume] Every Unreal helper
# result is checked through require(): a failed delete/create/load/
# spawn/save raises before any success text prints.
import unreal

DIR = "/Game/Gameplay/Thieves"
MAP_PATH = "/Game/Maps/Lair/Lair"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    if not value:
        raise RuntimeError("create_thieves failed: %s" % action)
    return value


# [2026-11-03 T-066] unreal.load_asset, not eal.load_asset: the latter
# is registry-backed and returns None headless even for on-disk assets
# (proven this session), while the former loads by package path directly
# (the T-046 dashed-name precedent in create_exterior.py).
def _load(path, what):
    return require(unreal.load_asset(path), "load %s %s" % (what, path))


# name, display, hp, sneak, flee, grab_s, cap, drops_all_on_roar, can_trap, scale
THIEVES = [
    ("DA_Thief_Burglar", "Burglar", 20, 220, 520, 1.5, 1, True, False, (0.8, 0.8, 0.8)),
    ("DA_Thief_DwarfParty", "Dwarf Carrier", 45, 180, 380, 2.5, 3, False, True, (0.9, 0.9, 0.9)),
    ("DA_Thief_Adventurer", "Adventurer", 80, 260, 480, 1.0, 2, True, False, (1.0, 1.0, 1.05)),
]
# [2026-10-28 T-044] Owner's Meshy models (docs/THIEF-ASSETS.md):
# one skeletal mesh + walk/run clips per type.
VISUALS = {
    "DA_Thief_Burglar": "burglar",
    "DA_Thief_DwarfParty": "dwarf_carrier",
    "DA_Thief_Adventurer": "adventurer",
}
for name, display, hp, sneak, flee, grab, cap, drops, traps, scale in THIEVES:
    path = DIR + "/" + name
    # [2026-10-28 T-014 resume] Update in place when the asset
    # exists: delete+recreate fails in-session for registry-loaded
    # DataAssets (delete reports success but the loaded package
    # still occupies the path, so create returns None — proven).
    # [2026-11-03 T-066] Load-first via unreal.load_asset (package path,
    # not the blind registry): existing assets update in place.
    da = _load(path, "thief asset")
    da.set_editor_property("display_name", display)
    da.set_editor_property("max_hp", float(hp))
    da.set_editor_property("sneak_speed", float(sneak))
    da.set_editor_property("flee_speed", float(flee))
    da.set_editor_property("grab_time_seconds", float(grab))
    da.set_editor_property("carried_cap", cap)
    da.set_editor_property("drops_all_loot_on_roar", drops)
    da.set_editor_property("can_place_traps", traps)
    da.set_editor_property("body_scale", unreal.Vector(*scale))
    slug = VISUALS[name]
    mesh_dir = "/Game/Meshy/Thieves/"
    da.set_editor_property("mesh_asset", _load(mesh_dir + slug + "_rigged", "mesh %s" % slug))
    da.set_editor_property("walk_anim", _load(mesh_dir + slug + "_walking_Anim", "walk anim %s" % slug))
    da.set_editor_property("run_anim", _load(mesh_dir + slug + "_running_Anim", "run anim %s" % slug))
    require(eal.save_asset(path), "save thief asset %s" % path)
    print("THIEF_ASSET", path, display)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world after loading %s" % MAP_PATH)
require(
    world.get_path_name().startswith(MAP_PATH + "."),
    "verify editor world is %s (got %s)" % (MAP_PATH, world.get_path_name()),
)
cls = require(unreal.load_class(None, "/Script/UnderTheMountain.ThiefCharacter"), "load ThiefCharacter class")
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("Thief_"):
        require(eas.destroy_actor(a), "destroy prior placement %s" % a.get_actor_label())

# label, asset, spawn, exit (spawn inside the mountain, exit at the
# nearest outside point: north crack / west cave / grand portal)
PLACEMENTS = [
    ("Thief_Burglar", "DA_Thief_Burglar", (7500, 3000, 150), (7500, 7200, 150)),
    ("Thief_Dwarf", "DA_Thief_DwarfParty", (6000, -2000, 150), (500, 0, 150)),
    ("Thief_Adventurer", "DA_Thief_Adventurer", (14000, 1500, 150), (18700, 0, 150)),
]
for label, asset, loc, exit_loc in PLACEMENTS:
    a = require(eas.spawn_actor_from_class(cls, unreal.Vector(*loc)), "spawn %s" % label)
    a.set_actor_label(label)
    a.set_editor_property("thief_data", _load(DIR + "/" + asset, "thief asset %s" % asset))
    a.set_editor_property("exit_location", unreal.Vector(*exit_loc))
    print("THIEF_PLACED", label, loc, "exit", exit_loc)
require(les.save_current_level(), "save level %s" % MAP_PATH)
print("THIEVES_DONE")
