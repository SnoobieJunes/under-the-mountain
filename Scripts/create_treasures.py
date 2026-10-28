# [2026-10-28 T-013] Author Content/Gameplay/Treasures/: treasure DataAssets,
# scent/true-sight materials, and place treasure actors in Lair.umap.
# Values are docs/GAMEPLAY.md section 3/6 examples; brackets do the rest.
import unreal

DIR = "/Game/Gameplay/Treasures"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def require(value, action):
    # [2026-10-28 T-037] Convert Unreal's False/None helper failures
    # into a raised, named failure before any success text prints.
    if not value:
        raise RuntimeError("create_treasures failed: %s" % action)
    return value


def make_material(name, color, emissive_scale, disable_depth=False, translucent=False):
    path = DIR + "/" + name
    if eal.does_asset_exist(path):
        require(eal.delete_asset(path), "delete existing material %s" % path)
    mat = require(
        asset_tools.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew()),
        "create material %s" % path,
    )
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
    require(eal.save_asset(path), "save material %s" % path)
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
        require(eal.delete_asset(path), "delete existing treasure asset %s" % path)
    da = require(
        asset_tools.create_asset(name, DIR, unreal.TreasureDataAsset, unreal.DataAssetFactory()),
        "create treasure asset %s" % path,
    )
    da.set_editor_property("display_name", display)
    da.set_editor_property("gold_value", value)
    da.set_editor_property("is_seeing_stone", stone)
    require(eal.save_asset(path), "save treasure asset %s" % path)
    print("TREASURE_ASSET", path, value, stone)

# --- place in the lair (loads Lair.umap, adds 4 actors, saves) ---
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MAP_PATH = "/Game/Maps/Lair/Lair"
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
# [2026-10-28 T-037] Also verify the editor world's identity: a
# successful load call followed by actor edits in some other current
# level would corrupt the wrong map while printing success.
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world after loading %s" % MAP_PATH)
require(world.get_path_name().startswith(MAP_PATH + "."), "verify editor world is %s (got %s)" % (MAP_PATH, world.get_path_name()))
cls = require(unreal.load_class(None, "/Script/UnderTheMountain.TreasureActor"), "load TreasureActor class")
# remove prior placements so re-runs stay deterministic
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("Treasure_"):
        require(eas.destroy_actor(a), "destroy prior placement %s" % a.get_actor_label())
PLACEMENTS = [
    ("Treasure_Goblet", "DA_Treasure_Goblet", (0, 900, 160)),
    ("Treasure_JewelledCup", "DA_Treasure_JewelledCup", (6000, -1000, 160)),
    ("Treasure_Chest", "DA_Treasure_Chest", (12000, 1000, 160)),
    ("Treasure_SeeingStone", "DA_Artifact_SeeingStone", (19000, 0, 300)),
]
for label, asset, loc in PLACEMENTS:
    a = require(eas.spawn_actor_from_class(cls, unreal.Vector(*loc)), "spawn %s" % label)
    a.set_actor_label(label)
    treasure_data = require(eal.load_asset(DIR + "/" + asset), "load treasure asset %s" % asset)
    a.set_editor_property("treasure_data", treasure_data)
    print("TREASURE_PLACED", label, loc)
require(les.save_current_level(), "save level %s" % MAP_PATH)
print("TREASURES_DONE")
