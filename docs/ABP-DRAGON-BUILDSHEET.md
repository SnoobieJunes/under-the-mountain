# ABP_Dragon build sheet (T-139, owner lane)

> [2026-11-04 T-125] Click-by-click spec for building ABP_Dragon in the
> open editor. The agent lane (T-125) proved programmatic graph
> authoring is unavailable (probes 1–3 + MCP BlueprintTools: asset
> create/compile/save work headless, node placement needs Slate, and no
> state-machine/montage API exists) — so the graph is a ~20–30 minute
> owner build and the loop verifies + closes T-139 with per-state
> captures. C++ already drives everything: the graph READS published
> variables, it invents no rules (docs/ANIMATION-SPIKE.md).

## 0. Asset setup (2 min)

1. In `Content/Meshy/Dragon/` (content browser path `/Game/Meshy/Dragon/`),
   create **Animation Blueprint** named exactly `ABP_Dragon`.
2. Parent class: `UDragonAnimInstance` (the C++ parent — pick the C++
   class, not a BP).
3. Target skeleton: `SK_Dragon_v6_Skeleton`.
4. Compile + save. From the next run on, the pawn auto-detects the graph
   (`Dragon anim path: ABP_Dragon` in the log) and the single-node driver
   goes quiet — no code changes needed on your side.

No EventGraph work: `NativeUpdateAnimation` (C++) publishes every input
the graph reads. Everything below is the AnimGraph.

## 1. AnimGraph chain (top to bottom)

```text
OutputPose
  └─ RotateRootBone (Pitch = MeshPitchDegrees, Roll = BankDegrees)
       └─ LayeredBoneBlend (base + UpperBody slot, branch: chest)
            ├─ Base: Slot 'FullBody'
            │         └─ StateMachine 'Locomotion'
            └─ Blend: Slot 'UpperBody'
```

- **Slot names must be exactly** `FullBody` and `UpperBody` (C++ plays
  one-shots through them; the `Dragon.ABPFallback` test pins the names).
- **LayeredBoneBlend**: one branch filter on bone `chest`, blend depth 0
  (covers chest + neck + head + wings — roar mantles with the chest).
  If the wings do not follow in the preview, move the filter up to
  `spine2` and note it — do not guess twice.
- **RotateRootBone**: whole-body bank/pitch in-pose (replaces the C++
  mesh tilt 1:1). Start with Pitch = `MeshPitchDegrees`,
  Roll = `BankDegrees`; check the lean direction against the T-113
  flight capture (`Saved/Profiling/t113_fly_proof.png`) and negate the
  mismatched axis if it reads mirrored.
- **Fold/Tuck**: NOT separate nodes in this slice — the baked clips
  already satisfy the fold/tuck contract (verified in
  `Dragon.AnimCurves`). `FoldAlpha`/`TuckAlpha` are published and
  reserved: T-126 wires them to an ApplyAdditive chain when additive
  clips exist. Do not invent procedural fold/tuck here.

## 2. Locomotion state machine (12 min)

Six states, each a single SequencePlayer wired to `FlapPlayRate`
(the C++ curve returns 1.0 for ground states, so one wiring fits all):

| State   | Clip (all `/Game/Meshy/Dragon/`)      | Note                              |
|---------|---------------------------------------|-----------------------------------|
| Idle    | `SK_Dragon_v4_IdleAnim`               | entry state                       |
| Walk    | `SK_Dragon_v4_Anim`                   | (the walk clip keeps its old name) |
| Fly     | `SK_Dragon_v4_FlyAnim`                |                                   |
| Glide   | `SK_Dragon_v4_GlideAnim`              |                                   |
| Hover   | `SK_Dragon_v4_HoverAnim`              |                                   |
| Dive    | `SK_Dragon_v4_GlideAnim`              | no Dive clip exists — Glide held near-frozen by the 0.15 rate |

Transitions: **every ordered pair** (30 transitions) with rule
`LocomotionState == <TargetState>` (the published C++ enum — equality
node against the enum value), crossfade **0.25 s** (spike band
0.2–0.35 s). Entry → Idle. No custom transition logic anywhere: if a
transition needs more than the equality test, stop and flag it — the
rule belongs in C++ (`SelectLocomotionState`), not in the graph.

## 3. One-shots: no graph work (awareness only)

C++ plays all four through the slots as dynamic montages (blend
0.15 s in / 0.25 s out) from the existing call sites — no montage
assets to author (real montages with notifies land in T-126+ when a
one-shot needs gameplay sync):

| Clip (`/Game/Meshy/Dragon/`) | Slot       | Trigger (already wired)         |
|------------------------------|------------|---------------------------------|
| `SK_Dragon_v4_TakeoffAnim`   | FullBody   | leaving ground                  |
| `SK_Dragon_v4_LandAnim`      | FullBody   | touchdown (no damage)           |
| `SK_Dragon_v4_RoarAnim`      | UpperBody  | roar verb                       |
| `SK_Dragon_v4_HitAnim`       | UpperBody  | damaging landing (light hit)    |

## 4. Done = compile + save + tell the loop

1. Compile (0 errors, 0 warnings) and **save** the asset (Ctrl+S —
   headless verification reads the `.uasset` from disk).
2. Quick preview check: scrub Idle → Fly → Glide → Hover → Dive in the
   ABP preview viewport; every state must show a posed dragon (never
   bind pose); transitions must crossfade, not pop.
3. Reply here or flip T-139's `Gated: owner` to `none` — the loop then
   runs the per-state `-game` captures, reviews them, and closes T-139
   (which closes the ABP clause of T-068).
