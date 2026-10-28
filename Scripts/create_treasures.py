# [2026-10-28 T-013] Author Content/Gameplay/Treasures/: treasure DataAssets,
# scent/true-sight materials, and place treasure actors in Lair.umap.
# Values are docs/GAMEPLAY.md section 3/6 examples; brackets do the rest.
import unreal

DIR = "/Game/Gameplay/Treasures"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

def make_material(name, color, emissive_scale, disable_depth=False, translucent=False):
    path = DIR + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    mat = asset_tools.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())
    c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -100)
    c.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR if emissive_scale else unreal.MaterialProperty.MP_BASE_COLOR)
    if translucent:
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
        o = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 150)
        o.set_editor_property("parameter_name", "Opacity")
        o.set_editor_property("default_value", 1.0)
        mel.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)
    if disable_depth:
        mat.set_editor_property("disable_depth_test", True)
    mel.recompile_material(mat)
    eal.save_asset(path)
    return mat

make_material("M_ScentGold", (0.686, 0.525, 0.345), True, translucent=True)
make_material("M_TrueSight", (0.46, 0.68, 0.64), True, disable_depth=True)

TREASURES = [
    ("DA_Treasure_Goblet", "Gold Goblet", 10, False),
    ("DA_Treasure_JewelledCup", "Jewelled Cup", 50, False),
    ("DA_Treasure_Chest", "Adventurer Chest", 250, False),
    ("DA_Artifact_SeeingStone", "Seeing Stone", 1200, True),
]
for name, display, value, stone in TREASURES:
    path = DIR + "/" + name
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    da = asset_tools.create_asset(name, DIR, unreal.TreasureDataAsset, unreal.DataAssetFactory())
    da.set_editor_property("display_name", display)
    da.set_editor_property("gold_value", value)
    da.set_editor_property("is_seeing_stone", stone)
    eal.save_asset(path)
    print("TREASURE_ASSET", path, value, stone)

# --- place in the lair (loads Lair.umap, adds 4 actors, saves) ---
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/Maps/Lair/Lair")
cls = unreal.load_class(None, "/Script/UnderTheMountain.TreasureActor")
# remove prior placements so re-runs stay deterministic
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("Treasure_"):
        eas.destroy_actor(a)
PLACEMENTS = [
    ("Treasure_Goblet", "DA_Treasure_Goblet", (0, 900, 160)),
    ("Treasure_JewelledCup", "DA_Treasure_JewelledCup", (6000, -1000, 160)),
    ("Treasure_Chest", "DA_Treasure_Chest", (12000, 1000, 160)),
    ("Treasure_SeeingStone", "DA_Artifact_SeeingStone", (19000, 0, 300)),
]
for label, asset, loc in PLACEMENTS:
    a = eas.spawn_actor_from_class(cls, unreal.Vector(*loc))
    a.set_actor_label(label)
    a.set_editor_property("treasure_data", eal.load_asset(DIR + "/" + asset))
    print("TREASURE_PLACED", label, loc)
les.save_current_level()
print("TREASURES_DONE")
