# [2026-10-28 T-014] Author Content/Gameplay/Thieves/ data assets (numbers from
# docs/GAMEPLAY.md section 5) and place one thief of each type in Lair.umap,
# each in the same chamber as a treasure so steering has a clear run.
import unreal

DIR = "/Game/Gameplay/Thieves"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

# name, display, hp, sneak, flee, grab_s, cap, drops_all_on_roar, can_trap, scale
THIEVES = [
    ("DA_Thief_Burglar", "Burglar", 20, 220, 520, 1.5, 1, True, False, (0.8, 0.8, 0.8)),
    ("DA_Thief_DwarfParty", "Dwarf Carrier", 45, 180, 380, 2.5, 3, False, True, (0.9, 0.9, 0.9)),
    ("DA_Thief_Adventurer", "Adventurer", 80, 260, 480, 1.0, 2, True, False, (1.0, 1.0, 1.05)),
]
for name, display, hp, sneak, flee, grab, cap, drops, traps, scale in THIEVES:
    path = DIR + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    da = asset_tools.create_asset(name, DIR, unreal.ThiefDataAsset, unreal.DataAssetFactory())
    da.set_editor_property("display_name", display)
    da.set_editor_property("max_hp", float(hp))
    da.set_editor_property("sneak_speed", float(sneak))
    da.set_editor_property("flee_speed", float(flee))
    da.set_editor_property("grab_time_seconds", float(grab))
    da.set_editor_property("carried_cap", cap)
    da.set_editor_property("drops_all_loot_on_roar", drops)
    da.set_editor_property("can_place_traps", traps)
    da.set_editor_property("body_scale", unreal.Vector(*scale))
    eal.save_asset(path)
    print("THIEF_ASSET", path, display)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/Lair/Lair")
cls = unreal.load_class(None, "/Script/UnderTheMountain.ThiefCharacter")
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("Thief_"):
        eas.destroy_actor(a)

# label, asset, spawn, exit (reachable points on the y=0 corridor network)
PLACEMENTS = [
    ("Thief_Burglar", "DA_Thief_Burglar", (1300, -800, 150), (-1400, 0, 150)),
    ("Thief_Dwarf", "DA_Thief_DwarfParty", (4500, 800, 150), (19000, 0, 150)),
    ("Thief_Adventurer", "DA_Thief_Adventurer", (13500, -800, 150), (19000, 0, 150)),
]
for label, asset, loc, exit_loc in PLACEMENTS:
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc))
    a.set_actor_label(label)
    a.set_editor_property("thief_data", eal.load_asset(DIR + "/" + asset))
    a.set_editor_property("exit_location", unreal.Vector(*exit_loc))
    print("THIEF_PLACED", label, loc, "exit", exit_loc)
les.save_current_level()
print("THIEVES_DONE")
