# [2026-10-28 T-043] create_flight_anim.py — author a looping 1 s wing-flap
# [2026-10-28 T-043] ("Fly") action for the dragon rig and export it for Unreal.
# [2026-10-28 T-043] Run (Blender 5.2, headless, from the repo root):
# [2026-10-28 T-043]   Blender -b --python Scripts/create_flight_anim.py
# [2026-10-28 T-043] Reads  Content/Meshy/Dragon/dragon_v4_rigged.blend
# [2026-10-28 T-043] Writes Content/Meshy/Dragon/dragon_v4_flight.fbx (Fly only)
# [2026-10-28 T-043] It renders nothing and never saves the .blend, so the rig
# [2026-10-28 T-043] file and any existing walk action are left untouched on disk.
# [2026-10-28 T-043] Re-running is idempotent: an old "Fly" action is replaced.

import math
import os

import bpy

# [2026-10-28 T-043] Repo root from this script's location (Scripts/..), not
# [2026-10-28 T-043] bpy.path.abspath("//"), which headless runs cannot rely on
# [2026-10-28 T-043] before a mainfile is opened.
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLEND_PATH = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v4_rigged.blend")
FBX_PATH = os.path.join(REPO_ROOT, "Content", "Meshy", "Dragon", "dragon_v4_flight.fbx")

FRAME_START = 1
FRAME_END = 24
FPS = 24
# [2026-10-28 T-043] UP at 1/24, mid at 7/19, DOWN at 13. The sine period is
# [2026-10-28 T-043] FRAME_END-FRAME_START (23), so frame 24 samples exactly one
# [2026-10-28 T-043] full period after frame 1: first and last keys are equal and
# [2026-10-28 T-043] the cycle loops seamlessly for every (even lagged) bone.
KEY_FRAMES = (1, 7, 13, 19, 24)
LOOP_SPAN = FRAME_END - FRAME_START

# [2026-10-28 T-043] (bone base name, flap peak-to-peak swing in degrees,
# [2026-10-28 T-043] phase lag in frames, unfold in degrees). Each joint lags
# [2026-10-28 T-043] its parent by 2 frames so the flap travels
# [2026-10-28 T-043] shoulder -> finger like membrane follow-through. The
# [2026-10-28 T-043] unfold is a constant (non-oscillating) sideways swing.
WING_CHAIN = (
    ("wing_shoulder", 70.0, 0, 55.0),
    ("wing_elbow", 28.0, 2, 30.0),
    ("wing_wrist", 20.0, 4, 15.0),
    ("wing_finger", 14.0, 6, 0.0),
)
# [2026-10-28 T-043] Verified wing geometry (dragon_v4_rigged.blend):
# [2026-10-28 T-043] wing_shoulder.L head (0.09, -0.39, 0.05) ->
# [2026-10-28 T-043] wing_finger.L tail (0.10, 0.04, 0.06): the chain runs
# [2026-10-28 T-043] +Y, folded along the body. Rotating about Y barely moves
# [2026-10-28 T-043] the tip (measured 7 cm over the cycle; wings stay folded
# [2026-10-28 T-043] in renders), so the flap cannot be about Y.
# [2026-10-28 T-043] Flap about X: rotating a +Y chain about X
# [2026-10-28 T-043] (rotation_euler[0]) pitches the tip up/down in Z. The
# [2026-10-28 T-043] same sign is used for .L and .R so both tips lift
# [2026-10-28 T-043] together; there is no sign mirroring on the flap.
# [2026-10-28 T-043] Unfold about Z: a constant rotation_euler[2] offset
# [2026-10-28 T-043] swings the folded chain outward sideways. Rotating +Y
# [2026-10-28 T-043] about Z by +angle moves the tip toward -X, so .L (at
# [2026-10-28 T-043] +X) takes -55/-30/-15 and .R takes +55/+30/+15 degrees
# [2026-10-28 T-043] (shoulder/elbow/wrist; finger 0). With SIDE_SIGN below,
# [2026-10-28 T-043] euler[2] = SIDE_SIGN * radians(unfold): .L sign -1 ->
# [2026-10-28 T-043] -unfold -> tip toward +X; .R sign +1 -> +unfold -> tip
# [2026-10-28 T-043] toward -X. Per-bone euler: [flap_about_X, 0, unfold_about_Z].
SIDE_SIGN = {"L": -1.0, "R": 1.0}
FLAP_AXIS = 0  # [2026-10-28 T-043] X in rotation_euler (XYZ order)
UNFOLD_AXIS = 2  # [2026-10-28 T-043] Z in rotation_euler (XYZ order)
PELVIS_BOB = 0.03  # +/-3 cm; rig is ~1.9 Blender units long
CHEST_PITCH_DEG = 3.0  # counter-pitch amplitude, about X -> rotation_euler[0]
TAIL_BONES = ("tail.01", "tail.02", "tail.03")
TAIL_SWAY_DEG = 4.0  # pitch amplitude about X, 2-frame lag per segment


def flap_value(frame, lag_frames):
    """[2026-10-28 T-043] Sine of the flap cycle: +1 (UP) at frames 1/24, -1 at 13."""
    phase = 2.0 * math.pi * (frame - FRAME_START - lag_frames) / LOOP_SPAN
    return math.sin(phase + math.pi / 2.0)


def pose_bone(armature, name):
    """[2026-10-28 T-043] Fetch a pose bone or fail loudly — never key silently."""
    bone = armature.pose.bones.get(name)
    if bone is None:
        raise RuntimeError(f"required bone not found in rig: {name}")
    return bone


def main():
    bpy.ops.wm.open_mainfile(filepath=BLEND_PATH)
    scene = bpy.context.scene
    scene.frame_start = FRAME_START
    scene.frame_end = FRAME_END
    scene.render.fps = FPS
    scene.render.fps_base = 1.0

    armature = next((o for o in bpy.data.objects if o.type == "ARMATURE"), None)
    if armature is None:
        raise RuntimeError(f"no ARMATURE object found in {BLEND_PATH}")

    # [2026-10-28 T-043] Replace any previous Fly action (idempotent re-runs);
    # [2026-10-28 T-043] existing actions such as the walk cycle are not touched.
    old_action = bpy.data.actions.get("Fly")
    if old_action is not None:
        if armature.animation_data and armature.animation_data.action == old_action:
            armature.animation_data.action = None
        bpy.data.actions.remove(old_action, do_unlink=True)
    action = bpy.data.actions.new(name="Fly")
    if armature.animation_data is None:
        armature.animation_data_create()
    armature.animation_data.action = action

    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="POSE")

    # [2026-10-28 T-043] Start from bind pose so unkeyed bones (legs, most of the
    # [2026-10-28 T-043] tail, neck, head) stay at bind in the baked FBX.
    for bone in armature.pose.bones:
        bone.rotation_euler = (0.0, 0.0, 0.0)
        bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
        bone.location = (0.0, 0.0, 0.0)
        bone.scale = (1.0, 1.0, 1.0)

    # [2026-10-28 T-043] (side, base) -> (bone, swing_deg, lag, unfold_sign, unfold_deg)
    wings = {}
    for side, unfold_sign in SIDE_SIGN.items():
        for base, swing_deg, lag, unfold_deg in WING_CHAIN:
            bone = pose_bone(armature, f"{base}.{side}")
            bone.rotation_mode = "XYZ"
            wings[(side, base)] = (bone, swing_deg, lag, unfold_sign, unfold_deg)
    pelvis = pose_bone(armature, "pelvis")
    pelvis.rotation_mode = "XYZ"
    pelvis_bind = tuple(pelvis.location)
    chest = pose_bone(armature, "chest")
    chest.rotation_mode = "XYZ"
    tails = []
    for index, name in enumerate(TAIL_BONES):
        bone = pose_bone(armature, name)
        bone.rotation_mode = "XYZ"
        tails.append((bone, 2 * (index + 1)))  # lag grows 2 frames per segment

    for frame in KEY_FRAMES:
        keyed = []
        for bone, swing_deg, lag, unfold_sign, unfold_deg in wings.values():
            # [2026-10-28 T-043] Peak-to-peak swing -> half is the sine amplitude
            # [2026-10-28 T-043] about bind (e.g. shoulder: -35..+35 degrees).
            # [2026-10-28 T-043] Flap about X, same sign for .L/.R (no mirroring);
            # [2026-10-28 T-043] unfold about Z is constant, mirrored by side.
            flap_angle = math.radians(swing_deg / 2.0) * flap_value(frame, lag)
            unfold_angle = unfold_sign * math.radians(unfold_deg)
            euler = [0.0, 0.0, 0.0]
            euler[FLAP_AXIS] = flap_angle
            euler[UNFOLD_AXIS] = unfold_angle
            # [2026-10-28 T-043] Result: [flap_about_X, 0, unfold_about_Z].
            bone.rotation_euler = euler
            keyed.append((bone, "rotation_euler"))
        # [2026-10-28 T-043] Body bobs opposite the wings (rises on the downstroke)
        # [2026-10-28 T-043] and the chest counter-pitches against the flap.
        bob = -PELVIS_BOB * flap_value(frame, 0)
        pelvis.location = (pelvis_bind[0], pelvis_bind[1], pelvis_bind[2] + bob)
        keyed.append((pelvis, "location"))
        chest.rotation_euler = (math.radians(CHEST_PITCH_DEG) * flap_value(frame, 0), 0.0, 0.0)
        keyed.append((chest, "rotation_euler"))
        for bone, lag in tails:
            sway = math.radians(TAIL_SWAY_DEG) * flap_value(frame, lag)
            bone.rotation_euler = (sway, 0.0, 0.0)
            keyed.append((bone, "rotation_euler"))
        for bone, data_path in keyed:
            bone.keyframe_insert(data_path=data_path, frame=frame)

    bpy.ops.object.mode_set(mode="OBJECT")
    # [2026-10-28 T-043] Export ONLY the Fly take: with Blender 5.x
    # slotted actions the exporter needs bake_anim_use_all_actions,
    # but the rig's Walk take contains _end helper-bone tracks UE
    # rejects (import raises). Removing other actions in memory
    # only — the .blend on disk is never saved by this script.
    for other in list(bpy.data.actions):
        if other != action:
            bpy.data.actions.remove(other, do_unlink=True)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in scene.objects:
        if obj.type in {"ARMATURE", "MESH"}:
            obj.select_set(True)
    bpy.context.view_layer.objects.active = armature

    os.makedirs(os.path.dirname(FBX_PATH), exist_ok=True)
    # [2026-10-28 T-043] Same export family as the walk FBX (dragon_v4_walk.fbx):
    # [2026-10-28 T-043] selection only, baked animation, active action only so the
    # [2026-10-28 T-043] walk cycle is never baked into the flight FBX.
    bpy.ops.export_scene.fbx(
        filepath=FBX_PATH,
        use_selection=True,
        bake_anim=True,
        bake_anim_use_all_actions=True,
        axis_forward="-Y",
        axis_up="Z",
        apply_unit_scale=True,
    )
    # [2026-10-28 T-043] Deliberately no save: the .blend on disk stays unchanged.
    print(f"FLIGHT_FBX_WRITTEN {FBX_PATH}")


if __name__ == "__main__":
    main()
