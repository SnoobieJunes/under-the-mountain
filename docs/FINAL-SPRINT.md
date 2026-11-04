# [2026-11-03] Final sprint plan — owner feedback intake

Owner-directed final sprint (~19h to the 2026-11-04 21:00 ET deadline in
docs/PLAN.md). Owner decisions recorded 2026-11-03: (1) full scope stands —
no deferrals, best effort in board order; (2) dragon repair is surgical on
the current v6, not a regeneration; (3) the world map is rebuilt
intelligently as a normal medieval world (the village sat in the shadow of
a poorly placed mountain mass); (4) god mode ships now plus a flight
stamina retune; (5) shipped graphics default is highest quality, with
lower presets available for weaker machines.

## Answers to the owner's two questions (proven from the tree)

- **God mode / infinite stamina: specced, never built.** The rule lives in
  docs/GAMEPLAY.md §2 ("Testing: Infinite Stamina (God Mode)") and the
  Settings toggle in T-088's Accept. A grep of Source/ for
  InfiniteStamina/GodMode finds zero implementation. There is no console
  command today. T-124 ships the testable slice first (cvar + Stats gate);
  T-088 keeps the Settings-screen toggle.
- **Animations not attached to the harness: there is no harness yet.** No
  ABP_Dragon asset exists. The pawn plays clips single-node
  (DragonCharacter.cpp anim driver, ~line 3204): 9 clip slots are wired —
  Idle, Walk, Fly, Glide, Hover, Takeoff, Land, Roar, Hit (light). Not
  attached / not existing: Hit heavy, Defeat, Sleep Idle + Wake + Lie Down,
  fire-breath pose + head aim, claw montage, bite/eat sync. Wing buffet and
  tail slam are full rule verbs (X / Z) with **no clips at all**. Walk and
  Fly are attached but are the clips the owner flags for quality. That is
  6 missing clip families + 2 clip-less verbs + 2 quality redo families
  against the adopted v1 set in docs/ANIMATION-SPIKE.md.

## Flight: which option worked better

Proven (T-075 course evidence, docs/course-evidence/scheme{1,2,3}.csv):
Scheme C (hybrid momentum + precision, the play.sh default) won — A
climbs/turns but cannot hover by design, B cannot fly (forced precision).
"Can't stay airborne" is therefore tuning, not scheme: hover band drains
8/s (~12 s of hover per pool), Ascent/Surge cost one 20-point reserve
each, and nothing regenerates in the air. T-124 retunes per the owner
pick (hover ~3/s, cheaper impulses, glide stays 0/s).

## Sprint order (## Now, top first)

1. T-124 god mode + flight retune (smallest, unblocks all playtesting).
2. T-125 ABP_Dragon harness (attaches what exists, with blends).
3. T-123 dragon v6 surgical repair (Blender; rig + 9 clips untouched).
4. T-126 animation clips finish (walk/flight/idle/roar/eat/tail + slither).
5. T-122 world map rebuild (medieval layout, sunlit villages).
6. T-127 scent density + gold sheen; T-128 graphics presets (max default).
7. Then board order resumes (T-068 closes on T-125+T-126; T-115, T-088…).

## Owner-parallel Meshy prompts (vegetation + set dressing)

Generate in Meshy (text-to-3D, textured, low-poly-realistic, Style B
grounded realism per docs/PLAN.md). Keep each under ~30k tris. Drop
exports in Content/Meshy/World/ with the `nat-` prefix so T-114 places
them through the existing chain.

1. `nat-pine-v2` — "tall medieval pine tree, dark green needle clusters,
   rough brown bark, slightly windswept, game-ready, realistic"
2. `nat-oak-v1` — "broad medieval oak tree, full green canopy, gnarled
   trunk, game-ready, realistic foliage"
3. `nat-birch-v1` — "slim birch tree, white bark, light green leaves,
   game-ready, realistic"
4. `nat-bush-berry-v1` — "dense berry bush with red berries, medieval
   forest undergrowth, game-ready"
5. `nat-fern-v1` — "forest fern cluster, lush green fronds, ground cover,
   game-ready"
6. `nat-grass-clump-v1` — "wild grass clump with seed heads, meadow,
   game-ready ground cover"
7. `nat-rock-moss-v1` — "moss-covered boulder, grey stone, damp green
   patches, game-ready"
8. `nat-stump-v1` — "old mossy tree stump with roots, forest floor,
   game-ready"
9. `nat-hay-cart-v1` — "medieval wooden hand cart loaded with hay bales,
   village prop, game-ready"
10. `nat-well-v2` — "medieval stone village well with wooden roof and
    bucket, game-ready"

Dragon/goat repair needs **zero** Meshy credits (owner pick: surgical
Blender repair). The 2000-credit budget stays untouched reserve unless a
sprint task records a spend decision first.

## Meshy/Muse credit rule for this sprint

Concept images stay on Muse Image (0 Meshy credits, docs/PLAN.md).
Meshy spends only on non-dragon meshes/textures, only after a balance
check (meshy_check_balance) and only inside a claimed task.
