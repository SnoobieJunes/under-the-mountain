# [2026-11-04 T-139] Create ABP_Dragon in the editor: C++ parent
# UDragonAnimInstance on the v6 skeleton, per docs/ABP-DRAGON-BUILDSHEET.md S0.
# Run: UnrealEditor-Cmd <uproject> -run=pythonscript -script=<this file>
# Graph authoring is NOT attempted here (T-125 proved node placement needs
# the live editor); this script only creates + parents + saves the asset.
import unreal

PATH = "/Game/Meshy/Dragon"
NAME = "ABP_Dragon"
FULL = PATH + "/" + NAME

unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Meshy"], True, True)

if unreal.EditorAssetLibrary.does_asset_exist(FULL):
    print("ABP_CREATE EXISTS " + FULL)
    raise SystemExit(0)

skeleton = unreal.EditorAssetLibrary.load_asset(PATH + "/SK_Dragon_v6_Skeleton")
if skeleton is None:
    raise RuntimeError("ABP_CREATE FAIL: SK_Dragon_v6_Skeleton not found")

parent = unreal.load_class(None, "/Script/UnderTheMountain.DragonAnimInstance")
if parent is None:
    raise RuntimeError("ABP_CREATE FAIL: DragonAnimInstance class not found")

factory = unreal.AnimBlueprintFactory()
factory.set_editor_property("target_skeleton", skeleton)
factory.set_editor_property("parent_class", parent)

tools = unreal.AssetToolsHelpers.get_asset_tools()
asset = tools.create_asset(NAME, PATH, unreal.AnimBlueprint, factory)
if asset is None:
    raise RuntimeError("ABP_CREATE FAIL: create_asset returned None")

unreal.EditorAssetLibrary.save_asset(FULL, only_if_is_dirty=False)
print("ABP_CREATE DONE " + FULL + " parent=" + str(asset.get_class().get_name()))
