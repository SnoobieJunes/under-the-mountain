# [2026-10-28 T-046] Batch-import every Content/Meshy/World/*.fbx with
# automated AssetImportTasks — no per-file FBX dialog. Meshes beside
# their FBX (skips any that already have a .uasset, so re-runs and
# the owner's manual imports are safe). Static props import as static
# meshes; liv-goat_rigged imports as a skeletal mesh with its anim.
import unreal

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    if not value:
        raise RuntimeError("import_world_assets failed: %s" % action)
    return value


WORLD_DIR = unreal.Paths.project_dir() + "Content/Meshy/World/"
GAME_DIR = "/Game/Meshy/World"

import os

fbx_files = sorted(f for f in os.listdir(WORLD_DIR) if f.lower().endswith(".fbx"))
print("WORLD_FBX_COUNT", len(fbx_files))
imported, skipped = 0, 0
for filename in fbx_files:
    name = filename[:-4]
    dest = GAME_DIR + "/" + name
    if eal.does_asset_exist(dest):
        skipped += 1
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", WORLD_DIR + filename)
    task.set_editor_property("destination_path", GAME_DIR)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    if name == "liv-goat_rigged":
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", True)
        options.set_editor_property("import_animations", True)
        task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])
    asset = eal.load_asset(dest)
    if not asset:
        raise RuntimeError("import_world_assets failed: %s produced no asset at %s" % (filename, dest))
    print("IMPORTED", name, asset.get_class().get_name())
    imported += 1
print("WORLD_IMPORT_DONE imported=%d skipped=%d" % (imported, skipped))
