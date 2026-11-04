# [2026-11-04 T-113] Import the newest owner-drop clips (Content/Meshy/
# [2026-11-04 T-113] dragon-wings-v6(new)/exports/animations/Anim_Dragon_*.fbx)
# [2026-11-04 T-113] onto SK_Dragon_v6_Skeleton at the pawn's nine clip paths,
# [2026-11-04 T-113] so the T-110 skeleton gate self-heals with no pawn change.
# [2026-11-04 T-113] Inventory behind this (Blender 5.2 headless, 2026-11-04):
# [2026-11-04 T-113] newest Dragon_v6.fbx == T-110 source bit-for-bit in what
# [2026-11-04 T-113] matters (289,485 verts / 413,454 tris / identical 39-bone
# [2026-11-04 T-113] DragonArmature; all 9 anim FBX armatures match exactly;
# [2026-11-04 T-113] 6 PNGs md5-identical to the T-110 imports) — so the mesh,
# [2026-11-04 T-113] skeleton, materials and textures stay as T-110 shipped
# [2026-11-04 T-113] them; only these 9 clips are new. Generalises the T-068
# [2026-11-04 T-113] import_dragon_anims.py procedure (same staging dance,
# [2026-11-04 T-113] same suffix selection, same loaded-CDO foot-gun rule:
# [2026-11-04 T-113] the 9 SK_Dragon_v4_*Anim.uassets are git-rm'd from disk
# [2026-11-04 T-113] BEFORE the editor launches; the FBX is the source of
# [2026-11-04 T-113] truth). Run headless: "$UE_EDITOR_CMD" "$UPROJECT"
# [2026-11-04 T-113] -run=pythonscript -script="<repo>/Scripts/
# [2026-11-04 T-113] import_newest_dragon_anims.py" -Unattended -NullRHI -NoSplash
# [2026-11-04 T-113] Exit 0 + NEWEST_DRAGON_ANIMS_DONE iff all 9 pawn paths
# [2026-11-04 T-113] hold an AnimSequence on the v6 skeleton with length > 0.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/Meshy/Dragon"
STAGE = DST + "/AnimImport"
# [2026-11-04 T-113] SOURCE NOTE: the owner drop's Anim_Dragon_*.fbx are
# [2026-11-04 T-113] animation-only (armature + action, no mesh — proven in
# [2026-11-04 T-113] Blender). UE5's Interchange OverridePipeline imports
# [2026-11-04 T-113] nothing from a mesh-less FBX even with import_animations
# [2026-11-04 T-113] set ("nothing to import", proven 2026-11-04, two attempts:
# [2026-11-04 T-113] with and without import_mesh). So this script reads the
# [2026-11-04 T-113] mesh-merged copies produced by Scripts/
# [2026-11-04 T-113] merge_newest_dragon_anim_mesh.py (Saved/T113AnimMerge/,
# [2026-11-04 T-113] gitignored ephemeral: mesh re-pointed at the animated
# [2026-11-04 T-113] armature, name-keyed weights intact, originals in
# [2026-11-04 T-113] dragon-wings-v6(new)/ never touched) and follows the
# [2026-11-04 T-113] proven T-043/T-068 mesh+anim staging dance exactly.
SRC = unreal.Paths.project_dir() + "Saved/T113AnimMerge/"

# [2026-11-04 T-113] Newest-drop file -> pawn clip path (the T-110 gate
# [2026-11-04 T-113] watches these paths; names stay v4-legacy so no pawn
# [2026-11-04 T-113] change is needed — the content is the newest rig's).
CLIPS = [
    ("Anim_Dragon_Walk.fbx", "SK_Dragon_v4_Anim"),
    ("Anim_Dragon_Fly.fbx", "SK_Dragon_v4_FlyAnim"),
    ("Anim_Dragon_Glide.fbx", "SK_Dragon_v4_GlideAnim"),
    ("Anim_Dragon_Hover.fbx", "SK_Dragon_v4_HoverAnim"),
    ("Anim_Dragon_Idle.fbx", "SK_Dragon_v4_IdleAnim"),
    ("Anim_Dragon_Takeoff.fbx", "SK_Dragon_v4_TakeoffAnim"),
    ("Anim_Dragon_Land.fbx", "SK_Dragon_v4_LandAnim"),
    ("Anim_Dragon_Roar.fbx", "SK_Dragon_v4_RoarAnim"),
    ("Anim_Dragon_HitLight.fbx", "SK_Dragon_v4_HitAnim"),
]


def require(value, action):
    # [2026-10-28 T-037] Surface Unreal's False/None helper failures
    # as a named exception before destructive work or success text.
    if not value:
        raise RuntimeError("import_newest_dragon_anims failed: %s" % action)
    return value


skeleton = require(eal.load_asset(DST + "/SK_Dragon_v6_Skeleton"),
                   "load SK_Dragon_v6_Skeleton (T-113: newest anims target the v6 rig)")

for fbx_name, asset_name in CLIPS:
    dest = DST + "/" + asset_name
    if eal.does_asset_exist(dest):
        # [2026-11-01 T-068] A loaded CDO (the pawn constructor finds these
        # [2026-11-01 T-068] clips) can refuse an outright delete in a
        # [2026-11-01 T-068] commandlet; rename-aside always works, and the
        # [2026-11-01 T-068] trash copy is deleted best-effort at the end.
        # [2026-11-04 T-113] Normal path is dest absent (git-rm'd pre-launch);
        # [2026-11-04 T-113] this branch is the belt-and-braces fallback.
        if not eal.delete_asset(dest):
            trash = dest + "_Replaced"
            if eal.does_asset_exist(trash):
                eal.delete_asset(trash)
            require(eal.rename_asset(dest, trash), "rename existing %s aside" % dest)
            eal.delete_asset(trash)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SRC + fbx_name)
    task.set_editor_property("destination_path", STAGE)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    # [2026-11-04 T-113] Mesh+anim import (the T-043/T-068 shape): the merged
    # [2026-11-04 T-113] FBXs carry the mesh, so the mesh pipeline runs and the
    # [2026-11-04 T-113] animation lands on the v6 skeleton; the staging mesh
    # [2026-11-04 T-113] is deleted after the *_Anim rename, as before.
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("skeleton", skeleton)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])

    anim_path = None
    for path in eal.list_assets(STAGE, recursive=False):
        # [2026-11-04 T-113] Suffix selection (the T-043/T-068 shape) first:
        # [2026-11-04 T-113] a mesh-carrying import names the clip *_Anim. An
        # [2026-11-04 T-113] animation-only import may instead keep the action
        # [2026-11-04 T-113] name, so fall back to class selection via the
        # [2026-11-04 T-113] registry (no in-process load needed — the T-043
        # [2026-11-04 T-113] load_asset-returns-None foot-gun).
        if path.endswith("_Anim"):
            anim_path = path
            break
    if anim_path is None:
        registry = unreal.AssetRegistryHelpers.get_asset_registry()
        for path in eal.list_assets(STAGE, recursive=False):
            data = registry.get_asset_by_object_path(path)
            if data and data.asset_class_path.asset_name.to_string() == "AnimSequence":
                anim_path = path
                break
    require(anim_path, "find AnimSequence for %s" % fbx_name)
    # [2026-11-01 T-068] The rename-aside above leaves a redirector at dest;
    # [2026-11-01 T-068] renaming onto a redirector fails, so clear it first.
    if eal.does_asset_exist(dest):
        eal.delete_asset(dest)
    require(eal.rename_asset(anim_path, dest), "rename %s -> %s" % (anim_path, dest))
    for path in list(eal.list_assets(STAGE, recursive=False)):
        require(eal.delete_asset(path), "delete staging asset %s" % path)
    anim = require(eal.load_asset(dest), "load imported %s" % dest)
    length = anim.get_editor_property("sequence_length")
    require(length > 0, "non-empty %s (got %s)" % (dest, length))
    require(anim.get_editor_property("skeleton") == skeleton,
            "%s on the v6 skeleton" % dest)
    print("NEWEST_DRAGON_ANIM_IMPORTED", dest, length)

print("NEWEST_DRAGON_ANIMS_DONE")
