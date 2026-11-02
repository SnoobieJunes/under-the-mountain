# [2026-11-01 T-068] lengthen_wings.py — the permanent wingspan fix.
# [2026-11-01 T-068] The v4 rig's wing chain is 0.43 units on a 1.9-unit body
# [2026-11-01 T-068] (measured 2026-11-01), so no rotation or pose-scale trick
# [2026-11-01 T-068] gives an honest flying span. This script lengthens the
# [2026-11-01 T-068] wing EDIT bones along their existing directions and moves
# [2026-11-01 T-068] every mesh vertex by its wing-weighted joint displacement
# [2026-11-01 T-068] (sum over wing bones of weight x (new head - old head)),
# [2026-11-01 T-068] so the membrane geometry itself becomes a bigger wing and
# [2026-11-01 T-068] the existing skin weights stay valid — same 33 bones, same
# [2026-11-01 T-068] names, same groups. Reads dragon_v4_rigged.blend, writes
# [2026-11-01 T-068] dragon_v5_rigged.blend (v4 kept untouched for rollback)
# [2026-11-01 T-068] plus dragon_v5_mesh.fbx (skeletal mesh + Walk) for Unreal.
# [2026-11-01 T-068] Run: Blender -b --python Scripts/lengthen_wings.py

import os

import bpy
from mathutils import Vector

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v4_rigged.blend")
DST_BLEND = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v5_rigged.blend")
DST_FBX = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v5_mesh.fbx")

# [2026-11-01 T-068] Per-segment length multipliers: chain 0.43 -> ~0.84,
# [2026-11-01 T-068] outer segments grow most (a wing is mostly hand/finger).
GROW = {"wing_shoulder": 1.5, "wing_elbow": 1.9, "wing_wrist": 2.2, "wing_finger": 2.4}
CHAIN = ("wing_shoulder", "wing_elbow", "wing_wrist", "wing_finger")


def main():
    bpy.ops.wm.open_mainfile(filepath=SRC)
    arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    mesh = next(o for o in bpy.data.objects if o.type == "MESH")

    # --- measure old joints (armature space == edit-bone head_local) ---
    # [2026-11-02 T-068] ALL new joints are precomputed here and applied from
    # [2026-11-02 T-068] storage below. The first version measured each edit
    # [2026-11-02 T-068] bone AFTER moving its parent: connected heads move
    # [2026-11-02 T-068] with the parent tail, so downstream lengths were
    # [2026-11-02 T-068] measured already-shrunk (elbow shipped 0.148, not the
    # [2026-11-02 T-068] printed 0.288) and verts/bones disagreed in-game.
    old_heads, new_heads, new_tails, deltas = {}, {}, {}, {}
    for side in ("L", "R"):
        prev_new_tail = None
        for base in CHAIN:
            name = f"{base}.{side}"
            bone = arm.data.bones[name]
            head, tail = Vector(bone.head_local), Vector(bone.tail_local)
            direction = (tail - head).normalized()
            new_head = head if prev_new_tail is None else prev_new_tail
            new_tail = new_head + direction * (tail - head).length * GROW[base]
            old_heads[name], new_heads[name], new_tails[name] = head, new_head, new_tail
            deltas[name] = new_head - head
            prev_new_tail = new_tail
            print(f"WING_GROW {name} len {(tail-head).length:.3f} -> {(new_tail-new_head).length:.3f}")

    # --- displace verts by wing-weighted joint deltas ---
    group_names = [g.name for g in mesh.vertex_groups]
    moved = 0
    for vert in mesh.data.vertices:
        disp = Vector((0.0, 0.0, 0.0))
        for grp in vert.groups:
            name = group_names[grp.group]
            if name in deltas:
                disp += deltas[name] * grp.weight
        if disp.length > 1e-6:
            vert.co += disp
            moved += 1
    print(f"VERTS_MOVED {moved}")

    # --- apply new edit-bone positions ---
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    for name in new_heads:
        ebone = arm.data.edit_bones[name]
        ebone.head = new_heads[name]
        ebone.tail = new_tails[name]
    bpy.ops.object.mode_set(mode="OBJECT")
    for name in new_heads:
        got = arm.data.bones[name].length
        want = (new_tails[name] - new_heads[name]).length
        print(f"WING_LEN_CHECK {name} got {got:.3f} want {want:.3f}")
        if abs(got - want) > 0.005:
            raise RuntimeError(f"wing bone length mismatch: {name} got {got} want {want}")

    for side in ("L", "R"):
        tip = arm.data.bones[f"wing_finger.{side}"].tail_local
        print(f"WING_TIP {side} {tuple(round(v, 3) for v in tip)}")

    bpy.ops.wm.save_as_mainfile(filepath=DST_BLEND)
    print(f"V5_BLEND_WRITTEN {DST_BLEND}")

    # --- export skeletal mesh (Walk action rides along, as the v4 walk FBX) ---
    for other in list(bpy.data.actions):
        if other.name != "Walk":
            bpy.data.actions.remove(other, do_unlink=True)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.context.scene.objects:
        if obj.type in {"ARMATURE", "MESH"}:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.fbx(
        filepath=DST_FBX, use_selection=True, bake_anim=True,
        bake_anim_use_all_actions=True, axis_forward="-Y", axis_up="Z",
        apply_unit_scale=True,
    )
    print(f"V5_FBX_WRITTEN {DST_FBX}")


if __name__ == "__main__":
    main()
