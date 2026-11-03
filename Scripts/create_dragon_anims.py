# [2026-11-02 T-068] create_dragon_anims.py — author the dragon clip set
# (Fly, Glide, Hover, Idle, Takeoff, Land, Roar, Hit) for the 7-wing-bone-per-side
# v6 rig (wing_shoulder, wing_elbow, wing_wrist, wing_finger + wing_finger_1..3),
# with rolled wing folding + finger-fan articulation, inward-tucked front arms
# that clear the wing root/downstroke, and streamlined rear feet in flight.

import math
import os

import bpy
from mathutils import Matrix, Quaternion, Vector

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_V6_PATH = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v6_experiment.blend")
_V5_PATH = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v5_rigged.blend")
BLEND_PATH = os.environ.get("DRAGON_BLEND", _V6_PATH if os.path.exists(_V6_PATH) else _V5_PATH)
OUT_DIR = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon")
FPS = 24

SIDE_SIGN = {"L": -1.0, "R": 1.0}
WING_BASES = ("wing_shoulder", "wing_elbow", "wing_wrist", "wing_finger")
FINGER_BASES = ("wing_finger_1", "wing_finger_2", "wing_finger_3")
FINGER_SPREAD_DEG = (38.904183, 62.805158, 88.191294)

# Flight leg tuck (deg in local XYZ Euler, where Y/Z signs mirror by (-SIDE_SIGN[side])).
# Front legs fold inward under the pectoral chest well clear of the wing root and downstroke;
# Hind legs sweep back and slightly below the tail line with plantar-flexed trailing feet.
TUCK_FRONT = {
    "upper": (44.0, -8.0, 15.0),
    "lower": (-68.0, 0.0, -10.0),
    "foot":  (8.0, 0.0, -22.0),
}
TUCK_HIND = {
    "upper": (40.0, -5.0, 10.0),
    "lower": (-14.0, 0.0, -6.0),
    "foot":  (24.0, -12.0, 48.0),
}
TAIL_BONES = ("tail.01", "tail.02", "tail.03", "tail.04", "tail.05", "tail.06")
REST_DIRS = {}


def pb(arm, name):
    bone = arm.pose.bones.get(name)
    if bone is None:
        raise RuntimeError(f"required bone not found: {name}")
    bone.rotation_mode = "XYZ"
    return bone


def reset_pose(arm):
    for bone in arm.pose.bones:
        bone.rotation_euler = (0.0, 0.0, 0.0)
        bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        bone.location = (0.0, 0.0, 0.0)
        bone.scale = (1.0, 1.0, 1.0)


def set_rot(bone, x=0.0, y=0.0, z=0.0):
    bone.rotation_euler = (math.radians(x), math.radians(y), math.radians(z))


def key(bones, frame, paths=None):
    for bone in bones:
        bone_paths = paths or (
            ("rotation_quaternion",) if bone.rotation_mode == "QUATERNION" else ("rotation_euler",)
        )
        for path in bone_paths:
            bone.keyframe_insert(data_path=path, frame=frame)


def wing_pose(arm, flap_deg, unfold_scale=1.0, lag_flap=None):
    """Pose both wings across all 7 wing bones per side (4 leading-edge + 3 trailing finger bones).

    At unfold_scale=1.0, aligns the leading-edge chain to the straight gull-sloped flight line
    with dorsal normal facing up, and fans wing_finger_1..3 out in the wing plane.
    At unfold_scale=0.0, returns to the rolled, fan-closed folded bind posture along each flank.
    Intermediate unfold_scale values slerp local quaternions so the wing smoothly unrolls while
    the finger fan opens in plane.
    """
    bones = []
    q_spread = {}
    for side in ("L", "R"):
        s_sign = 1.0 if side == "L" else -1.0
        for base in WING_BASES:
            bone = arm.pose.bones.get(f"{base}.{side}")
            if bone is None:
                raise RuntimeError(f"required bone not found: {base}.{side}")
            bone.rotation_mode = "QUATERNION"
            flap = lag_flap[base] if (lag_flap and base in lag_flap) else flap_deg
            phi = math.radians(flap - 15.0)
            y_dir = Vector((s_sign * math.cos(phi), 0.0, math.sin(phi))).normalized()
            b_trail = Vector((0.0, 1.0, 0.0))
            x_dir = (-s_sign) * b_trail
            z_dir = x_dir.cross(y_dir).normalized()
            x_dir = y_dir.cross(z_dir).normalized()
            R = Matrix((x_dir, y_dir, z_dir)).transposed().to_4x4()
            bpy.context.view_layer.update()
            R.translation = Vector(bone.head)
            bone.matrix = R
            bpy.context.view_layer.update()
            q_spread[bone.name] = bone.rotation_quaternion.copy()
            bones.append(bone)

        for fi, fbase in enumerate(FINGER_BASES):
            bone = arm.pose.bones.get(f"{fbase}.{side}")
            if bone is None:
                continue
            bone.rotation_mode = "QUATERNION"
            flap = (
                lag_flap[fbase]
                if (lag_flap and fbase in lag_flap)
                else (lag_flap["wing_wrist"] if (lag_flap and "wing_wrist" in lag_flap) else flap_deg)
            )
            phi = math.radians(flap - 15.0)
            d_lead = Vector((s_sign * math.cos(phi), 0.0, math.sin(phi))).normalized()
            b_wing = Vector((0.0, 1.0, 0.0))
            th = math.radians(FINGER_SPREAD_DEG[fi])
            y_dir = (math.cos(th) * d_lead + math.sin(th) * b_wing).normalized()
            b_trail = (-math.sin(th) * d_lead + math.cos(th) * b_wing).normalized()
            x_dir = (-s_sign) * b_trail
            z_dir = x_dir.cross(y_dir).normalized()
            x_dir = y_dir.cross(z_dir).normalized()
            R = Matrix((x_dir, y_dir, z_dir)).transposed().to_4x4()
            bpy.context.view_layer.update()
            R.translation = Vector(bone.head)
            bone.matrix = R
            bpy.context.view_layer.update()
            q_spread[bone.name] = bone.rotation_quaternion.copy()
            bones.append(bone)

    if unfold_scale < 1.0:
        ident = Quaternion((1.0, 0.0, 0.0, 0.0))
        for bone in bones:
            bone.rotation_quaternion = ident.slerp(q_spread[bone.name], max(0.0, min(1.0, unfold_scale)))
        bpy.context.view_layer.update()
    return bones


def _pose_tuple(val, default_adduct=0.0):
    if isinstance(val, (tuple, list)):
        return float(val[0]), float(val[1]), float(val[2])
    return float(val), float(default_adduct), 0.0


def leg_pose(arm, tuck_scale=1.0, front=None, hind=None):
    front = front or TUCK_FRONT
    hind = hind or TUCK_HIND
    bones = []
    for side in ("L", "R"):
        ms = -SIDE_SIGN[side]  # +1 on L, -1 on R
        for kind, pose in (("frontleg", front), ("hindleg", hind)):
            for part in ("upper", "lower", "foot"):
                bone = pb(arm, f"{kind}_{part}.{side}")
                rx, ry, rz = _pose_tuple(pose[part])
                set_rot(bone, x=rx * tuck_scale, y=ry * ms * tuck_scale, z=rz * ms * tuck_scale)
                bones.append(bone)
    return bones


def tail_wave(arm, frame, span, amp_deg=4.0, base_deg=0.0):
    bones = []
    for i, name in enumerate(TAIL_BONES):
        bone = pb(arm, name)
        phase = 2 * math.pi * (frame - 1 - 2 * i) / span if span else 0.0
        set_rot(bone, x=base_deg + amp_deg * math.sin(phase))
        bones.append(bone)
    return bones


def sine(frame, span, lag=0):
    return math.sin(2 * math.pi * (frame - 1 - lag) / span + math.pi / 2)


def open_rig(action_name, frame_end):
    bpy.ops.wm.open_mainfile(filepath=BLEND_PATH)
    scene = bpy.context.scene
    scene.frame_start = 1
    scene.frame_end = frame_end
    scene.render.fps = FPS
    scene.render.fps_base = 1.0
    arm = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    old = bpy.data.actions.get(action_name)
    if old is not None:
        bpy.data.actions.remove(old, do_unlink=True)
    action = bpy.data.actions.new(name=action_name)
    if arm.animation_data is None:
        arm.animation_data_create()
    arm.animation_data.action = action
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="POSE")
    reset_pose(arm)
    REST_DIRS.clear()
    for side in ("L", "R"):
        for base in WING_BASES:
            edit = arm.data.bones[f"{base}.{side}"]
            REST_DIRS[f"{base}.{side}"] = (
                Vector(edit.tail_local) - Vector(edit.head_local)
            ).normalized()
    return arm, action


def export(arm, action, filename):
    bpy.ops.object.mode_set(mode="OBJECT")
    for other in list(bpy.data.actions):
        if other != action:
            bpy.data.actions.remove(other, do_unlink=True)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in bpy.context.scene.objects:
        if obj.type in {"ARMATURE", "MESH"}:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = os.path.join(OUT_DIR, filename)
    bpy.ops.export_scene.fbx(
        filepath=path, use_selection=True, bake_anim=True,
        bake_anim_use_all_actions=True, axis_forward="-Y", axis_up="Z",
        apply_unit_scale=True,
    )
    print(f"DRAGON_ANIM_WRITTEN {path}")


# --- clips -----------------------------------------------------------------

def clip_fly():
    span, end = 23, 24
    arm, action = open_rig("Fly", end)
    pelvis, chest = pb(arm, "pelvis"), pb(arm, "chest")
    neck = pb(arm, "neck")
    pbind = tuple(pelvis.location)
    for frame in (1, 7, 13, 19, 24):
        lag_flap = {b: 32.0 * sine(frame, span) for b in WING_BASES}
        keyed = wing_pose(arm, 0, 1.0, lag_flap) + leg_pose(arm, 1.0) + tail_wave(arm, frame, span, 4.0)
        bob = -0.03 * sine(frame, span)
        pelvis.location = (pbind[0], pbind[1], pbind[2] + bob)
        set_rot(chest, x=3.0 * sine(frame, span))
        set_rot(neck, x=-2.0 * sine(frame, span))
        key(keyed + [chest, neck], frame)
        key([pelvis], frame, ("location",))
    export(arm, action, "dragon_v4_flight.fbx")


def clip_glide():
    span, end = 47, 48
    arm, action = open_rig("Glide", end)
    chest = pb(arm, "chest")
    for frame in (1, 13, 25, 37, 48):
        lag_flap = {b: 2.0 * sine(frame, span) for b in WING_BASES}
        keyed = wing_pose(arm, 0, 1.0, lag_flap) + leg_pose(arm, 1.0) + tail_wave(arm, frame, span, 2.5)
        set_rot(chest, x=1.0 * sine(frame, span))
        key(keyed + [chest], frame)
    export(arm, action, "dragon_v4_glide.fbx")


def clip_hover():
    span, end = 23, 24
    arm, action = open_rig("Hover", end)
    pelvis, chest, neck, head = pb(arm, "pelvis"), pb(arm, "chest"), pb(arm, "neck"), pb(arm, "head")
    half_f = {"upper": (22.0, -4.0, 8.0), "lower": (-34.0, 0.0, -5.0), "foot": (4.0, 0.0, -10.0)}
    half_h = {"upper": (18.0, -2.0, 5.0), "lower": (-8.0, 0.0, -3.0), "foot": (12.0, -6.0, 24.0)}
    for frame in (1, 7, 13, 19, 24):
        lag_flap = {b: 42.0 * sine(frame, span) for b in WING_BASES}
        keyed = wing_pose(arm, 0, 1.0, lag_flap)
        keyed += leg_pose(arm, 1.0, front=half_f, hind=half_h)
        keyed += tail_wave(arm, frame, span, 3.0, base_deg=10.0)
        set_rot(chest, x=-22.0 + 4.0 * sine(frame, span))
        set_rot(neck, x=14.0)
        set_rot(head, x=6.0)
        key(keyed + [chest, neck, head], frame)
    export(arm, action, "dragon_v4_hover.fbx")


def clip_idle():
    span, end = 47, 48
    arm, action = open_rig("Idle", end)
    chest, neck, head = pb(arm, "chest"), pb(arm, "neck"), pb(arm, "head")
    for frame in (1, 13, 25, 37, 48):
        s = sine(frame, span)
        keyed = wing_pose(arm, 0, 0.0) + leg_pose(arm, 0.0) + tail_wave(arm, frame, span, 1.5)
        set_rot(chest, x=1.6 * s)
        set_rot(neck, x=-1.0 * s, z=2.0 * sine(frame, span, 8))
        set_rot(head, x=0.8 * s)
        key(keyed + [chest, neck, head], frame)
    export(arm, action, "dragon_v4_idle.fbx")


def clip_takeoff():
    arm, action = open_rig("Takeoff", 24)
    pelvis, chest = pb(arm, "pelvis"), pb(arm, "chest")
    pbind = tuple(pelvis.location)
    bend = {"upper": (-28.0, 0.0, 0.0), "lower": (42.0, 0.0, 0.0), "foot": (0.0, 0.0, -18.0)}
    poses = [
        (1, -12.0, 0.15, 0.0, bend, -0.05),
        (8, -38.0, 0.85, 0.35, None, 0.10),
        (16, 30.0, 1.0, 0.85, None, 0.05),
        (24, 36.0, 1.0, 1.0, None, 0.03),
    ]
    for frame, flap, unfold, tuck, legs, bob in poses:
        keyed = wing_pose(arm, flap, unfold)
        keyed += leg_pose(arm, tuck, front=legs, hind=legs) if legs else leg_pose(arm, tuck)
        keyed += tail_wave(arm, frame, 0, 0.0)
        pelvis.location = (pbind[0], pbind[1], pbind[2] + bob)
        set_rot(chest, x=-6.0 if frame == 1 else 2.0)
        key(keyed + [chest], frame)
        key([pelvis], frame, ("location",))
    export(arm, action, "dragon_v4_takeoff.fbx")


def clip_land():
    arm, action = open_rig("Land", 24)
    pelvis, chest = pb(arm, "pelvis"), pb(arm, "chest")
    pbind = tuple(pelvis.location)
    reach = {"upper": (-12.0, 0.0, 0.0), "lower": (10.0, 0.0, 0.0), "foot": (0.0, 0.0, 0.0)}
    poses = [
        (1, 34.0, 1.0, 0.7, None, 0.05, 6.0),
        (10, 42.0, 1.0, 0.15, reach, 0.0, 14.0),
        (17, 8.0, 0.45, 0.0, None, -0.03, 4.0),
        (24, 0.0, 0.0, 0.0, None, 0.0, 0.0),
    ]
    for frame, flap, unfold, tuck, legs, bob, pitch in poses:
        keyed = wing_pose(arm, flap, unfold)
        keyed += leg_pose(arm, tuck, front=legs, hind=legs) if legs else leg_pose(arm, tuck)
        keyed += tail_wave(arm, frame, 0, 0.0)
        pelvis.location = (pbind[0], pbind[1], pbind[2] + bob)
        set_rot(chest, x=-pitch)
        key(keyed + [chest], frame)
        key([pelvis], frame, ("location",))
    export(arm, action, "dragon_v4_land.fbx")


def clip_roar():
    arm, action = open_rig("Roar", 36)
    chest, neck, head = pb(arm, "chest"), pb(arm, "neck"), pb(arm, "head")
    poses = [(1, 0.0, 0.0, 0.0, 0.0, 0.0), (8, 0.0, 0.1, -8.0, -6.0, -4.0),
             (18, 26.0, 0.55, 10.0, 16.0, 12.0), (26, 20.0, 0.5, 8.0, 12.0, 8.0),
             (36, 0.0, 0.0, 0.0, 0.0, 0.0)]
    for frame, flap, unfold, cx, nx, hx in poses:
        keyed = wing_pose(arm, flap, unfold) + leg_pose(arm, 0.0) + tail_wave(arm, frame, 0, 0.0)
        set_rot(chest, x=cx)
        set_rot(neck, x=nx)
        set_rot(head, x=hx)
        key(keyed + [chest, neck, head], frame)
    export(arm, action, "dragon_v4_roar.fbx")


def clip_hit():
    arm, action = open_rig("HitLight", 16)
    chest, neck, head = pb(arm, "chest"), pb(arm, "neck"), pb(arm, "head")
    poses = [(1, 0.0, 0.0, 0.0), (4, -10.0, 8.0, 14.0), (9, -5.0, 3.0, 6.0), (16, 0.0, 0.0, 0.0)]
    for frame, cx, cz, hx in poses:
        keyed = wing_pose(arm, 0, 0.0) + leg_pose(arm, 0.0) + tail_wave(arm, frame, 0, 0.0)
        set_rot(chest, x=cx, z=cz)
        set_rot(neck, x=hx * 0.7, z=cz)
        set_rot(head, x=hx, z=cz * 0.5)
        key(keyed + [chest, neck, head], frame)
    export(arm, action, "dragon_v4_hit.fbx")


CLIPS = (clip_fly, clip_glide, clip_hover, clip_idle, clip_takeoff, clip_land, clip_roar, clip_hit)

if __name__ == "__main__":
    for clip in CLIPS:
        clip()
    print("DRAGON_ANIMS_DONE")
