# [2026-10-28 T-043] Import dragon_v4_flight.fbx the way the walk cycle
# was imported (plain FBX import), then keep only the AnimSequence:
# move it to /Game/Meshy/Dragon/SK_Dragon_v4_FlyAnim and delete the
# staging skeletal mesh the import also creates.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    if not value:
        raise RuntimeError("import_flight_anim failed: %s" % action)
    return value


SRC = unreal.Paths.project_dir() + "Content/Meshy/Dragon/dragon_v4_flight.fbx"
DST = "/Game/Meshy/Dragon"
STAGE = DST + "/FlightImport"
NAME = "SK_Dragon_v4_FlyAnim"
dest = DST + "/" + NAME

if eal.does_asset_exist(dest):
    require(eal.delete_asset(dest), "delete existing %s" % dest)
# The failed factory/FbxImportUI attempts left a StaticMesh here.
if eal.does_asset_exist(dest):
    raise RuntimeError("import_flight_anim failed: %s still exists after delete" % dest)

task = unreal.AssetImportTask()
task.set_editor_property("filename", SRC)
task.set_editor_property("destination_path", STAGE)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
# [2026-10-28 T-043] Automated FBX import skips animations unless
# told: import the mesh as skeletal WITH animations, onto the
# existing SK_Dragon_v4 skeleton (staging mesh is deleted after).
skeleton = require(eal.load_asset(DST + "/SK_Dragon_v4_Skeleton"), "load SK_Dragon_v4_Skeleton")
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", True)
options.set_editor_property("skeleton", skeleton)
task.set_editor_property("options", options)
asset_tools.import_asset_tasks([task])

staged = eal.list_assets(STAGE, recursive=False)
print("STAGED", staged)
anim_path = None
for path in staged:
    # [2026-10-28 T-043] Select by suffix: right after the import
    # task, load_asset on the fresh AnimSequence can still return
    # None in-process (proven), while the path is already listed.
    if path.endswith("_Anim"):
        anim_path = path
require(anim_path, "find AnimSequence in staging import")
anim_probe = eal.load_asset(anim_path)
print("STAGED_ANIM", anim_path, anim_probe.get_class().get_name() if anim_probe else None)

require(eal.rename_asset(anim_path, dest), "rename %s -> %s" % (anim_path, dest))
for path in list(eal.list_assets(STAGE, recursive=False)):
    require(eal.delete_asset(path), "delete staging asset %s" % path)

anim = require(eal.load_asset(dest), "load imported %s" % dest)
print("FLY_ANIM", dest, anim.get_class().get_name(), anim.get_editor_property("sequence_length"))
print("FLIGHT_ANIM_DONE")
