#!/usr/bin/env python3
# [2026-11-04 T-139] verify_abp.py — editor-free proof that the owner-built
# [2026-11-04 T-139] ABP_Dragon graph exists and carries the build-sheet
# [2026-11-04 T-139] contract (docs/ABP-DRAGON-BUILDSHEET.md). No Unreal
# [2026-11-04 T-139] editor, no fabricated .uasset: this script only reads.
# [2026-11-04 T-139] A .uasset stores names as UTF-8/UTF-16 strings, so the
# [2026-11-04 T-139] graph's class, skeleton, slots and clip references are
# [2026-11-04 T-139] greppable in the raw bytes. Graph *topology* (node
# [2026-11-04 T-139] wiring, 30 transitions, 0.25 s crossfades) cannot be
# [2026-11-04 T-139] proven from bytes — those stay on the build-sheet
# [2026-11-04 T-139] checklist in docs/ABP-DRAGON-BUILDSHEET.md §5 and are
# [2026-11-04 T-139] proven by the -game per-state captures (ANIMSTATE
# [2026-11-04 T-139] clip=ABP + zero BIND frames), not by this script.
# Usage: python3 Scripts/verify_abp.py [--root DIR]
# Exit: 0 = all byte/code checks pass; 1 = blocked (asset missing/failing);
#       2 = code contract broken (fix C++/sheet, not the asset).
import argparse
import os
import sys

CLIPS = [
    "SK_Dragon_v4_IdleAnim", "SK_Dragon_v4_Anim", "SK_Dragon_v4_FlyAnim",
    "SK_Dragon_v4_GlideAnim", "SK_Dragon_v4_HoverAnim", "SK_Dragon_v4_TakeoffAnim",
    "SK_Dragon_v4_LandAnim", "SK_Dragon_v4_RoarAnim", "SK_Dragon_v4_HitAnim",
]
# [2026-11-04 T-126] New families are optional in the graph until imported;
# [2026-11-04 T-126] reported as INFO, never as failures.
T126_CLIPS = [
    "SK_Dragon_v4_HitHeavyAnim", "SK_Dragon_v4_DefeatAnim", "SK_Dragon_v4_SleepIdleAnim",
    "SK_Dragon_v4_WakeAnim", "SK_Dragon_v4_LieDownAnim", "SK_Dragon_v4_FirePoseAnim",
    "SK_Dragon_v4_ClawAnim", "SK_Dragon_v4_BiteEatAnim", "SK_Dragon_v4_TailWhipAnim",
    "SK_Dragon_v4_WingBuffetAnim",
]


def contains(raw: bytes, text: str) -> bool:
    return text.encode() in raw or text.encode("utf-16-le") in raw


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    args = ap.parse_args()
    root = args.root
    failures, blocked, infos = [], [], []

    # --- code contract (must hold whether or not the asset exists) ---
    anim_h = open(os.path.join(root, "Source/UnderTheMountain/Public/Dragon/DragonAnimInstance.h"),
                  encoding="utf-8", errors="replace").read()
    for token in ("Idle", "Walk", "Fly", "Glide", "Hover", "Dive", "EDragonClipFamily",
                  "SlotForClipFamily", "ComputeSlitherAmplitudeDeg", "ComputeTailLagDegrees"):
        if token not in anim_h:
            failures.append(f"code: DragonAnimInstance.h missing {token}")
    pawn_cpp = open(os.path.join(root, "Source/UnderTheMountain/Private/Dragon/DragonCharacter.cpp"),
                    encoding="utf-8", errors="replace").read()
    for token in ("ABP_Dragon.ABP_Dragon_C", "FullBody", "UpperBody", "ClipForFamily"):
        if token not in pawn_cpp:
            failures.append(f"code: DragonCharacter.cpp missing {token}")

    # --- asset checks ---
    asset = os.path.join(root, "Content/Meshy/Dragon/ABP_Dragon.uasset")
    if not os.path.isfile(asset) or os.path.getsize(asset) == 0:
        blocked.append("asset: Content/Meshy/Dragon/ABP_Dragon.uasset missing/empty "
                       "(owner lane T-139: build per docs/ABP-DRAGON-BUILDSHEET.md)")
    else:
        raw = open(asset, "rb").read()
        print(f"ABP_VERIFY asset bytes={len(raw)} path={asset}")
        for token, label in (
            ("DragonAnimInstance", "C++ parent UDragonAnimInstance"),
            ("SK_Dragon_v6_Skeleton", "v6 skeleton"),
            ("FullBody", "FullBody slot"), ("UpperBody", "UpperBody slot"),
        ):
            if not contains(raw, token):
                blocked.append(f"asset: no '{token}' string ({label}) in ABP_Dragon.uasset")
        for clip in CLIPS:
            if not contains(raw, clip):
                blocked.append(f"asset: locomotion/one-shot clip not referenced: {clip}")
        for clip in T126_CLIPS:
            infos.append(f"asset: T-126 clip {'referenced' if contains(raw, clip) else 'not yet referenced (ok until imported)'}: {clip}")
        for var in ("LocomotionState", "FlapPlayRate", "MeshPitchDegrees", "BankDegrees",
                    "SlitherAmplitudeDeg", "TailLagDegrees", "FireAimPitchDegrees"):
            if not contains(raw, var):
                infos.append(f"asset: published var not found in bytes (may be optimised name): {var}")

    for line in infos:
        print("ABP_VERIFY INFO", line)
    for line in blocked:
        print("ABP_VERIFY BLOCKED", line)
    for line in failures:
        print("ABP_VERIFY FAIL", line)
    if failures:
        print("ABP_VERIFY RESULT code-contract-broken")
        return 2
    if blocked:
        print("ABP_VERIFY RESULT blocked-on-editor (owner graph not built / incomplete)")
        return 1
    print("ABP_VERIFY RESULT pass (byte+code checks; topology/captures still owed per sheet §5)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
