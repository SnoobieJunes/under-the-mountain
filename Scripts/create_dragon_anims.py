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
# [2026-11-01 T-068] Reads  Content/Meshy/Dragon/dragon_v4_rigged.blend
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

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLEND_PATH = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v4_rigged.blend")
OUT_DIR = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon")
FPS = 24

SIDE_SIGN = {"L": -1.0, "R": 1.0}
WING_BASES = ("wing_shoulder", "wing_elbow", "wing_wrist", "wing_finger")
# [2026-11-01 T-068] Full-spread unfold (deg about Z): shoulder-driven so the
# [2026-11-01 T-068] wing reads as one straight span, not a zig-zag. T-043 used
# [2026-11-01 T-068] 55/30/15 (cumulative 100); v2 first used 68/22/8, still
# [2026-11-01 T-068] too tight (tips beside hips); 85/30/12 overshot into a
# [2026-11-01 T-068] dart sweep on the top view; settled 70/24/10 (cumulative
# [2026-11-01 T-068] 104, near-lateral) + STRETCH below for the span.
SPREAD = {"wing_shoulder": 70.0, "wing_elbow": 24.0, "wing_wrist": 10.0, "wing_finger": 0.0}
# [2026-11-01 T-068] Wing stretch (pose-bone Y scale at full spread). The rig's
# [2026-11-01 T-068] wing chain is 0.43 units on a 1.9-unit body, so rotation
# [2026-11-01 T-068] alone puts the tips beside the hips at ~1.0 span — the
# [2026-11-01 T-068] owner-visible "tips attached to hips" defect. Scaling each
# [2026-11-01 T-068] segment (compounding down the chain) extends the span in
# [2026-11-01 T-068] flight only; ground clips key no scale, so the folded
# [2026-11-01 T-068] bind silhouette is untouched. Verified by top-view renders
# [2026-11-01 T-068] + deformed-mesh span measurement (probe in commit notes).
STRETCH = {"wing_shoulder": 1.25, "wing_elbow": 1.30, "wing_wrist": 1.30, "wing_finger": 1.35}
# [2026-11-01 T-068] Leg tuck (deg about X, + = swept backward/streamlined).
# [2026-11-01 T-068] Front legs fold bird-style (upper back, forearm folded
# [2026-11-01 T-068] forward under it); hind legs trail straight back.
TUCK_FRONT = {"upper": 62.0, "lower": -95.0, "foot": 45.0}
TUCK_HIND = {"upper": 48.0, "lower": 38.0, "foot": 18.0}
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


def key(bones, frame, paths=("rotation_euler", "scale")):
    for bone in bones:
        for path in paths:
            bone.keyframe_insert(data_path=path, frame=frame)


def wing_pose(arm, flap_deg, unfold_scale=1.0, lag_flap=None):
    """Pose both wings: flap about X (deg per joint via lag_flap dict or scalar),
    unfold about Z scaled from SPREAD. Returns keyed bones."""
    bones = []
    for side, sign in SIDE_SIGN.items():
        for base in WING_BASES:
            bone = pb(arm, f"{base}.{side}")
            flap = lag_flap[base] if lag_flap else flap_deg
            set_rot(bone, x=flap, z=sign * SPREAD[base] * unfold_scale)
            # stretch grows/shrinks with the unfold so takeoff/land ease it
            s = 1.0 + (STRETCH[base] - 1.0) * unfold_scale
            bone.scale = (1.0, s, 1.0)
            bones.append(bone)
    return bones


def leg_pose(arm, tuck_scale=1.0, front=None, hind=None):
    front = front or TUCK_FRONT
    hind = hind or TUCK_HIND
    bones = []
    for side in ("L", "R"):
        for kind, pose in (("frontleg", front), ("hindleg", hind)):
            for part in ("upper", "lower", "foot"):
                bone = pb(arm, f"{kind}_{part}.{side}")
                adduct = 16.0 if kind == "frontleg" and part == "upper" else 0.0
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
    swings = {"wing_shoulder": 72.0, "wing_elbow": 30.0, "wing_wrist": 20.0, "wing_finger": 14.0}
    lags = {"wing_shoulder": 0, "wing_elbow": 2, "wing_wrist": 4, "wing_finger": 6}
    for frame in (1, 7, 13, 19, 24):
        lag_flap = {b: (swings[b] / 2) * sine(frame, span, lags[b]) for b in WING_BASES}
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
        lag_flap = {"wing_shoulder": 3.0 * sine(frame, span), "wing_elbow": 2.0 * sine(frame, span, 3),
                    "wing_wrist": 4.0 * sine(frame, span, 6), "wing_finger": 3.0 * sine(frame, span, 9)}
        keyed = wing_pose(arm, 0, 1.0, lag_flap) + leg_pose(arm, 1.0) + tail_wave(arm, frame, span, 2.5)
        set_rot(chest, x=1.0 * sine(frame, span))
        key(keyed + [chest], frame)
    export(arm, action, "dragon_v4_glide.fbx")


def clip_hover():
    span, end = 23, 24
    arm, action = open_rig("Hover", end)
    pelvis, chest, neck, head = pb(arm, "pelvis"), pb(arm, "chest"), pb(arm, "neck"), pb(arm, "head")
    swings = {"wing_shoulder": 95.0, "wing_elbow": 40.0, "wing_wrist": 26.0, "wing_finger": 18.0}
    lags = {"wing_shoulder": 0, "wing_elbow": 2, "wing_wrist": 4, "wing_finger": 6}
    half = {"upper": 20.0, "lower": -25.0, "foot": 10.0}  # legs ready, half-deployed
    for frame in (1, 7, 13, 19, 24):
        lag_flap = {b: (swings[b] / 2) * sine(frame, span, lags[b]) for b in WING_BASES}
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
