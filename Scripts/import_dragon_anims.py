# [2026-11-01 T-068] Import every T-068 dragon clip FBX (create_dragon_anims.py
# [2026-11-01 T-068] output) onto SK_Dragon_v4_Skeleton, keeping only the
# [2026-11-01 T-068] AnimSequences as SK_Dragon_v4_<Clip>Anim. Generalises the
# [2026-11-01 T-068] T-043 import_flight_anim.py procedure (same staging dance,
# [2026-11-01 T-068] same suffix selection) to the full v1 set. Run in the
# [2026-11-01 T-068] editor:  -run=pythonscript -script=Scripts/import_dragon_anims.py
# [2026-11-01 T-068] FOOT-GUN (paid for 2026-11-01): replacing an AnimSequence
# [2026-11-01 T-068] the pawn CDO already references (FlyAnim) cannot be done
# [2026-11-01 T-068] by delete/rename from a commandlet — the loaded asset
# [2026-11-01 T-068] refuses both. Delete that .uasset from disk BEFORE
# [2026-11-01 T-068] launching the editor; the FBX is the source of truth.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/Meshy/Dragon"
STAGE = DST + "/AnimImport"

CLIPS = [
    ("dragon_v4_flight.fbx", "SK_Dragon_v4_FlyAnim"),
    ("dragon_v4_glide.fbx", "SK_Dragon_v4_GlideAnim"),
    ("dragon_v4_hover.fbx", "SK_Dragon_v4_HoverAnim"),
    ("dragon_v4_idle.fbx", "SK_Dragon_v4_IdleAnim"),
    ("dragon_v4_takeoff.fbx", "SK_Dragon_v4_TakeoffAnim"),
    ("dragon_v4_land.fbx", "SK_Dragon_v4_LandAnim"),
    ("dragon_v4_roar.fbx", "SK_Dragon_v4_RoarAnim"),
    ("dragon_v4_hit.fbx", "SK_Dragon_v4_HitAnim"),
]


def require(value, action):
    if not value:
        raise RuntimeError("import_dragon_anims failed: %s" % action)
    return value


skeleton = require(eal.load_asset(DST + "/SK_Dragon_v4_Skeleton"), "load SK_Dragon_v4_Skeleton")

for fbx_name, asset_name in CLIPS:
    dest = DST + "/" + asset_name
    if eal.does_asset_exist(dest):
        # [2026-11-01 T-068] A loaded CDO (the pawn constructor finds these
        # [2026-11-01 T-068] clips) can refuse an outright delete in a
        # [2026-11-01 T-068] commandlet; rename-aside always works, and the
        # [2026-11-01 T-068] trash copy is deleted best-effort at the end.
        if not eal.delete_asset(dest):
            trash = dest + "_Replaced"
            if eal.does_asset_exist(trash):
                eal.delete_asset(trash)
            require(eal.rename_asset(dest, trash), "rename existing %s aside" % dest)
            eal.delete_asset(trash)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", unreal.Paths.project_dir() + "Content/Meshy/Dragon/" + fbx_name)
    task.set_editor_property("destination_path", STAGE)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("skeleton", skeleton)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    anim_path = None
    for path in eal.list_assets(STAGE, recursive=False):
        if path.endswith("_Anim"):
            anim_path = path
    require(anim_path, "find AnimSequence for %s" % fbx_name)
    # [2026-11-01 T-068] The rename-aside above leaves a redirector at dest;
    # [2026-11-01 T-068] renaming onto a redirector fails, so clear it first.
    if eal.does_asset_exist(dest):
        eal.delete_asset(dest)
    require(eal.rename_asset(anim_path, dest), "rename %s -> %s" % (anim_path, dest))
    for path in list(eal.list_assets(STAGE, recursive=False)):
        require(eal.delete_asset(path), "delete staging asset %s" % path)
    anim = require(eal.load_asset(dest), "load imported %s" % dest)
    print("DRAGON_ANIM_IMPORTED", dest, anim.get_editor_property("sequence_length"))

print("DRAGON_ANIMS_IMPORTED_DONE")
