# [2026-11-01 T-068] create_dragon_anims.py — author the dragon v1 clip set
# [2026-11-01 T-068] (Fly v2, Glide, Hover, Idle, Takeoff, Land, Roar, Hit)
# [2026-11-01 T-068] programmatically for SK_Dragon_v4 and export one FBX per
# [2026-11-01 T-068] clip for Unreal. Supersedes create_flight_anim.py's Fly
# [2026-11-01 T-068] (same dragon_v4_flight.fbx path, so the T-043 import and
# [2026-11-01 T-068] pose test keep working) and extends it per T-058/T-067:
# [2026-11-01 T-068] the flap now cycles around a fully-spread wing pose and
# [2026-11-01 T-068] all four legs are keyed tucked for the whole airborne
# [2026-11-01 T-068] cycle — the two defects the owner called out in F2.
# [2026-11-01 T-068] Run (Blender 5.2, headless, from the repo root):
# [2026-11-01 T-068]   Blender -b --python Scripts/create_dragon_anims.py
# [2026-11-01 T-068] Reads  Content/Meshy/Dragon/dragon_v5_rigged.blend
# [2026-11-01 T-068] Writes Content/Meshy/Dragon/dragon_v4_<clip>.fbx per clip.
# [2026-11-01 T-068] Never saves the .blend, so the Walk action on disk and the
# [2026-11-01 T-068] bind pose are untouched. Re-running is idempotent: each
# [2026-11-01 T-068] clip is authored from a fresh open of the rig file.
# [2026-11-01 T-068] Wing axes are the T-043-proven ones (flap about X, unfold
# [2026-11-01 T-068] about Z, mirrored by side — see create_flight_anim.py's
# [2026-11-01 T-068] header for the measured geometry). Legs point -Z in bind,
# [2026-11-01 T-068] so +X rotation sweeps a leg backward (+Y, tail-ward):
# [2026-11-01 T-068] tuck = upper swept back, lower folded, foot trailed.

import math
import os

import bpy
from mathutils import Matrix, Vector

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# [2026-11-01 T-068] v5 rig: wing bones lengthened in the armature itself
# [2026-11-01 T-068] (Scripts/lengthen_wings.py) — no pose-scale tricks.
BLEND_PATH = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v5_rigged.blend")
OUT_DIR = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon")
FPS = 24

SIDE_SIGN = {"L": -1.0, "R": 1.0}
WING_BASES = ("wing_shoulder", "wing_elbow", "wing_wrist", "wing_finger")
# [2026-11-01 T-068] Leg tuck (deg about X, + = swept backward/streamlined).
# [2026-11-01 T-068] Front legs fold bird-style (upper back, forearm folded
# [2026-11-01 T-068] forward under it); hind legs trail straight back.
TUCK_FRONT = {"upper": 62.0, "lower": -95.0, "foot": 45.0}
TUCK_HIND = {"upper": 58.0, "lower": 30.0, "foot": 14.0}
TAIL_BONES = ("tail.01", "tail.02", "tail.03", "tail.04", "tail.05", "tail.06")


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
    # [2026-11-02 T-068] Wing bones are quaternion-posed by the solver below;
    # [2026-11-02 T-068] key whichever rotation property the bone actually uses.
    for bone in bones:
        bone_paths = paths or (
            ("rotation_quaternion",) if bone.rotation_mode == "QUATERNION" else ("rotation_euler",)
        )
        for path in bone_paths:
            bone.keyframe_insert(data_path=path, frame=frame)


# [2026-11-02 T-068] Rest segment directions, filled by open_rig. The solver
# [2026-11-02 T-068] interpolates rest -> straight by unfold_scale so folded
# [2026-11-02 T-068] poses (scale 0) reproduce bind exactly.
REST_DIRS = {}


def wing_pose(arm, flap_deg, unfold_scale=1.0, lag_flap=None):
    """[2026-11-02 T-068] Pose both wings as ONE straight line per side.

    The previous per-joint Euler guess (flap X + cumulative unfold Z) measured
    30-57 deg at the elbow, ~100-113 at the wrist, ~115-125 at the finger on
    the evaluated pose — a zigzag fold, however the membrane renders hid it.
    This solver instead rotates each segment, root to tip, onto a single
    target direction: lateral (straight out) tilted by the flap angle about
    the body axis. Same direction every segment = collinear by construction;
    flap is the whole straight wing sweeping like a bird's. Owner contract
    2026-11-02: tips straight out, never touching the body, fully extended.
    """
    bones = []
    for side in ("L", "R"):
        out = 1.0 if side == "L" else -1.0  # .L tips toward +X in armature space
        for base in WING_BASES:
            bone = arm.pose.bones.get(f"{base}.{side}")
            if bone is None:
                raise RuntimeError(f"required bone not found: {base}.{side}")
            bone.rotation_mode = "QUATERNION"
            flap = lag_flap[base] if lag_flap else flap_deg
            # [2026-11-02 T-068] Gull slope (owner Image #2): the straight
            # [2026-11-02 T-068] wing line sits 15 deg below horizontal at
            # [2026-11-02 T-068] flap=0 — shoulder apex, tips out and down.
            phi = math.radians(flap - 15.0 * unfold_scale)
            straight = Vector((out * math.cos(phi), 0.0, math.sin(phi)))
            rest = REST_DIRS[f"{base}.{side}"]
            target = rest.slerp(straight, unfold_scale) if unfold_scale < 1.0 else straight
            bpy.context.view_layer.update()
            head, tail = Vector(bone.head), Vector(bone.tail)
            current = (tail - head).normalized()
            correction = current.rotation_difference(target.normalized())
            bone.matrix = (
                Matrix.Translation(head)
                @ correction.to_matrix().to_4x4()
                @ Matrix.Translation(-head)
                @ bone.matrix
            )
            bones.append(bone)
    bpy.context.view_layer.update()
    return bones


def leg_pose(arm, tuck_scale=1.0, front=None, hind=None):
    front = front or TUCK_FRONT
    hind = hind or TUCK_HIND
    bones = []
    for side in ("L", "R"):
        for kind, pose in (("frontleg", front), ("hindleg", hind)):
            for part in ("upper", "lower", "foot"):
                bone = pb(arm, f"{kind}_{part}.{side}")
                adduct = 16.0 if part == "upper" else 0.0  # [2026-11-02 T-068] hind too: trail along tail, not splayed
                set_rot(bone, x=pose[part] * tuck_scale, y=(-SIDE_SIGN[side]) * adduct * tuck_scale)
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
    # [2026-11-02 T-068] Bird flap: a single angle for the whole straight
    # [2026-11-02 T-068] wing (+-40 deg). Per-joint swings/lags were the fold.
    for frame in (1, 7, 13, 19, 24):
        lag_flap = {b: 32.0 * sine(frame, span) for b in WING_BASES}
        keyed = wing_pose(arm, 0, 1.0, lag_flap) + leg_pose(arm, 1.0) + tail_wave(arm, frame, span, 4.0)
        bob = -0.03 * sine(frame, span)
        pelvis.location = (pbind[0], pbind[1], pbind[2] + bob)
        set_rot(chest, x=3.0 * sine(frame, span))
        set_rot(neck, x=-2.0 * sine(frame, span))  # head stabilisation
        key(keyed + [chest, neck], frame)
        key([pelvis], frame, ("location",))
    export(arm, action, "dragon_v4_flight.fbx")


def clip_glide():
    span, end = 47, 48
    arm, action = open_rig("Glide", end)
    chest = pb(arm, "chest")
    for frame in (1, 13, 25, 37, 48):
        # held spread with micro-corrections only — never a slow flap
        lag_flap = {b: 2.0 * sine(frame, span) for b in WING_BASES}
        keyed = wing_pose(arm, 0, 1.0, lag_flap) + leg_pose(arm, 1.0) + tail_wave(arm, frame, span, 2.5)
        set_rot(chest, x=1.0 * sine(frame, span))
        key(keyed + [chest], frame)
    export(arm, action, "dragon_v4_glide.fbx")


def clip_hover():
    span, end = 23, 24
    arm, action = open_rig("Hover", end)
    pelvis, chest, neck, head = pb(arm, "pelvis"), pb(arm, "chest"), pb(arm, "neck"), pb(arm, "head")
    half = {"upper": 20.0, "lower": -25.0, "foot": 10.0}  # legs ready, half-deployed
    for frame in (1, 7, 13, 19, 24):
        # [2026-11-02 T-068] Hover beats deeper (+-42) but stays straight.
        lag_flap = {b: 42.0 * sine(frame, span) for b in WING_BASES}
        keyed = wing_pose(arm, 0, 1.0, lag_flap)
        keyed += leg_pose(arm, 1.0, front=half, hind=half)
        keyed += tail_wave(arm, frame, span, 3.0, base_deg=10.0)  # tail drooped as counterweight
        set_rot(chest, x=-22.0 + 4.0 * sine(frame, span))  # upright, steep body
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
        set_rot(chest, x=1.6 * s)          # breath
        set_rot(neck, x=-1.0 * s, z=2.0 * sine(frame, span, 8))
        set_rot(head, x=0.8 * s)
        key(keyed + [chest, neck, head], frame)
    export(arm, action, "dragon_v4_idle.fbx")


def clip_takeoff():
    arm, action = open_rig("Takeoff", 24)
    pelvis, chest = pb(arm, "pelvis"), pb(arm, "chest")
    pbind = tuple(pelvis.location)
    bend = {"upper": -28.0, "lower": 42.0, "foot": -18.0}
    # f1 crouch, f8 leap + first downstroke, f16 spread+tuck, f24 = Fly entry pose
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
    reach = {"upper": -12.0, "lower": 10.0, "foot": 0.0}  # legs reaching down
    poses = [
        (1, 34.0, 1.0, 0.7, None, 0.05, 6.0),
        (10, 42.0, 1.0, 0.15, reach, 0.0, 14.0),   # flare/brake
        (17, 8.0, 0.45, 0.0, None, -0.03, 4.0),    # touch
        (24, 0.0, 0.0, 0.0, None, 0.0, 0.0),       # settled, folded (bind)
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
    # inhale (f8) -> blast (f18, wings mantled, head thrown) -> recover (f36)
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
