# STRETCH-GOALS.md — castle-finale systems, scoped not scheduled

**Date:** 2026-11-01 · **Origin:** owner demo-story audit (castle-assault finale)
**Status:** sequencing note only. The task specs live in TASKS.md `## Later`
(T-094–T-099); this doc owns pull-in order, cost, and risk. Nothing here
changes the M1 exit gate in docs/PLAN.md.

## The rule

Stretch work promotes to `## Now` only when T-016 (notoriety/waves), T-018
(intro), T-019 (HUD), and T-020 (packaged build) are `[x]` and the M1 exit
gate is green. The owner moves a stretch task; agents never self-promote one.
Until then these are filed scope, not a licence to build ahead of the gate.

## Pull-in order (if development runs ahead)

Each step assumes the previous steps in its chain. Two chains run in parallel:

1. **T-099 (mage tower spike)** — cheapest first: a decision, no code. Do it
   early so no art or copy implies a magic enemy that may never exist.
   Cost: <1 session. Blocks nothing; clarifies everything downstream.
2. **T-097 (settlement burning)** — reuses T-055's burn states at building
   scale. Highest demo value per session: a burning village is legible from
   cruising altitude and charges GAMEPLAY.md §4's +6/structure against T-016.
   Cost: ~1–2 sessions after T-055. FPS risk: medium — concurrent Niagara
   fires + smoke; needs a burning-building cap and pooled emitters, measured
   with a whole village alight.
3. **T-096 (attackable castle)** — same structure-damage seam as T-097 grown
   to stone/keep/towers, plus the §4 +25 heist payoff as registered treasure.
   Cost: ~2 sessions after T-097. FPS risk: low–medium (state swaps, rubble
   meshes already exist as des-* assets for props; castle rubble variants may
   need a Meshy batch — budget check per docs/ASSET-PIPELINE.md first).
4. **T-094 (garrisons/formations)** — the expensive one. Dozens of skeletal
   actors at two castles is the single largest perf threat in this set; the
   deep dive must price instanced/cheap reps against full skeletal actors
   before any placement. Cost: ~2–3 sessions plus a humanoid asset pass
   shared with T-053. FPS risk: high until the dive's numbers land.
5. **T-095 (archers/projectiles)** — needs T-094 bodies to hold the bows and
   T-074's damage path to matter. Projectile pooling and a live-arrow cap are
   in its Accept for a reason. Cost: ~1–2 sessions after T-094/T-074.
   FPS risk: medium (projectile ticks), bounded by the cap.
6. **T-098 (mobilisation)** — last, because it is pure read-side glue over
   T-016 stars + T-094 garrisons. Also the bridge to PLAN.md M3's army event:
   the M1 "army incoming" flag (GAMEPLAY.md §4, 5★) graduates here.
   Cost: ~1 session after T-094. FPS risk: low (visibility/spawn toggles).

Total forecast for the full finale: ~8–11 sessions beyond the current board.
The demo does not need all six — T-097 alone, or T-097+T-096, already turns
"burn the villages, strike the castle" into live systemic play.

## Spec anchors already in GAMEPLAY.md (do not re-invent)

- §4 notoriety: fire vs settlement structure **+6/structure**; crown-jewel /
  castle heist **+25** (M2 row); 5★ = 150 points = army event (M3), M1
  substitutes max-gear final wave + "army incoming" flag.
- §4 gear table: 4★ spears (anti-landing reach), 5★ healer support in
  parties — organised troops are implied, never tasked until T-094.
- §5.3 adventurer crossbow: HP −4/projectile, dodgeable in flight — the
  precedent T-095 generalises. All stretch numbers beyond these are
  `[tuning: PT-THIEF/PT-WAVE]` proposals, decided in each task's deep dive.

## Demo-video warning (T-021)

T-021 requires unedited live capture with **no demo-only scripting**. A
stretch system used in the submission video must be reachable and beatable
in ordinary sandbox play (PLAN.md sandbox principle, 2026-10-31): formations
the player can ignore, castles that burn the same off-camera, mobilisation
driven by real stars. If a finale beat only works when scripted for the
camera, it belongs in a labelled target trailer — never in the T-021 capture.

## Mage tower (T-099 owns the decision)

Working assumption until T-099 closes: **there is no mage**. Castle towers
are watch towers under T-096's structure rules. Story copy, submission text,
and asset names must not promise enemy magic before that decision lands.
