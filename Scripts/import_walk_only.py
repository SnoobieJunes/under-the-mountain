# [2026-11-04 walk fix] Import ONLY the authored walk gait over the statue clip.
import os
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/Meshy/Dragon"
STAGE = DST + "/AnimImport"
FBX = unreal.Paths.project_dir() + "Content/Meshy/Dragon/dragon_v4_walk.fbx"
DEST = DST + "/SK_Dragon_v4_Anim"

def require(v, what):
    if not v:
        raise RuntimeError("WALK_IMPORT FAIL " + what)
    return v

unreal.log("WALK_IMPORT start exists=%s" % os.path.exists(FBX))
skeleton = require(eal.load_asset(DST + "/SK_Dragon_v6_Skeleton"), "skeleton")
task = unreal.AssetImportTask()
task.set_editor_property("filename", FBX)
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
    unreal.log("WALK_IMPORT staged " + str(path))
    if "_Anim" in str(path):
        anim_path = path
require(anim_path, "no _Anim staged")
anim_pkg = str(anim_path).split(".")[0]
if eal.does_asset_exist(DEST):
    if not eal.delete_asset(DEST):
        require(eal.rename_asset(DEST, DEST + "_Old"), "rename old aside")
        eal.delete_asset(DEST + "_Old")
require(eal.rename_asset(anim_pkg, DEST), "rename")
for path in list(eal.list_assets(STAGE, recursive=False)):
    eal.delete_asset(path)
anim = require(eal.load_asset(DEST), "load dest")
unreal.log("WALK_IMPORT DONE length=%s" % anim.get_editor_property("sequence_length"))
print("WALK_IMPORT DONE")
