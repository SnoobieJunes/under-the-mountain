# TASKS.md — Under the Mountain work queue
<!-- One queue. New DONE entries at the top of Done. Tasks without observable
     acceptance criteria are not pickable — sharpen them first. -->

## Now
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
- [ ] T-004 — goose-deuce sweep
      Why: standing audit cadence (AGENTS.md → Deviations).
      Accept: NEW-PROJECT-GUIDE step-5 checklist run; [process] marker entry filed; successor sweep task filed.
      Touches: goose-deuce.md, docs/PLAN.md, TASKS.md   Depends: none   Gated: cadence

## Blocked
- [!] T-029 — Dragon 500k + eye fix + quadruped walk cycle — BLOCKED 2026-10-28: model side DONE via Meshy rerun (owner redirected off Blender): dragon_v4 (Meshy task 01a181fc, single-image from SAM cutout, 35 cr) is rock-free with single regular eyes, 310,196 tris textured + 980,286-tri pre-remesh master in Content/Meshy/Raw/Dragon/, imported as SM_Dragon_v4. Meshy textured cap is ~310k (500k textured does not exist in Meshy; pre-remesh is untextured). Rig/walk remains: Meshy rig is humanoid-only (will melt again — proven T-010), Blender rig attempt (T-028 + uncommitted dragon_500k.* in Content/Meshy/Dragon/) was rejected by owner as slop. Decision needed: (a) invest in a proper Blender/manual quadruped rig + walk, (b) accept Meshy humanoid rig knowing it deforms, (c) defer rig/walk to T-011 flight work and ship static dragon for now. Recommendation: (a) as its own focused task, or (c) if schedule pressure wins.

## Dropped

## Done
- [x] T-028 — Blender dragon rig fixup: quadruped + wing + tail bones (2026-10-28, this commit) — ultra mesh rock base cut at feet plane, decimated 309k→79k, 27-bone rig (pelvis/spine/chest/neck/head, wing_shoulder/elbow/wrist/finger L+R, 4 leg chains, tail.01-05) envelope-skinned in Blender; FBX (dragon_rigged_v2.fbx + .blend) replaced SK_Dragon_Rigged in UE (loads verified). NOTE: bind pose is the source crouch/perch (reposing the fragmented AI mesh risked tearing it); standing/flight poses come from AnimBP posing in T-011. Evidence: Content/Meshy/Dragon/armature-diagram.png, dragon_v2_side.png
- [x] T-010 — Dragon asset gate: Meshy dragon → UE, rig verdict (2026-10-28, this commit) — Meshy multi-image-to-3D (30 cr) + rig (5 cr) via pinned meshy MCP server over stdio (plugin tools absent this session); 82,933-tri textured dragon imported as SK_Dragon_Rigged; VERDICT: humanoid 24-joint rig unusable (no wing/tail bones) → Blender fixup filed as T-028, verdict in goose-deuce 2026-10-28
- [x] T-022 — Instrument C++ line coverage (llvm-cov) and ratchet the floor (2026-10-28, this commit) — Scripts/coverage.sh prints real llvm-cov (module-only rsp-relink, LLVM continuous mode; measured 31.1% product lines, Tests/ excluded); AGENTS.md floor 0%→29%; gate.sh enforces fresh artifact ≥ floor
- [x] T-026 — Gameplay detail spec (docs/GAMEPLAY.md) (2026-10-27, this commit) — docs/GAMEPLAY.md 596 lines: loop timings, stamina/food, scent brackets, wave+notoriety tables, 3 thief types w/ tells+counters, all 11 idea.md artifacts, intro beats, win/lose, M2 sleep/tribute; every number [tuning:test] or [derived:reason]
- [x] T-025 — Concept art pack (Style B) via Muse Image + SAM 3.1 (2026-10-27, this commit) — OWNER APPROVED 2026-10-27 (gold/crimson dragon, other renders approved as-is); muse-harness tools verified live: muse_turn only (model muse-wonder-full-1.0, live call returned MUSE_LIVE); Muse Image called directly via Meta API (muse-image-1.0 + image_generation tool — hermes chat refuses function tools, proven 400); 10 concepts in docs/concepts/, docs/ART-DIRECTION.md written with SAM 3.1 segmentation step (runs under T-010)
- [x] T-003 — Walking skeleton: dragon in an empty lair, playable in PIE (2026-10-27, this commit) — PIE via MCP: pawn spawned at PlayerStart (0,0,150), IsPIERunning true; gate green incl. Dragon.Stats tests; keyboard/gamepad bindings in code (Enhanced Input), interactive feel unverified until human play (T-011 owns feel)
- [x] T-023 — Prove unreal-mcp live against the editor (2026-10-27, this commit) — hermes mcp test ✓ Connected (3 meta-tools); list_toolsets returned EditorApp/Niagara/PCG/AgentSkill/Logs; proof path substituted, see goose-deuce.md 2026-10-27 "T-023 proof path substituted"
- [x] T-024 — Cold-clone gate fails on UBA cache: make builds deterministic (2026-10-27, this commit)
- [x] T-002 — Toolchain and the test gate (2026-10-27, this commit)
- [x] T-001 — Plan, backlog, and living docs (2026-10-27, this commit)
- [x] T-000 — Scaffold Under the Mountain from templates (2026-10-27, this commit)
