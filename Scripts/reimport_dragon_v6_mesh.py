# [2026-11-04 T-123] In-place mesh reimport of the repaired v6 dragon.
#
# Scripts/repair_dragon_v6.py wrote the decimated repaired mesh to
# dragon_v6_game.fbx (same 39 bones, same 2 material slots, new
# topology). This script replaces SK_Dragon_v6's geometry IN PLACE
# (bReplaceExisting at the same path — the pawn CDO, the v6 skeleton,
# the 6 textures, the 2 materials and all 9 clips are untouched and
# keep pointing at the same asset), then verifies material slots,
# skeleton identity and clip compatibility (length > 0 on v6).
# Exit 0 + T123_REIMPORT_DONE iff all verifications pass.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/Meshy/Dragon"
MESH = DST + "/SK_Dragon_v6"
SKELETON = DST + "/SK_Dragon_v6_Skeleton"
CLIPS = [
    "SK_Dragon_v4_Anim", "SK_Dragon_v4_FlyAnim", "SK_Dragon_v4_GlideAnim",
    "SK_Dragon_v4_HoverAnim", "SK_Dragon_v4_IdleAnim", "SK_Dragon_v4_TakeoffAnim",
    "SK_Dragon_v4_LandAnim", "SK_Dragon_v4_RoarAnim", "SK_Dragon_v4_HitAnim",
]


def require(value, action):
    if not value:
        raise RuntimeError("reimport_dragon_v6_mesh failed: %s" % action)
    return value


def slot_material_names(mesh):
    return sorted(
        s.get_editor_property("material_interface").get_name()
        for s in mesh.get_editor_property("materials"))


mesh_before = require(eal.load_asset(MESH), "load SK_Dragon_v6")
skeleton = require(eal.load_asset(SKELETON), "load SK_Dragon_v6_Skeleton")
mats_before = slot_material_names(mesh_before)
print("T123_MATS_BEFORE", mats_before)
require(sorted(mats_before) == ["M_DragonScalesV6", "M_DragonWingV6"],
        "v6 materials assigned before reimport (got %s)" % mats_before)

task = unreal.AssetImportTask()
task.set_editor_property(
    "filename", unreal.Paths.project_dir() + "Content/Meshy/Dragon/dragon_v6_game.fbx")
task.set_editor_property("destination_path", DST)
task.set_editor_property("destination_name", "SK_Dragon_v6")
task.set_editor_property("replace_existing", True)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", False)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
options.set_editor_property("skeleton", skeleton)
task.set_editor_property("options", options)
asset_tools.import_asset_tasks([task])

mesh = require(eal.load_asset(MESH), "reload SK_Dragon_v6 after reimport")
mats_after = slot_material_names(mesh)
print("T123_MATS_AFTER", mats_after)
if mats_after != mats_before:
    # Geometry replace dropped the assignment: re-apply by slot name
    # (the T-110 slot-mapping shape), loudly.
    print("T123_MATS_REAPPLY assignments reset by reimport - re-applying")
    mat_body = require(eal.load_asset(DST + "/M_DragonScalesV6"), "load body material")
    mat_wing = require(eal.load_asset(DST + "/M_DragonWingV6"), "load wing material")
    new_slots = []
    for slot in mesh.get_editor_property("materials"):
        slot_name = str(slot.get_editor_property("material_slot_name"))
        picked = mat_wing if "wing" in slot_name.lower() else mat_body
        fresh = unreal.SkeletalMaterial()
        fresh.set_editor_property("material_interface", picked)
        new_slots.append(fresh)
        print("T123_SLOTMAP %s -> %s" % (slot_name, picked.get_name()))
    mesh.set_editor_property("materials", new_slots)
    require(eal.save_loaded_asset(mesh), "save SK_Dragon_v6 with materials")
    mats_after = slot_material_names(mesh)
require(mats_after == mats_before, "material assignments preserved (got %s)" % mats_after)

fresh_mesh = require(eal.load_asset(MESH), "reload for skeleton check")
require(fresh_mesh.get_editor_property("skeleton") == skeleton, "mesh still on the v6 skeleton")
for clip in CLIPS:
    seq = require(eal.load_asset(DST + "/" + clip), "clip %s survives" % clip)
    require(seq.get_editor_property("skeleton") == skeleton, "%s still on v6" % clip)
    require(seq.get_editor_property("sequence_length") > 0.0, "%s length > 0" % clip)
print("T123_CLIPS_OK 9/9 on SK_Dragon_v6_Skeleton")
print("T123_REIMPORT_DONE")
