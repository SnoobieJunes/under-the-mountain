# meatsuite.md — owner's manual task suite (parallel work while the loop runs)

> [2026-11-03] Companion to TASKS.md. The agent loop works TASKS.md top-down;
> this file is everything only **you** can do, in the order it unblocks the
> most agent work. Check boxes here; the board stays the agent's truth.
> Deadline: **2026-11-04 21:00 ET** (docs/PLAN.md).

## How we work in parallel (read once)

- The loop claims one `[~]` task at a time and commits it green. **Don't edit
  `Source/`, `Scripts/`, or `Config/` while a task is `[~]`** — your tree
  edits collide with its commit. Safe anytime: Meshy web app, this file,
  dropping assets into the folders named below, playtesting a *packaged*
  build (never the editor the loop may be using).
- Vetoes: if a capture/render in `Saved/Profiling/` looks wrong, write the
  filename + one line here under "Vetoes" — the loop files it as a task
  instead of you re-explaining it in chat.
- Anything you finish below: tick it and add the date. The loop reads this
  file at every pickup.

## Vetoes / notes for the loop

- (empty — write `file.png — what's wrong` lines here)

## 1. Decisions only you can make (5 minutes, unblocks the most)

- [x] **T-102 gamepad Right face: A** (2026-11-03 — tap=Tint, hold=Eat/sink, Claw-tap when T-076 lands). T-102 unblocked on the board.
- [x] **T-042 AGENTS.md edits: approved + applied 2026-11-03** (floor 37%, v1.1, package.sh line, coverage foot-gun). T-042 closed.
- [x] **Friend music credit (2026-11-03, verbatim below in S3).** Ships in T-071 ingestion + T-129 SUBMISSION.md + in-game credits.
- [x] **Meshy balance: 2000 credits confirmed by owner 2026-11-03.**
      (last *proven* balance was 500 on 2026-10-28 — the "2000 credits"
      figure was unverified — now confirmed.)

## 2. Meshy generation batch (web app, parallel-safe)

Rules that keep the loop fast: **textured FBX, Smart Topology on, ≤5k tris
for props/foliage** (docs/ASSET-PIPELINE.md — the 30k figure in
FINAL-SPRINT.md was wrong for repeated foliage), **download the same
session** (links expire ~3 days), drop into `Content/Meshy/World/` with
the exact names below. Style anchor for every prompt:
"medieval, grounded realism, game-ready, no characters".

- [x] `nat-pine-v2` — tall pine, dark needles, rough bark, windswept
- [ ] `nat-oak-v1` — broad oak, full canopy, gnarled trunk
- [ ] `nat-birch-v1` — slim birch, white bark, light leaves
- [ ] `nat-bush-berry-v1` — dense berry bush, red berries
- [ ] `nat-fern-v1` — fern cluster (ask for alpha-card fronds if offered)
- [ ] `nat-grass-clump-v1` — wild grass clump with seed heads
- [ ] `nat-rock-moss-v1` — moss-covered boulder
- [ ] `nat-stump-v1` — mossy stump with roots
- [ ] Skip well/cart — the world already has `vil-well`/`vil-cart`; only
      regenerate if you dislike them, named `vil-well-v2` / `vil-cart-v2`.
- [ ] Confirm each download landed: `ls Content/Meshy/World/nat-*`

Agent side (not yours): T-114 imports + places these, fixes the goat
texture holes, and proves it in a capture.

## 3. Audio intake (10 minutes, unblocks T-071)

**Music credit (owner-supplied 2026-11-03 — reproduce verbatim in
SUBMISSION.md and in-game credits):**

> The works listed are for use only in the Game: Under the Mountain —
> Created & Designed by Auston Leroy. The following titles are original
> compositions by Keith Vernon-Hutchinson and published by Hutchinson
> Music. Registered with PRS-UK, MCPS-UK, PPL-UK. All Rights Reserved.
> Titles: Desert Winds · The Ancient One · Ryze of the Darkness ·
> Drooon & Drooon Dark Wizard · Vikings Sounds & Samples.
> Contact: Keith Vernon-Hutchinson, Composer, Orchestrator & Producer —
> www.HutchinsonMusic.com — kvhutch@gmail.com


Current state: music/SFX sit **untracked** (`Content/Audio/` + 3 MP3s at
repo root) — a fresh clone loses them and nothing is licensed on record.

- [ ] Move/confirm masters: friend tracks belong in
      `Content/Audio/Raw/Music/` untouched; note composer + licence above.
- [ ] Say which root MP3s are keepers (`Desert Winds…`, `RyZE…`,
      `VIKINGS SOUNDS.mp3`) vs scratch: ___
- [ ] Any SFX you already own (roar/fire/flap/eat/UI)? Drop in
      `Content/Audio/Raw/` and note the source/licence per file here.

## 4. Playtest sessions only a human can do (T-030, ~30 min in PIE)

The loop cannot drive Slate input. One session covers all of these —
record pass/fail + anything that feels wrong:

- [ ] Flight feel (Scheme C): pitch/momentum at speed, hover-hold,
      walk→takeoff→flight→landing transitions, S-flare, A/D turns
- [ ] Full chimney climb hoard→peak→hoard + descent
- [ ] RMB free-look + double-click latch; D-pad Up latch
- [ ] Live Walk state on the new dragon (never yet seen in-game)
- [ ] Spawn view reads as the great chamber (not a beige box)
- [ ] N-key scent toggle hides/shows ribbons
- [ ] Ground-walk visual + flap→altitude / dive→speed with HUD up

## 5. Visual sign-offs (async, veto = follow-up task, never a blocker)

- [ ] T-123 dragon repair before/after (chest + tail holes gone, one
      uniform material read)
- [ ] T-122 sunlit settlement captures (no village in mountain shadow)
- [ ] T-125/T-126 animation capture sets (walk/flight/idle/roar/eat/tail)
- [ ] T-115/T-128 per-preset fps: default stays max fidelity (your call);
      confirm the lower preset that holds 60 is acceptable to ship named

## 6. Submission day (T-021 + T-129/T-130, deadline 21:00 ET)

- [ ] Screen Recording permission on for the capture machine
- [ ] Record the unedited run (intro exit → open-world flight → scent-led
      play → systemic response) from the **fresh packaged build**
- [ ] Double-click the fresh `.app` (LaunchServices) — judges' path, T-130
- [ ] Read `docs/SUBMISSION.md` draft; confirm credits + project-page text
- [ ] Upload/submit; paste the submission URL here: ___

## 7. The generic loop (paste this to run the agent side)

```text
/loop Read LOOP.md, then AGENTS.md. Run the task loop on TASKS.md:
pick the first eligible [ ] in ## Now (Depends done, Gated none; skip
Gated: owner/hardware; T-114 waits on my Meshy drop — if its nat-* files
are absent, skip to the next eligible task and note it here). One task
per iteration: claim [~], implement smallest complete change, test-fast
while iterating, full Scripts/gate.sh before commit, one task one
commit in the AGENTS.md message format, TASKS.md closed in the same
commit. Check meatsuite.md at each pickup for my vetoes/decisions and
apply them. Never push. Park per AGENTS.md if blocked; if two
iterations in a row produce no commit, stop and report the smallest
decision I owe. Deadline awareness: T-129 (final freeze) must start by
2026-11-04 12:00 ET — if the board cannot reach it by then, stop and
report what to cut instead of starting new scope.
```

## Review findings behind this file (2026-11-03, 3 parallel audits)

Submission-critical gaps now tasked: **T-129** final freeze/repackage
(packaged .app predates all sprint work; SUBMISSION.md didn't exist),
**T-130** judge double-click/Gatekeeper path, **T-021** re-depended on
the sprint + audio + freeze. System gaps now tasked: **T-131** session
cadence/incursions, **T-132** notoriety gear + raider loot, **T-133**
dwarf roles + crack entries, **T-134** witness-based notoriety, **T-135**
Pyrrhic end screen; **T-136/T-137** (DataAssets, README honesty) in
Later. Known debt left alone deliberately: Meshy Raw duplication (~1.9G,
never cooks), world-pack LOD/collision debt, M2 phases (levels,
streaming) undecomposed past PLAN.md — post-hackathon by the owner's
full-scope/best-effort order, not silently dropped.
