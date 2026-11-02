# [2026-11-01 T-016] Author DA_HoardThresholds, the section 8
# tuning asset UHoardSubsystem reads (win/lose fractions, final
# wave). Load-or-create like the other content scripts; values are
# the spec's (0.5 win, 0.2 lose, final wave 6) - retunes edit the
# asset, not code.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    if not value:
        raise RuntimeError("create_waves failed: %s" % action)
    return value


unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Gameplay"], True, True)

DIR = "/Game/Gameplay/Waves"
if not eal.does_directory_exist(DIR):
    require(eal.make_directory(DIR), "make directory %s" % DIR)
path = DIR + "/DA_HoardThresholds"
da = eal.load_asset(path) if eal.does_asset_exist(path) else None
if da is None:
    da = require(
        asset_tools.create_asset("DA_HoardThresholds", DIR, unreal.HoardThresholdsAsset,
                                 unreal.DataAssetFactory()),
        "create %s" % path)
da.set_editor_property("win_fraction", 0.5)
da.set_editor_property("lose_fraction", 0.2)
da.set_editor_property("final_wave", 6)
require(eal.save_asset(path), "save %s" % path)
print("HOARD_THRESHOLDS_DONE", path)
