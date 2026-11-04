# [2026-11-04 T-114] place_nat_foliage.py — idempotent nat_* foliage
# [2026-11-04 T-114] placement for Lair.umap, following create_exterior.py's
# [2026-11-04 T-114] pattern exactly: destroy every prior NatFoliage_ actor,
# [2026-11-04 T-114] re-place from a fixed table, assert unique labels, save.
# [2026-11-04 T-114] EDITOR-ONLY (imports unreal): run inside the editor
# [2026-11-04 T-114] python console / -run=pythonscript. NOT run headlessly.
# [2026-11-04 T-114] Budget: docs/ASSET-PIPELINE.md cap is <=5000 foliage
# [2026-11-04 T-114] props total (not 30k); this pass places 120 broadleaf/
# [2026-11-04 T-114] bush/grass actors and refuses to exceed the cap.
# [2026-11-04 T-114] Grass/bush repeat heavily in real play: swap these
# [2026-11-04 T-114] StaticMeshActors for an InstancedStaticMesh pass in the
# [2026-11-04 T-114] editor lane before scaling counts up (noted, not faked).
# [2026-11-04 T-114] PRECONDITION (inventory 2026-11-04): nat-broadleaf /
# [2026-11-04 T-114] nat-bush / nat-grass have FBX+GLB+basecolor on disk but
# [2026-11-04 T-114] NO .uasset yet — import them first (editor lane) or the
# [2026-11-04 T-114] require() below fails loud. Same for the goat drop:
# [2026-11-04 T-114] liv-goat_quad_v2 is .blend-only (no FBX to import).
import os as _os
import sys as _sys

import unreal

_sys.path.insert(0, _os.path.join(unreal.Paths.project_dir(), "Scripts"))
import map_guard
from terrain_profile import terrain_height

MAP_PATH = "/Game/Maps/Lair/Lair"
MAX_FOLIAGE_PROPS = 5000  # [2026-11-04 T-114] ASSET-PIPELINE cap, hard stop
eal = unreal.EditorAssetLibrary
unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(
    ["/Game/Maps", "/Game/Meshy"], True, True)


def require(value, action):
    if not value:
        raise RuntimeError("place_nat_foliage failed: %s" % action)
    return value


les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_guard.backup_level("natfoliage")
require(les.load_level(MAP_PATH), "load level %s" % MAP_PATH)

# [2026-11-04 T-114] Idempotency, same shape as create_exterior.py:49-50.
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
        a.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False)
    return a


# [2026-11-04 T-114] Fixed deterministic table (no editor RNG): clusters on
# [2026-11-04 T-114] the valley/valley-village fringe + farmsteads from
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
print("NAT_FOLIAGE_DONE")
