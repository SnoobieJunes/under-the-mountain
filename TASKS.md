# TASKS.md — Under the Mountain work queue
<!-- One queue. New DONE entries at the top of Done. Tasks without observable
     acceptance criteria are not pickable — sharpen them first. -->

## Now
- [ ] T-026 — Gameplay detail spec (docs/GAMEPLAY.md)
      Why: PLAN.md pillars are not yet implementable numbers; thief/scent/notoriety tasks (T-013+) need tables, not prose.
      Accept: docs/GAMEPLAY.md covers loop timings, stamina/food economy numbers, scent readability rules, wave table, per-thief behaviour + tells + counters, artifact specs (all idea.md artifacts), intro beat script, win/lose thresholds, sleep/tribute rules for M2; every number either marked [tuning] with its test, or derived from a stated reason.
      Touches: docs/GAMEPLAY.md, docs/PLAN.md   Depends: T-003   Gated: none
- [ ] T-010 — Dragon asset gate: Meshy dragon → UE, rig verdict
      Why: winged-dragon auto-rig is the project's biggest asset risk; verdict must land before flight animation is designed around it.
      Accept: Meshy dragon generated via docs/ASSET-PIPELINE.md, imported to Content/Meshy/Dragon; written verdict in goose-deuce.md ([decision]): rig usable, or Blender fixup task filed with the failing evidence (screenshot + bone list).
      Touches: Content/Meshy/Dragon/, docs/ASSET-PIPELINE.md, goose-deuce.md   Depends: T-025   Gated: none
- [ ] T-011 — Dragon flight feel: flap/dive/stamina/landing damage
      Why: flight is design pillar 3; feel gates every exterior decision.
      Accept: In PIE, flap gains altitude, dive gains speed (velocity readout in debug HUD), landing above threshold speed deals damage; automation tests for stamina drain + landing damage pass via Scripts/gate.sh.
      Touches: Source/UnderTheMountain/Dragon/, Content/Maps/   Depends: T-010   Gated: none
- [ ] T-012 — Lair level: 3 chambers + hoard room + peak exit, dressed
      Why: the patrol space the scent mechanic is played in.
      Accept: Level loads in PIE, dragon traverses all chambers on foot and exits via the peak in flight; no collision soft-locks found in a scripted traversal; 60 fps in lair on this Mac (stat fps capture in task evidence).
      Touches: Content/Maps/Lair/, Content/Meshy/   Depends: T-011   Gated: none
- [ ] T-013 — Treasure + scent system
      Why: design pillar 1, the signature mechanic.
      Accept: Placing a treasure DataAsset in the lair produces a scent ribbon whose thickness scales with value; Seeing Stone toggle switches to true-sight markers; automation tests for registry/value scaling pass.
      Touches: Source/UnderTheMountain/Treasure/, Source/UnderTheMountain/Scent/, Content/Gameplay/Treasures/   Depends: T-012   Gated: none
- [ ] T-014 — Thief AI: burglar, dwarf party, adventurer
      Why: the antagonist of the core loop; three behaviours from one data-driven brain.
      Accept: Each type, spawned in PIE, sneaks to a treasure, grabs it, and flees to the exit; adventurer drops loot when roared at; dwarf trap slows the dragon once; outcomes logged by an automation test scenario.
      Touches: Source/UnderTheMountain/Thief/, Content/Gameplay/Thieves/, Content/Meshy/Thieves/   Depends: T-013   Gated: none
- [ ] T-015 — Combat: fire breath, roar, eat
      Why: the verbs the notoriety choice is made with.
      Accept: Fire breath kills a thief in PIE (cone + damage over time), roar scares within radius, eating a corpse/livestock restores stamina; each covered by an automation test.
      Touches: Source/UnderTheMountain/Dragon/, Source/UnderTheMountain/Stats/   Depends: T-014   Gated: none
- [ ] T-016 — Notoriety + waves + win/lose
      Why: design pillar 2; turns verbs into a game with stakes.
      Accept: Public kill raises stars in HUD, lair kill does not; stars scale wave size/gear; hoard below threshold loses, surviving final wave wins; state machine covered by automation tests.
      Touches: Source/UnderTheMountain/Notoriety/, Source/UnderTheMountain/Hoard/, Content/Gameplay/Waves/   Depends: T-015   Gated: none
- [ ] T-017 — Exterior flight zone: village vista + livestock
      Why: gives the chase/eat choice somewhere to happen; M2's perf proxy.
      Accept: Dragon flies lair→village→lair in PIE with no load screen; goats present and edible (Notoriety cost applied); ≥60 fps sustained over the village on this Mac (capture in evidence).
      Touches: Content/Maps/World/, Source/UnderTheMountain/Notoriety/   Depends: T-016   Gated: none
- [ ] T-018 — Intro: hoard wake-up + gnome choice
      Why: the tutorial is the dilemma; first 60 seconds judges see.
      Accept: New game plays the wake-up, gnome steals, choice UI (let go / chase) applies the correct Notoriety consequence for each branch; both branches reach free play without soft-lock.
      Touches: Content/Maps/Lair/, Source/UnderTheMountain/Intro/, Content/Gameplay/   Depends: T-017   Gated: none
- [ ] T-019 — HUD + menus + gamepad parity
      Why: HP/magic/stamina, stars, hoard total must read at a glance on both inputs.
      Accept: HUD shows all four values live in PIE; every action in the game is performable on gamepad alone and keyboard/mouse alone (checklist run and recorded in task evidence).
      Touches: Source/UnderTheMountain/UI/, Content/UI/, Config/   Depends: T-018   Gated: none
- [ ] T-020 — Packaged macOS build + perf gate
      Why: judges run a package, not PIE; the Mac-gaming claim must be measured.
      Accept: Scripts/build.sh produces a runnable .app; fresh launch reaches gameplay; 60 fps lair + village on this Mac; install/run steps in README verified as written.
      Touches: Scripts/, README.md, Config/   Depends: T-019   Gated: none
- [ ] T-021 — Submission package: video capture + project page draft
      Why: hackathon deliverable beyond the build.
      Accept: Unedited gameplay capture (intro→one full wave) recorded from the T-020 build; submission text (pillars, stack, agentic workflow) drafted in docs/SUBMISSION.md.
      Touches: docs/SUBMISSION.md   Depends: T-020   Gated: owner

## Later
- [ ] T-027 — Dragon scale-colour picker (RGB slider)
      Why: owner request 2026-10-27 — players set their dragon's scale colour; canonical art stays gold/crimson (docs/ART-DIRECTION.md), the picker tints from that base.
      Accept: In PIE, an RGB slider picker changes the dragon's scale tint live (material parameter, crimson membranes stay crimson unless separately tinted), choice persists across a PIE restart; automation test for the tint parameter passing via Scripts/gate.sh.
      Touches: Source/UnderTheMountain/Dragon/, Source/UnderTheMountain/UI/, Content/Meshy/Dragon/   Depends: T-010   Gated: none
- [ ] T-022 — Instrument C++ line coverage (llvm-cov) and ratchet the floor
      Why: T-002 coverage floor is an honest 0% — UE C++ line coverage is not instrumented; a measured floor is owed before M1 code grows.
      Accept: Scripts/coverage.sh prints a real line-coverage percentage for Source/UnderTheMountain; AGENTS.md floor set to just under the measured number; gate fails below it.
      Touches: Scripts/coverage.sh, Scripts/gate.sh, AGENTS.md   Depends: T-003   Gated: none
- [ ] T-004 — goose-deuce sweep
      Why: standing audit cadence (AGENTS.md → Deviations).
      Accept: NEW-PROJECT-GUIDE step-5 checklist run; [process] marker entry filed; successor sweep task filed.
      Touches: goose-deuce.md, docs/PLAN.md, TASKS.md   Depends: none   Gated: cadence

## Blocked

## Dropped

## Done
- [x] T-025 — Concept art pack (Style B) via Muse Image + SAM 3.1 (2026-10-27, this commit) — OWNER APPROVED 2026-10-27 (gold/crimson dragon, other renders approved as-is); muse-harness tools verified live: muse_turn only (model muse-wonder-full-1.0, live call returned MUSE_LIVE); Muse Image called directly via Meta API (muse-image-1.0 + image_generation tool — hermes chat refuses function tools, proven 400); 10 concepts in docs/concepts/, docs/ART-DIRECTION.md written with SAM 3.1 segmentation step (runs under T-010)
- [x] T-003 — Walking skeleton: dragon in an empty lair, playable in PIE (2026-10-27, this commit) — PIE via MCP: pawn spawned at PlayerStart (0,0,150), IsPIERunning true; gate green incl. Dragon.Stats tests; keyboard/gamepad bindings in code (Enhanced Input), interactive feel unverified until human play (T-011 owns feel)
- [x] T-023 — Prove unreal-mcp live against the editor (2026-10-27, this commit) — hermes mcp test ✓ Connected (3 meta-tools); list_toolsets returned EditorApp/Niagara/PCG/AgentSkill/Logs; proof path substituted, see goose-deuce.md 2026-10-27 "T-023 proof path substituted"
- [x] T-024 — Cold-clone gate fails on UBA cache: make builds deterministic (2026-10-27, this commit)
- [x] T-002 — Toolchain and the test gate (2026-10-27, this commit)
- [x] T-001 — Plan, backlog, and living docs (2026-10-27, this commit)
- [x] T-000 — Scaffold Under the Mountain from templates (2026-10-27, this commit)
