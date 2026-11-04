# [2026-11-04 T-114] import_and_place_nat.py — ONE editor-run script that
# [2026-11-04 T-114] (a) imports every nat_* FBX in Content/Meshy/World/
# [2026-11-04 T-114] that has no .uasset yet into /Game/Meshy/World/ via
# [2026-11-04 T-114] unreal.AssetImportTask (the import_world_assets.py
# [2026-11-04 T-114] pattern: automated, saved, skip-if-asset-exists, so
# [2026-11-04 T-114] re-runs and manual imports are safe), then (b) runs
# [2026-11-04 T-114] the idempotent NatFoliage_ placement copied from
# [2026-11-04 T-114] place_nat_foliage.py (destroy prior NatFoliage_
# [2026-11-04 T-114] actors, re-place from the fixed table, assert unique
# [2026-11-04 T-114] labels, save). Together they close the editor half
# [2026-11-04 T-114] of T-114 that place_nat_foliage.py's precondition
# [2026-11-04 T-114] left owed: nat-broadleaf / nat-bush / nat-grass have
# [2026-11-04 T-114] FBX+GLB+basecolor on disk but no .uasset yet.
# [2026-11-04 T-114] Grounding is the shared terrain_profile closed form
# [2026-11-04 T-114] (terrain_height, sunk 30 cm as in place_nat_foliage)
# [2026-11-04 T-114] and the foliage budget is the docs/ASSET-PIPELINE.md
# [2026-11-04 T-114] cap: <=5000 foliage props total, enforced as a hard
# [2026-11-04 T-114] stop while placing (this pass places 120).
# [2026-11-04 T-114] Grass/bush repeat heavily in real play: swap these
# [2026-11-04 T-114] StaticMeshActors for an InstancedStaticMesh pass
# [2026-11-04 T-114] before scaling counts up (noted, not faked).
# [2026-11-04 T-114] Out of scope by design: the goat. liv-goat_quad_v2
# [2026-11-04 T-114] is .blend-only (no FBX to import — see
# [2026-11-04 T-114] Scripts/inventory_world_drop.py) and the existing
# [2026-11-04 T-114] liv-goat / liv-goat_rigged .uassets already exist.
# [2026-11-04 T-114] EDITOR-ONLY (imports unreal): run inside the live
# [2026-11-04 T-114] editor's Python console with:
# [2026-11-04 T-114]   py "<project>/Scripts/import_and_place_nat.py"
# [2026-11-04 T-114] (or -run=pythonscript with the same path). Do NOT run
# [2026-11-04 T-114] it with system python3 — it will fail on `import
# [2026-11-04 T-114] unreal`, by design. Success prints one line:
# [2026-11-04 T-114]   IMPORT_AND_PLACE_NAT_DONE imported=<n> skipped_existing=<n> placed=<n> cap=5000
import os
import sys as _sys

import unreal

_sys.path.insert(0, os.path.join(unreal.Paths.project_dir(), "Scripts"))
import map_guard
from terrain_profile import terrain_height

MAP_PATH = "/Game/Maps/Lair/Lair"
GAME_DIR = "/Game/Meshy/World"
WORLD_DIR = unreal.Paths.project_dir() + "Content/Meshy/World/"
MAX_FOLIAGE_PROPS = 5000  # [2026-11-04 T-114] ASSET-PIPELINE cap, hard stop
# [2026-11-04 T-114] The three stems placement requires. Each must end the
# [2026-11-04 T-114] import phase with a loadable StaticMesh or this script
# [2026-11-04 T-114] fails loud (missing-source list) instead of placing
# [2026-11-04 T-114] a partial set.
PLACEMENT_ASSETS = ("nat-broadleaf", "nat-bush", "nat-grass")

eal = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def require(value, action):
    if not value:
        raise RuntimeError("import_and_place_nat failed: %s" % action)
    return value


# --- Phase 1: import nat_* FBX sources that have no .uasset yet --------
# [2026-11-04 T-114] Same commandlet warm-up as create_exterior.py: the
# [2026-11-04 T-114] Asset Registry is cold at script start and
# [2026-11-04 T-114] does_asset_exist / load_asset lie about on-disk
# [2026-11-04 T-114] assets until a synchronous scan.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Maps", "/Game/Meshy"], True, True)

fbx_files = sorted(
    f for f in os.listdir(WORLD_DIR)
    if f.lower().startswith("nat-") and f.lower().endswith(".fbx")
    and os.path.isfile(os.path.join(WORLD_DIR, f)))
print("NAT_FBX_COUNT", len(fbx_files), fbx_files)

missing_sources = []
for asset_name in PLACEMENT_ASSETS:
    if asset_name + ".fbx" not in fbx_files and not eal.does_asset_exist(
            GAME_DIR + "/" + asset_name):
        missing_sources.append(asset_name + ".fbx")
require(not missing_sources,
        "missing nat FBX source(s) in %s: %s" % (WORLD_DIR, missing_sources))

imported, skipped_existing = 0, 0
for filename in fbx_files:
    name = filename[:-4]
    dest = GAME_DIR + "/" + name
    if eal.does_asset_exist(dest):
        skipped_existing += 1
        print("NAT_IMPORT_SKIP existing", dest)
        continue
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", WORLD_DIR + filename)
    task.set_editor_property("destination_path", GAME_DIR)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    # [2026-11-04 T-114] Static foliage, not the liv-goat_rigged skeletal
    # [2026-11-04 T-114] special case in import_world_assets.py: import
    # [2026-11-04 T-114] the mesh with its materials/textures, no skeleton
    # [2026-11-04 T-114] and no animations.
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", True)
    task.set_editor_property("options", options)
    asset_tools.import_asset_tasks([task])
    # [2026-11-04 T-114] unreal.load_asset, not eal.load_asset: the latter
    # [2026-11-04 T-114] returns None for these dashed Meshy names despite
    # [2026-11-04 T-114] listing them (proven, create_exterior.py).
    asset = require(unreal.load_asset(dest),
                    "import %s produced no asset at %s" % (filename, dest))
    require(asset.get_class().get_name() == "StaticMesh",
            "import %s produced %s at %s, expected StaticMesh"
            % (filename, asset.get_class().get_name(), dest))
    print("NAT_IMPORTED", name, asset.get_class().get_name())
    imported += 1
print("NAT_IMPORT_DONE imported=%d skipped_existing=%d"
      % (imported, skipped_existing))

# [2026-11-04 T-114] Rescan so phase 2's load_asset calls see the assets
# [2026-11-04 T-114] phase 1 just saved to disk.
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Meshy"], True, True)
for asset_name in PLACEMENT_ASSETS:
    require(unreal.load_asset(GAME_DIR + "/" + asset_name),
            "load placement asset %s after import" % asset_name)

# --- Phase 2: idempotent NatFoliage_ placement (place_nat_foliage.py) --
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_guard.backup_level("natfoliage")
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
world = require(ues.get_editor_world(), "get editor world")
require(world.get_path_name().startswith(MAP_PATH + "."),
        "verify editor world is %s" % MAP_PATH)

# [2026-11-04 T-114] Idempotency, same shape as create_exterior.py: every
# [2026-11-04 T-114] prior NatFoliage_ actor is destroyed before placing.
map_guard.log_census(eas, ("NatFoliage_",))
for a in list(eas.get_all_level_actors()):
    if a.get_actor_label().startswith("NatFoliage_"):
        require(eas.destroy_actor(a), "destroy prior foliage %s" % a.get_actor_label())


def place(label, asset_name, x, y, target_h, yaw=0.0):
    mesh = require(unreal.load_asset("/Game/Meshy/World/" + asset_name), "load %s" % asset_name)
    bounds = mesh.get_bounds()
    height = max(1.0, 2.0 * bounds.box_extent.z)
    scale = target_h / height
    a = require(eas.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, terrain_height(x, y) - 30.0)), "spawn %s" % label)
    a.set_actor_label(label)
    a.static_mesh_component.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    if yaw:
        # [2026-11-04 T-114] Keyword Rotator args are load-bearing: this
        # [2026-11-04 T-114] Python build assigns Rotator positionally as
        # [2026-11-04 T-114] (roll, pitch, yaw) — see create_exterior.py.
        a.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False)
    return a


# [2026-11-04 T-114] Fixed deterministic table (no editor RNG): clusters
# [2026-11-04 T-114] on the valley/valley-village fringe + farmsteads from
# [2026-11-04 T-114] create_exterior.py, heights in cm, ground from the
# [2026-11-04 T-114] shared terrain_profile closed form.
SITES = ((50000.0, 0.0), (80000.0, 155000.0), (-65000.0, -185000.0),
         (125000.0, -145000.0), (-185000.0, -45000.0), (-155000.0, 55000.0))
placed = 0
for si, (cx, cy) in enumerate(SITES):
    for i in range(20):
        dx = ((i * 733) % 4000) - 2000
        dy = ((i * 389) % 4000) - 2000
        asset = ("nat-broadleaf", "nat-bush", "nat-grass")[i % 3]
        target_h = (900, 220, 120)[i % 3]
        place("NatFoliage_S%d_%02d" % (si, i), asset, cx + dx, cy + dy, target_h, yaw=float((i * 37) % 360))
        placed += 1
        require(placed <= MAX_FOLIAGE_PROPS, "foliage budget cap %d" % MAX_FOLIAGE_PROPS)
print("NAT_FOLIAGE_PLACED", placed)
map_guard.assert_unique_labels(eas, ("NatFoliage_",))
require(les.save_current_level(), "save level %s" % MAP_PATH)
print("IMPORT_AND_PLACE_NAT_DONE imported=%d skipped_existing=%d placed=%d cap=%d"
      % (imported, skipped_existing, placed, MAX_FOLIAGE_PROPS))
