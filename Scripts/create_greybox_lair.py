# [2026-10-27 T-003] Create the greybox lair map headlessly:
# UnrealEditor-Cmd <project> -run=pythonscript -script=create_greybox_lair.py
# Greybox only: one big floor, three chamber divider walls, PlayerStart,
# sun + sky light. T-012 replaces this dressing in the same map slot.
import unreal

MAP_PATH = "/Game/Maps/Lair/GreyboxLair"

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.new_level(MAP_PATH)

cube = unreal.load_asset("/Engine/BasicShapes/Cube")

def box(name, loc, scale):
    actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc))
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

start = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 150))
start.set_actor_label("DragonStart")

sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(500, -500, 900))
sun.set_actor_label("Sun")
sun.set_actor_rotation(unreal.Rotator(-50, -40, 0), False)
sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
sky.set_actor_label("SkyLight")

les.save_current_level()
print("GREYBOX_LAIR_CREATED", MAP_PATH)
