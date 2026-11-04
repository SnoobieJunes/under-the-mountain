import bpy
import os
import sys

REPO = "/Users/auston/Development Projects/under-the-mountain"
MESH_FBX = os.path.join(REPO, "Content/Meshy/dragon-wings-v6(new)/exports/Dragon_v6.fbx")
ANIM_DIR = os.path.join(REPO, "Content/Meshy/dragon-wings-v6(new)/exports/animations")
OUT_DIR = os.path.join(REPO, "Saved/T113AnimMerge")

os.makedirs(OUT_DIR, exist_ok=True)
anims = sorted(f for f in os.listdir(ANIM_DIR) if f.endswith(".fbx"))


def fail(msg):
    print("T113_MERGE_FAIL: %s" % msg)
    sys.exit(1)


for anim_file in anims:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.fbx(filepath=MESH_FBX)
    meshes = [o for o in bpy.data.objects if o.type == "MESH"]
    mesh_arms = [o for o in bpy.data.objects if o.type == "ARMATURE"]
    if len(meshes) != 1 or len(mesh_arms) != 1:
        fail("%s: mesh fbx gave %d meshes %d armatures" % (anim_file, len(meshes), len(mesh_arms)))
    mesh, mesh_arm = meshes[0], mesh_arms[0]
    mesh_bones = set(mesh_arm.data.bones.keys())

    bpy.ops.import_scene.fbx(filepath=os.path.join(ANIM_DIR, anim_file))
    all_arms = [o for o in bpy.data.objects if o.type == "ARMATURE"]
    anim_arms = [a for a in all_arms if a is not mesh_arm]
    if len(anim_arms) != 1:
        fail("%s: anim fbx gave %d extra armatures" % (anim_file, len(anim_arms)))
    anim_arm = anim_arms[0]
    if set(anim_arm.data.bones.keys()) != mesh_bones:
        fail("%s: bone sets differ" % anim_file)
    if not (anim_arm.animation_data and anim_arm.animation_data.action):
        fail("%s: anim armature has no action" % anim_file)

    # Re-point the mesh at the animated armature (vertex groups are
    # name-keyed, so identical bone names keep every weight intact).
    for mod in [m for m in mesh.modifiers if m.type == "ARMATURE"]:
        mesh.modifiers.remove(mod)
    mesh.parent = None
    mesh.matrix_world.identity()
    bpy.ops.object.select_all(action="DESELECT")
    mesh.select_set(True)
    anim_arm.select_set(True)
    bpy.context.view_layer.objects.active = anim_arm
    bpy.ops.object.parent_set(type="ARMATURE")
    bpy.data.objects.remove(mesh_arm, do_unlink=True)

    out = os.path.join(OUT_DIR, anim_file)
    bpy.ops.export_scene.fbx(
        filepath=out,
        use_selection=False,
        bake_anim=True,
        bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False,
        bake_anim_step=1.0,
        bake_anim_simplify_factor=0.0,
        add_leaf_bones=False,
        mesh_smooth_type="FACE",
    )
    print("T113_MERGED %s (%d bytes)" % (out, os.path.getsize(out)))

print("T113_MERGE_DONE")
