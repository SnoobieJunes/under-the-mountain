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

## Architecture sketch (modules and seams)

UE5 C++ module `UnderTheMountain`, one system per class, subsystems injected
so automation tests run without a live level (AGENTS.md → Engineering):

| System | Owns | Seam |
|---|---|---|
| `ADragonPawn` + `UDragonFlightComponent` | walk/flight movement, stamina, landing damage | input via Enhanced Input actions only |
| `UTreasureSubsystem` (GameInstanceSubsystem) | treasure registry, value, carried/stolen state | scent + HUD read through it, never direct actor refs |
| `UScentComponent` + Niagara ribbons | scent streams to every registered treasure | thickness from treasure value; toggleable by Seeing Stone |
| `AThiefCharacter` + `UThiefBrain` (StateTree) | sneak → grab → flee → scared/trapped/dead | thief type is data (DataAsset): burglar, dwarf party, adventurer |
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

- **Meshy dragon rig** (wings/quadruped auto-rig is unreliable): Day-1 gate
  in M1 — Meshy dragon → remesh → rig test in UE; fail → Blender fixup path
  (Blender MCP) with code-driven wing flap. Decided by test, not optimism.
  Budget for Style B texture-carried realism: 3–5 dragon iterations
  (~150–250 Meshy credits of the 1150 balance) before the grade fallback.
- **Open-world performance on M1 Max:** M1 flight zone is the perf proxy;
  Nanite/Lumen settings tuned there before M2 streaming is attempted.
- **Timeline:** 8 days to submission. M1 tasks are sized ≤1 session each on
  TASKS.md; anything slipping moves within M1, M2 absorbs the cut first.
