# [2026-10-28 T-013] Author Content/Gameplay/Treasures/: treasure DataAssets,
# the true-sight material, and place treasure actors in Lair.umap.
# [2026-10-28 T-041] Scent colour is not a material here any more: it
# lives in NS_ScentRibbon (see the make_material calls below).
# Values are docs/GAMEPLAY.md section 3/6 examples; brackets do the rest.
import unreal

DIR = "/Game/Gameplay/Treasures"
eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

# [2026-11-01 T-082] In a -run=pythonscript commandlet the Asset
# Registry has not scanned yet when this script starts, so
# does_asset_exist/load_asset lie (False/None) about assets plainly
# on disk - the original script only ever ran inside a live editor,
# where the registry was warm. Scan the two trees this script
# touches, synchronously, before trusting either call.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Gameplay", "/Game/Maps"], True, True)


def require(value, action):
    # [2026-10-28 T-037] Convert Unreal's False/None helper failures
    # into a raised, named failure before any success text prints.
    if not value:
        raise RuntimeError("create_treasures failed: %s" % action)
    return value


def make_material(name, color, emissive_scale, disable_depth=False, translucent=False):
    path = DIR + "/" + name
    # [2026-11-01 T-082] Same re-run trap as the DataAssets below:
    # an existing material is returned as-is (its expressions were
    # built by the first run to this same spec) instead of dying on
    # delete-then-create inside a commandlet.
    if eal.does_asset_exist(path):
        return require(eal.load_asset(path), "load existing material %s" % path)
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

# [2026-10-28 T-041] No M_ScentGold: scent colour is baked into
# NS_ScentRibbon (Niagara), so nothing references that material. Any
# existing M_ScentGold.uasset on disk is left untouched by this script.
make_material("M_TrueSight", (0.46, 0.68, 0.64), True, disable_depth=True)

TREASURES = [
    ("DA_Treasure_Goblet", "Gold Goblet", 10, False),
    ("DA_Treasure_Coins", "Coin Pile", 15, False),
    ("DA_Treasure_JewelledCup", "Jewelled Cup", 50, False),
    ("DA_Treasure_GildedWeapon", "Gilded Weapon", 75, False),
    ("DA_Treasure_Chest", "Adventurer Chest", 250, False),
    ("DA_Treasure_PrizeChest", "Prize Chest", 350, False),
    ("DA_Artifact_SeeingStone", "Seeing Stone", 1200, True),
]
for name, display, value, stone in TREASURES:
    path = DIR + "/" + name
    # [2026-11-01 T-082] Load-or-create: the original delete-then-create
    # only worked on a virgin Content tree - delete_asset leaves the
    # package in memory in a commandlet run, so re-runs died on
    # "already exists". Updating the loaded asset in place is also the
    # safer retune path: actor references to the DA survive.
    da = eal.load_asset(path) if eal.does_asset_exist(path) else None
    if da is None:
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
# [2026-11-01 T-082] Bulk fill to the GAMEPLAY.md section 8 total:
# 1800 gold of Trinket-Piece-Prize treasure (each item <=399; the
# Seeing Stone is an artifact and never counts as bulk). Before this
# the lair held 310 bulk. The plan below spreads denominations over
# all four chambers so scent ribbon widths (bracketed by value) vary
# in every room, with the hoard richest. The script asserts its own
# sum and bracket spread before placing anything - a retune that
# breaks the section 8 total fails here, not in a hand count.
# Coordinates are zone centres in the CURRENT mountain topology
# (chamber 9500, hoard mound 12500, west cave 2500, east portal 16800);
# T-066's topology rebuild re-ran this script with new centres (was the
# T-012 chain C1 0, C2 6000, C3 12000, hoard 19000). The denominations
# and the 1800 total carry over unchanged.
VALUES = {name: value for name, _display, value, _stone in TREASURES}
ZONE_SLOTS = {
    # Hoard ring: on/around the mound (centre 12500,0), all 1900+ out.
    "Hoard": [(1800, -1200), (2200, 800), (-2000, 1500), (2000, -1600),
              (-1800, -1400), (1500, 1800)],
    # Chamber ring: clear of the 12 m touchdown disc AND the hoard mound.
    "Chamber": [(-3000, 2000), (3000, -2000), (-2500, -2500), (2500, 2500),
                (0, 3000), (0, -3000), (-3500, 0)],
    # Caves: along the axis, inside the 500-wide west cave / 800 tunnel.
    "WestCave": [(-800, 0), (800, 0)],
    "EastPortal": [(-1000, 200), (1000, -200)],
}
CHAMBERS = [("Hoard", 12500), ("Chamber", 9500), ("WestCave", 2500), ("EastPortal", 16800)]
PLAN = [
    # (zone, asset, extra placements beyond the four originals below)
    ("Hoard", "DA_Treasure_Goblet", 1), ("Hoard", "DA_Treasure_Coins", 2),
    ("Hoard", "DA_Treasure_JewelledCup", 3), ("Hoard", "DA_Treasure_GildedWeapon", 2),
    ("Hoard", "DA_Treasure_Chest", 1), ("Hoard", "DA_Treasure_PrizeChest", 1),
    ("Chamber", "DA_Treasure_Goblet", 3), ("Chamber", "DA_Treasure_Coins", 3),
    ("Chamber", "DA_Treasure_JewelledCup", 3), ("Chamber", "DA_Treasure_GildedWeapon", 1),
    ("WestCave", "DA_Treasure_Goblet", 2), ("WestCave", "DA_Treasure_Coins", 2),
    ("WestCave", "DA_Treasure_JewelledCup", 1),
    ("EastPortal", "DA_Treasure_Goblet", 1), ("EastPortal", "DA_Treasure_Coins", 1),
    ("EastPortal", "DA_Treasure_JewelledCup", 1), ("EastPortal", "DA_Treasure_GildedWeapon", 1),
]
PLACEMENTS = [
    ("Treasure_Goblet", "DA_Treasure_Goblet", (7000, 2500, 160)),
    ("Treasure_JewelledCup", "DA_Treasure_JewelledCup", (11500, 2500, 160)),
    ("Treasure_Chest", "DA_Treasure_Chest", (12500, 500, 280)),
    ("Treasure_SeeingStone", "DA_Artifact_SeeingStone", (12900, -100, 300)),
]
_centres = dict(CHAMBERS)
_slot_use = {name: 0 for name, _cx in CHAMBERS}
_counters = {}
for chamber, asset, count in PLAN:
    cx = _centres[chamber]
    slots = ZONE_SLOTS[chamber]
    for _ in range(count):
        dx, dy = slots[_slot_use[chamber] % len(slots)]
        _slot_use[chamber] += 1
        _counters[asset] = _counters.get(asset, 0) + 1
        label = "Treasure_%s_%02d" % (asset.replace("DA_Treasure_", "").replace("DA_Artifact_", ""),
                                      _counters[asset])
        PLACEMENTS.append((label, asset, (cx + dx, dy, 160)))
_bulk_total = sum(VALUES[asset] for _label, asset, _loc in PLACEMENTS
                  if VALUES[asset] <= 399)
_brackets = {"Trinket(1-24)": 0, "Piece(25-99)": 0, "Prize(100-399)": 0}
for _label, asset, _loc in PLACEMENTS:
    v = VALUES[asset]
    if v <= 24:
        _brackets["Trinket(1-24)"] += 1
    elif v <= 99:
        _brackets["Piece(25-99)"] += 1
    elif v <= 399:
        _brackets["Prize(100-399)"] += 1
require(_bulk_total == 1800, "bulk total is %d, section 8 wants 1800" % _bulk_total)
require(all(n > 0 for n in _brackets.values()), "a bulk bracket is empty: %s" % _brackets)
print("TREASURE_BULK_TOTAL", _bulk_total, "placements", len(PLACEMENTS), "brackets", _brackets)
for label, asset, loc in PLACEMENTS:
    a = require(eas.spawn_actor_from_class(cls, unreal.Vector(*loc)), "spawn %s" % label)
    a.set_actor_label(label)
    treasure_data = require(eal.load_asset(DIR + "/" + asset), "load treasure asset %s" % asset)
    a.set_editor_property("treasure_data", treasure_data)
    print("TREASURE_PLACED", label, loc)
require(les.save_current_level(), "save level %s" % MAP_PATH)
print("TREASURES_DONE")
