# add_jaw_bone.py — add ONE 'jaw' bone to dragon_v6_experiment.blend, skin the
# lower jaw, add a FirePose jaw-open, and export dragon_v6_jaw.blend/.fbx.
# Blender only; no mesh vertex is moved and no topology is changed. The only
# skin change is a new 'jaw' vertex group plus a proportional (1-w) rescale of
# pre-existing weights on those same lower-jaw verts so weights still sum ~1.
# Run: Blender --background --python Scripts/add_jaw_bone.py
import math
import os

import bpy
from mathutils import Vector

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IN_BLEND = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v6_experiment.blend")
OUT_BLEND = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v6_jaw.blend")
OUT_FBX = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v6_jaw.fbx")
SAVED_DIR = os.path.join(REPO_ROOT, "Saved")
JAW_OPEN_DEG = 25.0


def clamp(v, lo=0.0, hi=1.0):
    return max(lo, min(hi, v))


def group_weight(vert, group_index):
    for ge in vert.groups:
        if ge.group == group_index:
            return ge.weight
    return 0.0


def main():
    if os.path.abspath(IN_BLEND) == os.path.abspath(OUT_BLEND):
        raise RuntimeError("output would overwrite experiment blend")
    bpy.ops.wm.open_mainfile(filepath=IN_BLEND)
    arm_obj = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    mesh_obj = next(o for o in bpy.data.objects if o.type == "MESH")
    print(f"ARMATURE {arm_obj.name} bones_before={len(arm_obj.data.bones)}")
    print("BONES_BEFORE", [b.name for b in arm_obj.data.bones])
    print(f"MESH {mesh_obj.name} verts={len(mesh_obj.data.vertices)} polys={len(mesh_obj.data.polygons)} groups_before={[g.name for g in mesh_obj.vertex_groups]}")
    print("ACTIONS_BEFORE", [a.name for a in bpy.data.actions])
    if "head" not in arm_obj.data.bones:
        raise RuntimeError("head bone not found")

    head_bone = arm_obj.data.bones["head"]
    head_head = Vector(head_bone.head_local)
    head_tail = Vector(head_bone.tail_local)
    print(f"HEAD_BONE head_local={tuple(head_head)} tail_local={tuple(head_tail)} parent={head_bone.parent.name if head_bone.parent else None}")

    # --- measurements from mesh data (world == local: both objects identity) ---
    gidx = {g.name: g.index for g in mesh_obj.vertex_groups}
    head_pts = []  # (co, head_w, head_end_w)
    for v in mesh_obj.data.vertices:
        hw = group_weight(v, gidx["head"]) if "head" in gidx else 0.0
        ew = group_weight(v, gidx["head_end"]) if "head_end" in gidx else 0.0
        if hw + ew > 0.1:
            head_pts.append((Vector(v.co), hw, ew))
    ys = [p[0].y for p in head_pts]; zs = [p[0].z for p in head_pts]; xs = [p[0].x for p in head_pts]
    y_front, y_back = min(ys), max(ys)  # forward is -Y
    forward_third_y = y_front + (y_back - y_front) / 3.0
    print(f"MEASURE head_weighted(>0.1) n={len(head_pts)} X[{min(xs):.4f},{max(xs):.4f}] Y_front={y_front:.4f} Y_back={y_back:.4f} Z[{min(zs):.4f},{max(zs):.4f}] forward_third_Y<={forward_third_y:.4f}")

    # Hinge: just forward of the head bone's head (skull/neck junction), low in
    # the local head cross-section at that Y.
    hinge_y = head_head.y - 0.02
    near = [p[0] for p in head_pts if abs(p[0].y - hinge_y) < 0.03]
    nz = [p.z for p in near]
    hinge_z = min(nz) + 0.35 * (max(nz) - min(nz)) if nz else head_head.z - 0.03
    hinge = Vector((0.0, hinge_y, hinge_z))
    # Head-axis slope dz/dy, reused for the mouth line and jaw tail.
    slope = (head_tail.z - head_head.z) / (head_tail.y - head_head.y) if head_tail.y != head_head.y else 0.0
    tail_y = y_front + 0.02
    jaw_tail = Vector((0.0, tail_y, hinge_z + slope * (tail_y - hinge_y)))
    print(f"MEASURE near_hinge n={len(near)} Z[{min(nz):.4f},{max(nz):.4f}] slope_dzdy={slope:.4f}")
    print(f"JAW_HINGE head={tuple(round(v,4) for v in hinge)} tail={tuple(round(v,4) for v in jaw_tail)} length={(jaw_tail-hinge).length:.4f}")

    def mouth_z(y):
        # Mouth line sits just above the jaw-bone axis so the bone lies inside
        # the lower jaw rather than exactly on the seam.
        return hinge_z + slope * (y - hinge_y) + 0.025

    def dist_to_jaw_segment(co):
        ab = jaw_tail - hinge
        t = clamp((co - hinge).dot(ab) / ab.length_squared) if ab.length_squared else 0.0
        return (co - (hinge + ab * t)).length

    # --- add jaw bone (idempotent: replace a previous jaw) ---
    bpy.context.view_layer.objects.active = arm_obj
    arm_obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    ebones = arm_obj.data.edit_bones
    if "jaw" in ebones:
        ebones.remove(ebones["jaw"])
    jaw_eb = ebones.new("jaw")
    jaw_eb.head = hinge
    jaw_eb.tail = jaw_tail
    jaw_eb.parent = ebones["head"]
    jaw_eb.use_connect = False
    jaw_eb.use_deform = True
    bpy.ops.object.mode_set(mode="OBJECT")
    print(f"BONE jaw parent=head bones_after={len(arm_obj.data.bones)}")

    # --- skin lower jaw ---
    if "jaw" in mesh_obj.vertex_groups:
        mesh_obj.vertex_groups.remove(mesh_obj.vertex_groups["jaw"])
    jaw_group = mesh_obj.vertex_groups.new(name="jaw")
    jaw_idx = jaw_group.index
    # Refresh group indices after creation.
    gidx = {g.name: g.index for g in mesh_obj.vertex_groups}
    jaw_idx = gidx["jaw"]
    jaw_span = max(0.01, hinge_y - y_front)
    weighted = []
    weights = []
    for v in mesh_obj.data.vertices:
        co = Vector(v.co)
        if co.y >= hinge_y:  # must be forward of hinge
            continue
        head_inf = 0.0
        if "head" in gidx:
            head_inf += group_weight(v, gidx["head"])
        if "head_end" in gidx:
            head_inf += group_weight(v, gidx["head_end"])
        if head_inf <= 0.05:  # only existing head skin can become jaw skin
            continue
        mz = mouth_z(co.y)
        if co.z >= mz:  # must be below mouth line
            continue
        # Falloff by distance, two ways: ramp in over the first 30% forward of
        # the hinge (so the hinge itself does not tear), then fall off with
        # distance from the jaw-bone segment (verts hugging the bone move most).
        forward_ramp = clamp((hinge_y - co.y) / (0.3 * jaw_span))
        proximity = clamp(1.0 - dist_to_jaw_segment(co) / 0.10)
        # Fade in just below the seam so the closed sculpt's mouth line does
        # not tear/stretch across a single edge (there is no mouth cavity).
        seam_fade = clamp((mz - co.z) / 0.03, 0.25, 1.0)
        w = clamp(forward_ramp * (0.35 + 0.65 * proximity) * seam_fade, 0.0, 0.95)
        if w <= 0.01:
            continue
        # Proportional rescale of existing weights on this vert only, then jaw.
        for ge in list(v.groups):
            if ge.group != jaw_idx:
                ge.weight *= (1.0 - w)
        jaw_group.add([v.index], w, "REPLACE")
        weighted.append(v.index)
        weights.append(w)
    print(f"JAW_GROUP verts_weighted={len(weighted)} weight_min={min(weights):.4f} weight_max={max(weights):.4f} weight_mean={sum(weights)/len(weights):.4f}" if weights else "JAW_GROUP verts_weighted=0")

    # --- choose jaw-open sign empirically: +X should pitch the tip down ---
    # Any action assigned in the source blend would override manual pose-bone
    # writes during depsgraph evaluation, so detach it for this test.
    saved_action = arm_obj.animation_data.action if arm_obj.animation_data else None
    if arm_obj.animation_data:
        arm_obj.animation_data.action = None

    def reset_all_pose():
        bpy.ops.object.mode_set(mode="POSE")
        for b in arm_obj.pose.bones:
            b.rotation_mode = "XYZ"
            b.location = (0.0, 0.0, 0.0)
            b.rotation_euler = (0.0, 0.0, 0.0)
            b.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
            b.scale = (1.0, 1.0, 1.0)

    def avg_jaw_tip_dz(angle_deg):
        reset_all_pose()
        jaw_pb = arm_obj.pose.bones["jaw"]
        jaw_pb.rotation_mode = "XYZ"
        jaw_pb.rotation_euler = (math.radians(angle_deg), 0.0, 0.0)
        bpy.context.view_layer.update()
        dg = bpy.context.evaluated_depsgraph_get()
        ev = mesh_obj.evaluated_get(dg)
        tmp = ev.to_mesh()
        try:
            tip = [i for i, w in zip(weighted, weights) if w > 0.5]
            if not tip:
                tip = weighted[:100]
            dz = sum(tmp.vertices[i].co.z - mesh_obj.data.vertices[i].co.z for i in tip) / len(tip)
            dy = sum(tmp.vertices[i].co.y - mesh_obj.data.vertices[i].co.y for i in tip) / len(tip)
            return dz, dy, len(tip)
        finally:
            ev.to_mesh_clear()

    dz_rest, dy_rest, _ = avg_jaw_tip_dz(0.0)
    dz_pos, dy_pos, ntip = avg_jaw_tip_dz(+JAW_OPEN_DEG)
    dz_neg, dy_neg, _ = avg_jaw_tip_dz(-JAW_OPEN_DEG)
    dz_pos -= dz_rest; dz_neg -= dz_rest
    sign = 1.0 if dz_pos < dz_neg else -1.0
    print(f"JAW_SIGN_TEST n_tip={ntip} rest_dz={dz_rest:.5f} +25deg dz={dz_pos:.5f} dy={dy_pos:.5f} ; -25deg dz={dz_neg:.5f} dy={dy_neg:.5f} ; chosen_sign={sign:+.0f} (down is -Z)")

    # Reset pose before authoring the action.
    reset_all_pose()

    # --- FirePose action: extend if present, else create the clip_firepose pose ---
    action = bpy.data.actions.get("FirePose")
    created = False
    if action is None:
        action = bpy.data.actions.new(name="FirePose")
        created = True
    if arm_obj.animation_data is None:
        arm_obj.animation_data_create()
    arm_obj.animation_data.action = action
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end, scene.render.fps = 1, 48, 24
    chest = arm_obj.pose.bones["chest"]; neck = arm_obj.pose.bones["neck"]
    head_pb = arm_obj.pose.bones["head"]; jaw_pb = arm_obj.pose.bones["jaw"]
    for bone in (chest, neck, head_pb, jaw_pb):
        bone.rotation_mode = "XYZ"

    def set_rot(bone, x=0.0):
        bone.rotation_euler = (math.radians(x), 0.0, 0.0)

    # Same frames/values as Scripts/create_dragon_anims.py clip_firepose for
    # chest/neck/head (sine span 47); jaw opens after frame 1 and holds.
    for frame in (1, 13, 25, 37, 48):
        s = math.sin(2 * math.pi * (frame - 1) / 47 + math.pi / 2)
        if created:
            set_rot(chest, 8.0 + 1.5 * s); set_rot(neck, 10.0); set_rot(head_pb, 8.0 + 1.0 * s)
            for bone in (chest, neck, head_pb):
                bone.keyframe_insert(data_path="rotation_euler", frame=frame)
        jaw_deg = 0.0 if frame == 1 else sign * JAW_OPEN_DEG
        jaw_pb.rotation_euler = (math.radians(jaw_deg), 0.0, 0.0)
        jaw_pb.keyframe_insert(data_path="rotation_euler", frame=frame)
    # Functional check (layered-action fcurve introspection varies by version):
    # sampling the action must give jaw 0 deg at frame 1 and the open angle at 25.
    scene.frame_set(1)
    jaw_f1 = math.degrees(jaw_pb.rotation_euler.x)
    scene.frame_set(25)
    jaw_f25 = math.degrees(jaw_pb.rotation_euler.x)
    print(f"FIREPOSE action={'created' if created else 'extended'} frames=1,13,25,37,48 jaw_deg_frame1={jaw_f1:+.1f} jaw_deg_frame25={jaw_f25:+.1f}")

    # Reset to closed for the saved file's initial pose, keep FirePose assigned.
    scene.frame_set(1)
    bpy.context.view_layer.update()

    # --- save blend (never the experiment file), then FBX with the same
    # export settings as create_dragon_anims.py:export() ---
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.wm.save_as_mainfile(filepath=OUT_BLEND)
    print(f"SAVED_BLEND {OUT_BLEND} size={os.path.getsize(OUT_BLEND)}")

    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.context.scene.objects:
        if obj.type in {"ARMATURE", "MESH"}:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = arm_obj
    bpy.ops.export_scene.fbx(
        filepath=OUT_FBX, use_selection=True, bake_anim=True,
        bake_anim_use_all_actions=True, axis_forward="-Y", axis_up="Z",
        apply_unit_scale=True,
    )
    print(f"EXPORTED_FBX {OUT_FBX} size={os.path.getsize(OUT_FBX)}")

    # --- evidence renders (camera/light added after save/export; not saved) ---
    os.makedirs(SAVED_DIR, exist_ok=True)
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = 800, 600
    scene.render.resolution_percentage = 100
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs[0].default_value = (0.05, 0.05, 0.06, 1)
    scene.world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    cam_data = bpy.data.cameras.new("JawCam"); cam_data.type = "ORTHO"; cam_data.ortho_scale = 0.48
    cam = bpy.data.objects.new("JawCam", cam_data)
    bpy.context.collection.objects.link(cam)
    cam.location = (0.85, -0.83, 0.14)
    direction = Vector((0.0, -0.83, 0.12)) - cam.location
    cam.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    sun_data = bpy.data.lights.new("JawSun", "SUN"); sun_data.energy = 3.0
    sun = bpy.data.objects.new("JawSun", sun_data)
    bpy.context.collection.objects.link(sun)
    sun.location = (0.5, -0.8, 1.0); sun.rotation_euler = (math.radians(20), 0.0, math.radians(30))
    area_data = bpy.data.lights.new("JawFill", "AREA"); area_data.energy = 500.0; area_data.shape = "DISK"; area_data.size = 1.0
    area = bpy.data.objects.new("JawFill", area_data)
    bpy.context.collection.objects.link(area)
    area.location = (-0.6, -1.2, 0.5)
    area.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler() if False else (math.radians(60), 0.0, math.radians(-30))

    # Isolate jaw: no action influence during renders.
    arm_obj.animation_data.action = None
    reset_all_pose()
    jaw_pb = arm_obj.pose.bones["jaw"]
    for name, deg in (("jaw_closed", 0.0), ("jaw_open", sign * JAW_OPEN_DEG)):
        jaw_pb.rotation_euler = (math.radians(deg), 0.0, 0.0)
        bpy.context.view_layer.update()
        out_png = os.path.join(SAVED_DIR, f"{name}.png")
        scene.render.filepath = out_png
        bpy.ops.render.render(write_still=True)
        print(f"RENDER {out_png} size={os.path.getsize(out_png)} jaw_deg={deg:+.1f}")
    print("ADD_JAW_DONE")


if __name__ == "__main__":
    main()
