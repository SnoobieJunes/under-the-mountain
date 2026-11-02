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

    # [2026-11-02 T-068] Chord cut: bird wing, not bat sail. Weight detach
    # [2026-11-02 T-068] alone left stretched triangles webbing from the wing
    # [2026-11-02 T-068] to the hips (owner screenshot 2026-11-02: delta
    # [2026-11-02 T-068] silhouette despite straight bones, UE sweep test
    # [2026-11-02 T-068] green). Pull wing-dominant verts toward the nearest
    # [2026-11-02 T-068] point on the (new) wing chain: perpendicular offset
    # [2026-11-02 T-068] scaled to 45%, blended by wing weight so the root
    # [2026-11-02 T-068] blend zone does not tear. The membrane becomes a
    # [2026-11-02 T-068] surface hugging the arm; nothing spans to the hips.
    def closest_chain_point(pos, side):
        best, best_d = None, 1e9
        pts = [new_heads[f"wing_shoulder.{side}"]] + [new_tails[f"{b}.{side}"] for b in CHAIN]
        for a, b in zip(pts, pts[1:]):
            ab = b - a
            denom = ab.length_squared
            s = 0.0 if denom == 0 else max(0.0, min(1.0, (pos - a).dot(ab) / denom))
            cand = a + ab * s
            d = (pos - cand).length
            if d < best_d:
                best, best_d = cand, d
        return best

    cut = 0
    for vert in mesh.data.vertices:
        weights = {group_names[g.group]: g.weight for g in vert.groups}
        for side in ("L", "R"):
            wing_w = sum(weights.get(f"{b}.{side}", 0.0) for b in CHAIN)
            if wing_w < 0.35:
                continue
            blend = min(1.0, max(0.0, (wing_w - 0.35) / 0.4))
            scale = 1.0 + (0.45 - 1.0) * blend
            near = closest_chain_point(Vector(vert.co), side)
            vert.co = near + (Vector(vert.co) - near) * scale
            cut += 1
            break
    print(f"VERTS_CHORD_CUT {cut}")

    # [2026-11-02 T-068] Detach the membrane from the hips (owner contract:
    # [2026-11-02 T-068] bird-like independent wing arm). The bat-style sail
    # [2026-11-02 T-068] existed because membrane verts carry pelvis/hindleg/
    # [2026-11-02 T-068] tail/spine weights, anchoring the trailing edge at
    # [2026-11-02 T-068] the hips in every pose. For every vert that is
    # [2026-11-02 T-068] meaningfully wing-driven (>=0.35 same-side wing
    # [2026-11-02 T-068] weight), move all pelvis/hindleg/tail/spine weight to
    # [2026-11-02 T-068] its dominant wing bone. Chest weight is kept (root
    # [2026-11-02 T-068] blend at the shoulder). Asserted below so a regression
    # [2026-11-02 T-068] fails the export instead of shipping silently.
    hip_groups = {"pelvis", "spine", "spine2"} | {
        f"{k}.{s}" for k in ("hindleg_upper", "hindleg_lower", "hindleg_foot") for s in ("L", "R")
    } | {f"tail.0{i}" for i in range(1, 7)}
    group_index = {g.name: g.index for g in mesh.vertex_groups}
    detached = 0
    for vert in mesh.data.vertices:
        weights = {group_names[g.group]: g.weight for g in vert.groups}
        hip_total_all = sum(w for n, w in weights.items() if n in hip_groups)
        for side in ("L", "R"):
            wing = {f"{b}.{side}": weights.get(f"{b}.{side}", 0.0) for b in CHAIN}
            # [2026-11-02 T-068] Wing-DOMINANT verts only: hind-thigh surface
            # [2026-11-02 T-068] verts carry minor wing weight from heat
            # [2026-11-02 T-068] transfer; stripping their hindleg weights made
            # [2026-11-02 T-068] thighs follow the wing and hang untucked.
            if sum(wing.values()) < 0.35 or sum(wing.values()) <= hip_total_all:
                continue
            dominant = max(wing, key=wing.get)
            moved_w = 0.0
            for name in hip_groups:
                if name in weights and weights[name] > 0.0:
                    moved_w += weights[name]
                    mesh.vertex_groups[group_index[name]].remove([vert.index])
            if moved_w > 0.0:
                mesh.vertex_groups[group_index[dominant]].add(
                    [vert.index], weights[dominant] + moved_w, "REPLACE")
                detached += 1
            break
    print(f"VERTS_DETACHED {detached}")
    for vert in mesh.data.vertices:
        weights = {group_names[g.group]: g.weight for g in vert.groups}
        wing_total = sum(w for n, w in weights.items() if n.startswith("wing_"))
        hip_total = sum(w for n, w in weights.items() if n in hip_groups)
        if wing_total >= 0.35 and wing_total > hip_total and hip_total > 0.01:
            raise RuntimeError(f"membrane vert {vert.index} still hip-anchored ({hip_total:.3f})")

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
