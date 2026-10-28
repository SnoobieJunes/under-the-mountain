# [2026-10-27 T-003] Create the greybox lair map headlessly:
# UnrealEditor-Cmd <project> -run=pythonscript -script=create_greybox_lair.py
# Greybox only: one big floor, three chamber divider walls, PlayerStart,
# sun + sky light. T-012 replaces this dressing in the same map slot.
import unreal

MAP_PATH = "/Game/Maps/Lair/GreyboxLair"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
eal = unreal.EditorAssetLibrary


def require(value, action):
    # [2026-10-28 T-037] Fail before mutating anything when an Unreal
    # helper reports failure through False/None.
    if not value:
        raise RuntimeError("create_greybox_lair failed: %s" % action)
    return value


# This generator creates a fresh greybox only. Refuse to run over an
# existing map instead of spawning its boxes into whatever level is
# currently open.
if eal.does_asset_exist(MAP_PATH):
    raise RuntimeError("create_greybox_lair failed: %s already exists; refusing to overwrite it" % MAP_PATH)
require(les.new_level(MAP_PATH), "create level %s" % MAP_PATH)

cube = require(unreal.load_asset("/Engine/BasicShapes/Cube"), "load Cube mesh")

def box(name, loc, scale):
    actor = require(eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc)), "spawn %s" % name)
    actor.set_actor_label(name)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor

# Floor: 8000x8000, top surface at z=0.
box("GreyboxFloor", (0, 0, -50), (80, 80, 1))
# Chamber dividers (hoard room reads as three patrol spaces, T-012 expands).
box("ChamberWallA", (1500, 0, 250), (1, 60, 5))
box("ChamberWallB", (-1500, 0, 250), (1, 60, 5))
box("BackWall", (0, -2500, 250), (80, 1, 5))

start = require(eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 150)), "spawn PlayerStart")
start.set_actor_label("DragonStart")

sun = require(eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(500, -500, 900)), "spawn DirectionalLight")
sun.set_actor_label("Sun")
sun.set_actor_rotation(unreal.Rotator(-50, -40, 0), False)
sky = require(eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500)), "spawn SkyLight")
sky.set_actor_label("SkyLight")

require(les.save_current_level(), "save level %s" % MAP_PATH)
print("GREYBOX_LAIR_CREATED", MAP_PATH)
