# [2026-11-01 T-068] Import dragon_v5_mesh.fbx (Scripts/lengthen_wings.py
# [2026-11-01 T-068] output: lengthened wing bones + moved membrane geometry)
# [2026-11-01 T-068] as the dragon skeletal mesh, replacing SK_Dragon_v4 +
# [2026-11-01 T-068] SK_Dragon_v4_Skeleton + SK_Dragon_v4_Anim (Walk) at the
# [2026-11-01 T-068] SAME paths so pawn/tests keep resolving. The old uassets
# [2026-11-01 T-068] must be deleted from disk before launch (loaded-CDO
# [2026-11-01 T-068] foot-gun, see import_dragon_anims.py header). The mesh
# [2026-11-01 T-068] material slot is re-pointed at M_DragonScales so
# [2026-11-01 T-068] tint/textures survive the mesh replacement.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
DST = "/Game/Meshy/Dragon"
STAGE = DST + "/MeshImport"


def require(value, action):
    if not value:
        raise RuntimeError("import_dragon_mesh failed: %s" % action)
    return value


for old in (DST + "/SK_Dragon_v4", DST + "/SK_Dragon_v4_Skeleton", DST + "/SK_Dragon_v4_Anim"):
    if eal.does_asset_exist(old):
        raise RuntimeError("import_dragon_mesh: %s still exists - delete it from disk before launch" % old)

task = unreal.AssetImportTask()
task.set_editor_property("filename", unreal.Paths.project_dir() + "Content/Meshy/Dragon/dragon_v5_mesh.fbx")
task.set_editor_property("destination_path", STAGE)
task.set_editor_property("automated", True)
task.set_editor_property("save", True)
options = unreal.FbxImportUI()
options.set_editor_property("import_mesh", True)
options.set_editor_property("import_as_skeletal", True)
options.set_editor_property("import_animations", True)
options.set_editor_property("import_materials", False)
options.set_editor_property("import_textures", False)
task.set_editor_property("options", options)
asset_tools.import_asset_tasks([task])

staged = list(eal.list_assets(STAGE, recursive=False))
print("STAGED_MESH", staged)
renames = []
for path in staged:
    if path.endswith("_Skeleton"):
        renames.append((path, DST + "/SK_Dragon_v4_Skeleton"))
    elif path.endswith("_Anim"):
        renames.append((path, DST + "/SK_Dragon_v4_Anim"))
    elif path.endswith("_PhysicsAsset"):
        renames.append((path, None))
    else:
        asset = eal.load_asset(path)
        if asset and asset.get_class().get_name() == "SkeletalMesh":
            renames.append((path, DST + "/SK_Dragon_v4"))
        else:
            renames.append((path, None))
require(any(dst == DST + "/SK_Dragon_v4" for _, dst in renames), "find staged SkeletalMesh")
require(any(dst == DST + "/SK_Dragon_v4_Skeleton" for _, dst in renames), "find staged Skeleton")

# skeleton first so the mesh rename never points at a vanished path
for src, dst in sorted(renames, key=lambda r: 0 if r[1] and r[1].endswith("Skeleton") else 1):
    if dst is None:
        require(eal.delete_asset(src), "delete staged %s" % src)
    else:
        require(eal.rename_asset(src, dst), "rename %s -> %s" % (src, dst))

# [2026-11-01 T-068] A fresh worktree's registry may not have scanned
# [2026-11-01 T-068] pre-existing assets (materials) in a commandlet run;
# [2026-11-01 T-068] force the scan or load_asset returns None for them.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([DST], force_rescan=True)
mesh = require(eal.load_asset(DST + "/SK_Dragon_v4"), "load SK_Dragon_v4")
# [2026-11-01 T-068] M_DragonScales (Scripts/create_dragon_material.py)
# [2026-11-01 T-068] owns textures + the ScaleTint parameter.
material = eal.load_asset(DST + "/M_DragonScales.M_DragonScales")
if material is not None:
    # [2026-11-01 T-068] Mutating fetched SkeletalMaterial structs does NOT
    # [2026-11-01 T-068] persist (proven: slot stayed WorldGridMaterial and
    # [2026-11-01 T-068] the dragon rendered untextured in-game). Construct a
    # [2026-11-01 T-068] fresh slot, replace the array, save_loaded_asset.
    slot = unreal.SkeletalMaterial()
    slot.set_editor_property("material_interface", material)
    mesh.set_editor_property("materials", [slot])
    require(eal.save_loaded_asset(mesh), "save SK_Dragon_v4 with material")
    print("DRAGON_MATERIAL_ASSIGNED M_DragonScales slots=1")
else:
    raise RuntimeError("import_dragon_mesh failed: M_DragonScales not found - run create_dragon_material.py first")

skeleton = require(eal.load_asset(DST + "/SK_Dragon_v4_Skeleton"), "load skeleton")
print("DRAGON_MESH_IMPORTED skeleton=%s" % skeleton.get_name())
print("DRAGON_MESH_IMPORT_DONE")
