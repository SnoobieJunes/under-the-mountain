# TASKS.md — Under the Mountain work queue
<!-- One queue. New DONE entries at the top of Done. Tasks without observable
     acceptance criteria are not pickable — sharpen them first. -->

## Now
- [ ] T-002 — Toolchain and the test gate
      Why: one pinned UE/Xcode toolchain + a runnable gate before product code.
      Accept: Scripts/setup.sh, run-editor.sh, test-fast.sh, gate.sh, coverage.sh, build.sh all run as written in AGENTS.md → Project snapshot; no `TBD step 3` markers remain; cold-clone gate green.
      Touches: Scripts/, UnderTheMountain.uproject, Source/, Config/, AGENTS.md   Depends: T-001   Gated: none
- [ ] T-003 — Walking skeleton: dragon in an empty lair, playable in PIE
      Why: smallest end-to-end slice proving the architecture (input → dragon pawn → camera → flight) on real hardware.
      Accept: PIE on this Mac: dragon pawn spawns in a greybox lair, walks, flaps/dives with stamina drain, gamepad + keyboard; automation smoke test passes via Scripts/gate.sh.
      Touches: Source/UnderTheMountain/, Content/Maps/, Config/   Depends: T-002   Gated: none
- [ ] T-010 — Dragon asset gate: Meshy dragon → UE, rig verdict
      Why: winged-dragon auto-rig is the project's biggest asset risk; verdict must land before flight animation is designed around it.
      Accept: Meshy dragon generated via docs/ASSET-PIPELINE.md, imported to Content/Meshy/Dragon; written verdict in goose-deuce.md ([decision]): rig usable, or Blender fixup task filed with the failing evidence (screenshot + bone list).
      Touches: Content/Meshy/Dragon/, docs/ASSET-PIPELINE.md, goose-deuce.md   Depends: T-003   Gated: none
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
- [ ] T-004 — goose-deuce sweep
      Why: standing audit cadence (AGENTS.md → Deviations).
      Accept: NEW-PROJECT-GUIDE step-5 checklist run; [process] marker entry filed; successor sweep task filed.
      Touches: goose-deuce.md, docs/PLAN.md, TASKS.md   Depends: none   Gated: cadence

## Blocked

## Dropped

## Done
- [x] T-001 — Plan, backlog, and living docs (2026-10-27, this commit)
- [x] T-000 — Scaffold Under the Mountain from templates (2026-10-27, this commit)
