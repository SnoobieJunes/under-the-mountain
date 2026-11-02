# PLAN.md — Under the Mountain (normative plan)

Dated: 2026-10-27. Amended only per AGENTS.md (task-loop step 7 or the sweep).
Owner decision 2026-10-27: full open-world design retained (see goose-deuce.md
2026-10-27 "Engine and scope"). This plan therefore phases the full vision;
phases are delivery order, not scope cuts. A phase is only "done" when its
exit gate runs green on this Mac (M1 Max, 64 GB).

## Goal

You are a dragon. Guard a mountain hoard in an open world that reacts to how
you behave: steal-and-you-may-eat-them inside the lair, but every public kill
raises Notoriety, and Notoriety brings bigger, better-armed thieves — who also
carry better treasure. Built from scratch with agentic coding (Hermes/Muse),
Meshy-generated assets, and Unreal Engine 5.8 on macOS/Apple Silicon, as a
Meta AI Developer Hackathon 2026 (Indie track) submission due 2026-11-04
21:00 ET. The Mac-gaming claim to prove: a native Apple Silicon UE5 build
holding 60 fps in the lair + flight zone on an M1 Max.

## Users and observable success

- Primary: hackathon judges (Indie track: creativity, technical ambition,
  whether it works) playing a packaged macOS build.
- Success, observably: a fresh player finishes the intro choice (let the
  gnome go / chase him), flies out of the mountain, understands scent without
  a tutorial wall of text, loses or defends the hoard over a session, and the
  packaged build runs ≥60 fps in the lair on this machine with gamepad and
  keyboard/mouse. Submission video shows the same build, unedited capture.

## Core design pillars (from idea.md, refined)

1. **Smell everything, know nothing precisely.** Every treasure emits a golden
   scent stream (Niagara ribbon). Stream thickness = value. No distance or
   room information — the dragon must physically patrol. This is the signature
   mechanic; everything else serves it.
2. **The notoriety choice.** Kills inside the lair are invisible. Kills or
   eaten livestock outside raise Notoriety (GTA-style stars). Notoriety↑ →
   more/better raiders (more risk, more loot). Sleeping lets Notoriety decay
   across "a generation" (sleep = time-skip + partial hoard risk).
3. **Flight is the game feel.** Flap to gain altitude, dive to gain speed,
   hard landings hurt. Stamina drains with movement/flight; at zero it eats
   HP. Eating (wildlife < livestock < people, in notoriety cost) refills it.
4. **The hoard is the score and the loadout.** Treasure has value; artifacts
   (Seeing Stone, hardened scales, pendant, crown, rings…) are wearable power
   the dragon can literally lose if a thief escapes with them.

**Cross-cutting sandbox principle (owner decision 2026-10-31).** Waves,
incursions, and the PT-LOOP benchmark are pressures and tuning references,
never rails. After the intro the player may ignore the hoard, raid, roam,
experiment, or record an unscripted demo run; the world must keep simulating
and respond through theft, witnesses, Notoriety, food pressure, tribute, and
win/lose state. A feature that only works when the player follows the
benchmark route is unfinished.

## Architecture sketch (modules and seams)

UE5 C++ module `UnderTheMountain`, one system per class, subsystems injected
so automation tests run without a live level (AGENTS.md → Engineering):

| System | Owns | Seam |
|---|---|---|
| `ADragonCharacter` + `UDragonStatsComponent` | walk/flight movement, stamina, landing damage | input via Enhanced Input actions only (amended at the T-004 sweep, 2026-10-28: the T-003 skeleton shipped as one Character class, not Pawn+FlightComponent) |
| `UTreasureSubsystem` (WorldSubsystem) + `UTreasureRegistry` | treasure registry, value, carried/stolen state | scent + HUD read through it, never direct actor refs (amended at the T-004 sweep: WorldSubsystem, with the pure registry UObject as the testable seam) |
| `UScentComponent` + Niagara ribbons | scent streams to every registered treasure | thickness from treasure value; toggleable by Seeing Stone |
| `AThiefCharacter` + `UThiefBrain` (plain UObject state machine) | sneak → grab → flee → scared/trapped/dead | thief type is data (DataAsset): burglar, dwarf party, adventurer (amended at the T-004 sweep: the brain shipped as a hermetic UObject, not a StateTree — same states, no level needed to test) |
| `UNotorietySubsystem` | stars, decay (sleep), wave scaling | single writer; HUD + spawner subscribe |
| `UHoardSubsystem` | hoard total, win/lose thresholds, dragon XP/level | — |
| `UDragonStatsComponent` | HP / magic / stamina, artifacts & effects | artifacts are DataAssets applied as modifiers |

Content: `Content/Maps/` (lair, world), `Content/Meshy/` (imports),
`Content/Gameplay/` (DataAssets: treasures, artifacts, thief types, waves).

## Phases

- **M1 — Core loop in one mountain (hackathon-critical path).** Greybox→dressed
  lair (3 chambers + hoard room + peak entrance), exterior flight zone with one
  village vista + livestock, dragon (walk, flap/dive flight, fire breath, roar),
  scent system, 3 thief types, notoriety + waves, stamina/food, intro gnome
  choice, HUD, win/lose, packaged macOS build. *Exit gate: playable start-to-
  finish in PIE and in the packaged build on this Mac, gate script green.*
- **M2 — Open world expansion.** Streamed exterior: multiple settlements,
  crown-jewel heists, tribute (townsfolk + zealots), dragon XP/levels and the
  full artifact set, sleep time-skip. *Exit gate: fly lair→village→lair with
  no load screen, World Partition streaming stable at 60 fps on this Mac.*
- **M3 — Escalation endgame.** Dragonslayers at high Notoriety, army event at
  max Notoriety, rival dragons contesting the hoard (AI). *Exit gate: full
  notoriety ladder playable from 0★ to army without soft-lock.*
- **M4 — Systems depth.** Mind-control crown guards, elemental rings, magic
  healing economy, richer thief traps/counterplay. *Exit gate per system:
  automation tests + a playable scenario each.*
- **M5 — Multiplayer (post-hackathon candidate).** Other players as dragons.
  Not designed until M3 ships; networking is explicitly not retrofitted into
  M1 code beyond UE replication-ready movement. *Exit gate: TBD at M5 kickoff.*

Hackathon submission = M1 complete + as much of M2 as lands without breaking
the M1 exit gate. The submission never ships a red gate.

## Addendum 2026-10-29 — owner playtest feedback (Path A session)

Filed as T-057–T-070, plus T-071–T-074 (agent-proposed demo gaps,
owner-approved the same day); no work started. Owner direction: most
items get a one-at-a-time deep dive before implementation. These amend
M1's content, not its exit gate — the gate above stands unchanged.

- **Standalone launch is broken** (T-057): the built binary segfaults /
  cannot resolve the project descriptor / asserts on missing ICU data.
  Until fixed, the editor is the only play path; this is also T-020 risk
  arriving early. See T-056 (play.sh).
- **Flight feel is reopened** (T-058, T-062–T-065): the shipped flap reads
  as a perched dragon (wings never extend, legs never tuck), controls are
  stiff without true 3-axis movement or a hover-hold, the chimney cannot
  be climbed with them, dive's intent is unclear to the player, and the
  single camera pivots at the capsule instead of the head. Pillar 3 is
  not yet met in feel, whatever the tests say.
- **The lair's spatial design changes** (T-066): from the T-012 linear
  chamber chain + shaft to a mountain — one great open chamber, side
  caves (some dragon-passable, some thief-only), and a large central
  fly-in/out crown. Thief-only caves are a design gain: entrances the
  dragon cannot camp.
- **Dragon verbs grow** (T-067 spike → T-068, T-069, T-070): roar /
  wing-flap (blind/daze/knockback) / tail-slam animations, land/hover/hit
  coverage, smart lock-on targeting across treasure/NPCs/animals/
  artifacts, and picking up treasure — pillar 4 made literal; the
  registry's carried state gains the dragon as a carrier.
- **Small, spec-clear fixes** (T-060, T-061): one draggable colour picker
  replacing the 3 tint sliders; a scent show/hide hotkey distinct from
  true-sight.
- **Sourcing spikes are report-only** (T-059, T-067): free/open-source
  animation candidates may be surveyed with licences and retarget costs;
  nothing is adopted without owner approval.
- **Mouse-steered flight specified** (T-063, owner 2026-10-29): RMB-hold
  is free look; double-click moves/steers the dragon by that view, exact
  mechanics by best UX at prototype; any movement key cancels it.
- **Demo gaps the plan never named, now M1 scope by owner approval**
  (2026-10-29): audio (T-071 — the game is currently silent; **music is
  friend-composed and arriving before the deadline, never generated**, while
  SFX/ambience are sourced under the licence discipline), a
  game-flow shell of title/pause/win-lose/restart/quit (T-072),
  onboarding prompts + help overlay (T-073, after controls settle), and
  enemies that can damage the dragon with a defined defeat (T-074 —
  today only landings and exhaustion cost HP).

## Addendum 2026-10-31 — full-scope audit gaps filed (no cuts)

A repo-wide audit (2026-10-31) verified what is actually built against
this plan and docs/GAMEPLAY.md. The owner rejected reducing scope for
the alpha: the full scoped game stands, open world included. Items the
audit proved are neither built nor previously tasked are now filed as
T-075–T-092, and delivery order among them is set by their `Depends:`
fields, not by cutting:

- **Two spikes before their overhauls:** flight controls (T-075,
  prototypes → owner pick → T-062) joining the animation spike (T-067).
- **Specced-but-untasked systems:** claw + crush verbs (T-076, T-077),
  magic pool (T-080), the full §6 artifact set + wearables (T-081),
  bulk treasure to the §8 hoard total (T-082), XP (T-083), hoard mound
  scaling (T-086), sleep (T-084) and tribute (T-085) — these carry the
  phase tags docs/GAMEPLAY.md gave them; the tags are delivery order.
- **Proven code holes:** fire cannot damage goats (T-078), high-speed
  flight impacts are free (T-079), thief steering is hard-coded to the
  T-012 map (real navigation: T-089).
- **Never designed, now owned:** save system beyond tint (T-087),
  settings (T-088), wildlife below livestock on the food ladder
  (T-090), water and dragon-in-water behaviour (T-091), and a
  day/night world clock that sleep and cadence can read (T-092).

## Spike findings + adopted decisions 2026-10-31 — flight and animation

T-075 and T-067 were pulled forward together at owner direction. Their
research/design findings and decision records are in `docs/FLIGHT-SPIKE.md`
and `docs/ANIMATION-SPIKE.md`. The owner annotated both documents and then
delegated every remaining "idk" to agent best judgement; the adopted working
decisions below are therefore prototype law unless course/render evidence
falsifies them. No scope was cut.

- **Flight:** the current yaw-plane `MOVE_Flying`, velocity-overwrite flap,
  and camera-forward dive cannot meet pillar 3. Adopted prototype: a custom
  movement mode blending Dragonflight-style pitch/momentum flight at speed
  with automatic precision thrust/hover at low airspeed (Brake forces
  precision). Dive is nose-down posture; Shift/LT is Brake; F/top-face is
  contextual Landing Assist. Stamina stays one continuous 100 pool shown as
  five 20-point reserves; Ascent/Surge each spend one reserve. WoW-style
  high-speed/skim stamina regeneration is **rejected** — speed must not create
  calories in a food-survival loop; skimming only receives physical ground
  effect (less drag/sink). GAMEPLAY §2 now carries the target bands, starting
  speeds (2400 cruise / 3200 Surge / 4000 dive terminal), zero-stamina mercy
  rule, and a Settings testing toggle for Infinite Stamina. No scheme has been
  flown yet: T-075's PIE course (chimney, slalom, village circuit, dive-to-eat,
  cave precision, zero-stamina) still gates production adoption in T-062.
- **Animation:** single-node, two-clip playback is the ceiling. Adopted
  architecture is a lean `ABP_Dragon` on a C++ `UDragonAnimInstance` parent:
  locomotion states/blends, montage slots, fold/tuck/bank/aim additive layers,
  then selective Control Rig only where captures prove the need. The adopted
  v1 set is ground idle, preserved Walk, Fly v2 via T-058, held Glide,
  Hover/precision, Takeoff, Land, Roar, fire pose + aim, Hit light/heavy,
  Defeat, and a reusable Sleep/Wake set; combat montages ship as full verbs,
  not stubs. Loops are Blender-programmatic unless the CC0-only T-059 survey
  finds a better locomotion retarget; hero one-shots are hand-posed behind
  batched contact-sheet + in-engine capture gates. The proven hero pipeline
  stands — Muse multi-images → Meshy mesh/textures → Blender rig/animation →
  Unreal; only Meshy's biped rig/animation route is excluded for the dragon.
  Death presentation:
  dragon authored collapse/slink, thieves authored-fall → ragdoll settle,
  goats authored fall.
- **Rig:** the 33-bone rig is sufficient for the immediate spread/tuck fix.
  Fly's legs are currently unkeyed. A bind/current/test-spread render decides
  whether T-058 is clip-only or also needs the now-approved focused wing
  weight correction; re-binding is the last resort, not the first move.

## Explicit non-goals (hackathon build)

Windows/Linux builds; multiplayer (M5); rival-dragon AI (M3); "train AI using
JEV decision model" from idea.md — thief AI is StateTree state machines for
the hackathon, learned models are an M4+ research item; custom Metal 4
renderer work — UE5.8 on Mac already renders via Metal, the showcase is a
native Apple Silicon build at 60 fps, not engine-renderer modification;
Kaigen3D (evaluated 2026-10-27, rejected for the submission: closed beta,
licence-gated, no local install, unproven Meshy/animation pipeline — see
goose-deuce.md 2026-10-27 "Engine and scope").

## Art direction (owner decision 2026-10-27)

**Style B — grounded realism.** Realistic world (terrain, village, lair) lit
cinematically; the dragon's realism is carried by texture/normal maps over a
clean silhouette, not by raw mesh detail (Meshy "photoreal" geometry melts at
fine detail). Mockups (dragon / countryside / flight / lair, styles A+B) were
generated during selection; the *official* concepts are regenerated with Muse
Image through the muse-harness plugin and approved by the owner before any 3D
spend (T-025). SAM 3.1's role is segmentation: cutting the approved dragon out
of its concept background for a clean Meshy image-to-3D input. If dragon
iterations stall below bar, fallback is an A-style colour grade on the same
assets — a lighting change, not a rebuild. Perf contract unchanged: 60 fps on
this M1 Max, measured per level, dynamic resolution + Lumen software RT +
2K texture cap (docs/ASSET-PIPELINE.md budgets).

## Risks with a gate, not a hope

- **Meshy dragon rig — decided by test (2026-10-28):** Meshy supplied the
  hero mesh/textures; its auto-rig was a humanoid biped with no wing/tail
  bones and was rejected. The shipped path is Blender quadruped rig/skin/
  animation (docs/ASSET-PIPELINE.md dragon gate). Repeat that pipeline; do
  not re-spend on Meshy dragon rig/animation unless a future verified
  quadruped route passes a fresh gate.
- **Meshy budget (finite owner budget):** proven spend reconciles 1150 → 500
  credits (last proven balance 2026-10-28). Remaining forecast to finish the
  scoped hackathon game: ~450 credits efficient, ~625 quality-safe, plus a
  100-credit untouchable reserve — hold ~550 available for the efficient path
  or ~725 for quality-safe (docs/ASSET-PIPELINE.md owns the line items, batch
  rules, and upgrade guidance). Concept images stay on Muse (0 Meshy credits);
  no generation batch starts without a balance check and an approved concept
  sheet.
- **Open-world performance on M1 Max:** M1 flight zone is the perf proxy;
  Nanite/Lumen settings tuned there before M2 streaming is attempted.
- **Timeline:** 8 days to submission. M1 tasks are sized ≤1 session each on
  TASKS.md; anything slipping moves within M1, M2 absorbs the cut first.
