# ANIMATION-SOURCES.md — T-059 CC0 animation survey (report only)

**Date:** 2026-11-01 · **Task:** T-059 · **Bar:** CC0 only (owner decision 2026-10-31)
**Scope:** survey only. Nothing was downloaded into `Content/`, imported, or
adopted. Adoption of any item below is a separate owner-approved task.
Evidence was read from source pages on the date above; licence claims are
quoted from those pages, not from aggregator summaries, except where marked.

## Judging criteria (from docs/ANIMATION-SPIKE.md §D)

- Judge mainly for **locomotion value**: idle / walk / trot / gallop / lie-down.
- Format: FBX/BVH/Blend with named bones and a documented rest pose.
- Any **flight** claim needs ≥3 wing joints to be credible against our wing.
- Retarget baseline (spike): 0.5–1 session setup per source (two IK Rigs,
  chain mapping, rest-pose offsets, scale, root conventions), then minutes
  per usable clip. Numbers below adjust that baseline per source.

Our target rig, `SK_Dragon_v4` (33 bones, T-029): pelvis/spine/chest/neck/head,
wing chains `wing_shoulder/elbow/wrist/finger` L+R (4 joints, one finger),
4 leg chains × 3, tail `.01–.06`. Bind pose is a **folded/perched** pose, which
is the single biggest retarget cost against every source below: all of them
bind in a standing/spread pose, so rest-pose offsets are unavoidable.

## Verdict up front

CC0 supply is **real for quadruped locomotion, thin for winged flight**.
Of six sources examined, two are credible keeps — both from one author
(Quaternius). That meets the survey's search count but concentrates risk:
there is no second locomotion supplier at this licence bar, and **no source
shares our folded-bind, four-joint, one-finger wing**, so flight and combat
clips stay Blender-authored exactly as the spike adopted.

## Sources examined

### 1. Quaternius — Ultimate Animated Animal Pack — **KEEP (primary)**

- **URL:** https://quaternius.com/packs/ultimateanimatedanimals.html
- **Licence evidence:** pack page states License: CC0, linking
  creativecommons.org/publicdomain/zero/1.0/; multiple independent project
  ledgers record the same `License.txt` (CC0 1.0 Universal) inside the pack.
- **Contents:** 12 quadrupeds (horse, wolf/dogs, deer/stag, cow/bull, fox,
  donkey, alpaca…), each with 12+ animations per the pack page: Attack,
  Death, Kicks, Gallops, Walk, Jump "and many more". Third-party ledgers
  enumerate Idle, Walk, Run, Gallop among them.
- **Formats:** FBX, OBJ, glTF, Blend (pack page).
- **Skeleton compatibility:** standard quadruped chains (spine, neck/head,
  4×3 legs, tail on most) map cleanly onto our leg/spine/tail chains by role.
  No wings — locomotion only, which is the judged category. Exact bone names
  and rest pose are not published on the page; verify at adoption.
- **Retarget cost:** spike baseline (0.5–1 session) + rest-pose offset work
  for our folded bind. Gallop/trot cycles are the prize: they serve the
  spike's "Later locomotion depth" (trot/gallop, start/stop/turn) and could
  reference-check the shipped Walk.
- **Risk:** low-poly cute proportions — retargeted timing may read "cute" on
  the hero mesh. The spike's contact-sheet/capture gates decide; budget a
  timing cleanup pass per clip, not zero-touch reuse.

### 2. Quaternius — Ultimate Monsters: Dragon + Dragon Evolved — **KEEP (secondary)**

- **URLs:** https://poly.pizza/m/VBvzjFIYws (Dragon),
  https://poly.pizza/m/LlwD0QNUPj (Dragon Evolved),
  pack page https://quaternius.com/packs/ultimatemonsters.html
- **Licence evidence:** Poly Pizza model pages state "Public Domain (CC0)"
  with the CC0 deed link; the bundle page repeats CC0 for all 50 monsters.
- **Contents:** two winged quadruped dragons in the pack's "Flying" rig
  family. Published clip names come from third-party integrations of the
  actual files (marked as such): `Flying_Idle`, `Fast_Flying`, plus the
  family vocabulary Idle/Walk/Run/Attack/Death/HitReact (~8 clips per rig).
- **Formats:** FBX, glTF.
- **Skeleton compatibility:** the only CC0 winged candidate found. Wing joint
  count is **not published**; low-poly monster wings are unlikely to match
  our 4-joint chain 1:1, so per spike §D the flight claim is **weak until
  bones are inspected at adoption**. Ground/posture clips are usable
  regardless.
- **Retarget cost:** baseline + wing chain remapping + folded-bind offsets.
  Value is mainly as flap-timing/posture reference for Fly v2 (T-058) and
  held Glide, with direct retarget as a bonus if chains align.
- **Risk:** same cute-style timing caveat as source 1, stronger here.

### 3. Kenney — Cube Pets (and catalogue generally) — **DROP for the dragon**

- **URL:** https://kenney.nl/assets/cube-pets
- **Licence evidence:** asset page states "Creative Commons CC0"; Kenney's
  catalogue-wide CC0 stance is well attested.
- **Contents:** 16 cube-proportioned pets with walk/run-style clips (v2.0).
- **Why drop:** cube body proportions and stub rigs cannot retarget onto a
  long-spined 33-bone dragon without gait-destroying offsets. Kenney's
  realistic-proportion animal packs are static. Examined and set aside;
  Kenney remains a fine CC0 source for non-animated needs elsewhere.

### 4. KayKit (Kay Lousberg) — **DROP for the dragon**

- **URL:** https://www.kaylousberg.com/game-assets/character-animations
- **Licence evidence:** author's page: "Free for personal and commercial
  use, no attribution required. (CC0 Licensed)".
- **Contents:** humanoid (`Rig_Medium`) animation library + biped character
  packs, FBX/glTF.
- **Why drop:** no quadruped or winged content exists in the line. Noted
  only because it is the other major CC0 animation supplier — its absence
  from the quadruped category is part of the shortfall picture.

### 5. OpenGameArt — per-asset dragons — **DROP**

- **URLs:** https://opengameart.org/node/76785 (Mazo Dragon, CC0 .blend);
  Cethiel's "Dragon — Fully Animated" via opengameart.org.
- **Licence evidence:** per-asset licence fields on each page (CC0 shown for
  Mazo). OpenGameArt mixes licences per upload; every asset needs its own
  check — there is no catalogue-level guarantee.
- **Why drop:** Cethiel's dragon is 2D animation artwork, not a polygonal
  model (confirmed on its own discussion page) — unusable in-engine. Mazo
  is a humanoid/furry dragon build; wrong body plan for retargeting.

### 6. BlendSwap — "Low-Poly Dragon" by tastyfish — **DROP (examined)**

- **URL:** https://blendswap.com/blend/22805
- **Licence evidence:** BlendSwap licence field on the page: CC0; author
  text confirms the release.
- **Contents:** one rigged+animated low-poly dragon `.blend` (Blender 2.7
  era, 463 KB), derived from Cethiel's 2D dragon. Clip list, bone names,
  and rest pose are not published.
- **Why drop:** single hobbyist asset of unknown clip inventory; the
  verification cost (open, inventory, chain-map an undocumented rig) exceeds
  its expected value next to source 2, which covers the same niche with a
  maintained pack behind it. Revisit only if source 2's wings disappoint.

**Channel note — Poly Pizza:** useful discovery/download channel for the
Quaternius keeps, but licences are **per-asset mixed CC0/CC-BY**; its CC0
filter must be applied and the model page licence re-read at adoption time.

## Integration for T-067 (read with docs/ANIMATION-SPIKE.md)

- The adopted v1 set and architecture are **confirmed unchanged**. Nothing
  found displaces Blender-authored hero work: no CC0 source offers flight,
  roar, combat, sleep/wake, or hit/death motion matched to our wing/rig.
- One amendment of route, not scope: the spike's "Later locomotion depth"
  (trot/gallop, start/stop/turn) now has a **named retarget candidate** —
  source 1 — instead of an open question. Source 2 is the named reference
  for flap timing in T-058's Fly v2 work.
- Shortfall, recorded honestly per the task: credible CC0 **suppliers**
  number one (Quaternius, two packs). If the owner ever wants a second
  source, the bar itself (CC0-only) is the constraint to revisit — CC-BY
  would add candidates such as Sketchfab's CC-Attribution dragons, which
  this survey excluded by instruction (one verified example: "European
  Dragon" by Regina Cachoa, CC-BY, game-ready with animations).
- Adoption, when approved, needs its own task: download outside `Content/`,
  inventory bones/clips, IK-rig retarget spike on one gallop cycle, render
  gate against the shipped Walk — then decide.
