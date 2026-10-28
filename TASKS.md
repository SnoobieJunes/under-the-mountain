# TASKS.md — Under the Mountain work queue
<!-- One queue. New DONE entries at the top of Done. Tasks without observable
     acceptance criteria are not pickable — sharpen them first. -->

## Now
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
- [!] T-014 — Thief AI: burglar, dwarf party, adventurer — PARKED 2026-10-28 in git stash "T-014" (apply with `git stash apply stash^{/T-014}`; do not pop until claimed). Done in the stash: Thief brain/character/trap C++ (muse-drafted, parent-reviewed), dragon ApplyTrapSlow hook, Scripts/create_thieves.py, 3 thief DataAssets placed in Lair.umap, tests Thief.States + Thief.Traps passing (8/8 suite at park time). Left: (1) add `float DebugLogTimer = 0.0f;` to ThiefCharacter.h — the debug-log code in the stash references it and will not compile without it; (2) rebuild and re-run the -game scenario to prove the new chamber-waypoint steering produces grab→flee→exit logs for all 3 thieves (straight-line steering wedged on walls; waypoint fix is in the stash, unverified in-game); (3) prove dwarf trap slows the dragon once in-game and roar-drop in a live session; (4) coverage + full gate + close-out docs/commit. Owner preempted with the dragon-mesh task (T-032) on 2026-10-28.
- [!] T-030 — Dive input never clears on key release + PIE flight-feel check — BLOCKED 2026-10-28: code half DONE in the park commit (Dive Completed/Canceled now clear bDiving; gate green, coverage 31.0%). Remaining: PIE session (human or MCP — needs GUI editor with live MCP, currently a stale instance) recording flap→altitude, dive→speed with HUD, hard-landing HP damage; plus (added 2026-10-28, T-012) an input-driven chamber→hoard→peak traversal of Lair.umap in PIE — scripted geometric traversal is proven, the in-PIE walk is not.

## Dropped

## Done
- [x] T-027 — Dragon scale-colour picker (RGB slider) (2026-10-28, this commit) — UDragonAppearanceComponent owns the tint (clamped, MID ScaleTint write, SaveGame slot 'DragonScaleTint' persistence); code-built UDragonTintWidget (3 sliders) toggled with C / gamepad right face. M_DragonScales authored from the V4 maps (Scripts/create_dragon_material.py): final = base x ScaleTint except crimson-dominant pixels (membranes/spines protected in-graph); assigned to SK_Dragon_v4 slot 0, which had been WorldGridMaterial — the mesh shipped untextured. Proven: Dragon.Tint test passes (clamp + save/load round-trip + MID parameter readback; the strengthened test failed before its mesh-slot fix); PIE screenshot Content/Meshy/Dragon/dragon_textured_pie.png shows textured gold/tan dragon with crimson ridge at canonical white. Slider drag itself not live-exercised (same MCP-input limit as T-030; binding follows the proven input pattern).
- [x] T-031 — Niagara scent ribbons (replace cube-geometry ribbons) (2026-10-28, this commit) — NS_ScentRibbon authored via Niagara toolsets (LocationBasedRibbon emitter + SpawnRate 240/s, infinite, lifetime 0.6s, particle colour tarnished gold, RibbonWidth DirectSet linked to User.RibbonWidth, ScaleColor alpha linked to User.RibbonOpacity); AScentRibbonActor is now a Niagara component ping-pong sweeping nose<->treasure at 6000cm/s (location-based particles make the sweep the ribbon), cube mesh path deleted. Proven: 7/7 tests incl. new Scent.RibbonWidthPassThrough (actor carries exact FScentMath widths); -game log compiles NS in 0.097s with 4 ribbons active, no Niagara errors; PIE screenshot Content/Gameplay/Treasures/niagara_ribbon_pie.png (thin tan line at lair scale — readability at gameplay distance is PT-SCENT tuning, still owed by playtest, not this task).
- [x] T-032 — Real dragon mesh on the pawn (replace placeholder ball) (2026-10-28, this commit) — ADragonCharacter loads SK_Dragon_v4 onto GetMesh() (visible, NoCollision; capsule still owns movement), probed bounds 190x48x72cm scaled 2x at z-24 so feet sit on the capsule floor, yaw -90 verified correct in PIE (tail to camera, head down the corridor: Content/Meshy/Dragon/dragon_pawn_pie.png); placeholder sphere hidden; walk cycle plays single-node while moving, bind pose at rest. Flight posing/AnimBP remains with the flight-feel work (mesh holds bind/walk pose in flight — known limit, not new). Gate green; coverage 34.2%.
- [x] T-013 — Treasure + scent system (2026-10-28, this commit) — UTreasureDataAsset/Registry/Subsystem + ATreasureActor; UScentComponent spawns value-scaled scent ribbons (GAMEPLAY §3 brackets 4/8/14/22/32cm + micro-variation, carried pulse, no-distance encoding) and Seeing Stone true-sight (30% dim, teal through-rock markers, refused while stone carried); 4 treasures authored + placed in Lair. Proven: 3 new automation tests pass (Registry/Width/NoDistanceAndTrueSight, 6/6 total), standalone -game log shows 4 registrations + "Scent ribbons active: 4", coverage 33.5%. Deviations: cube-geometry ribbon not Niagara (T-031 filed); T-key toggle bound but not live-pressed (T-030 owns input checks) — goose-deuce 2026-10-28.
- [x] T-012 — Lair level: 3 chambers + hoard room + peak exit, dressed (2026-10-28, this commit) — Lair.umap built headless (203 actors: 190 static meshes, gold/rock/wood materials, 10 point lights + shaft sun/sky); verify_lair.py geometric traversal 14/14 with live controls (physics traces return None headless — probed, see goose-deuce 2026-10-28); PIE loads with dragon pawn spawning at DragonStart (editor log); standalone -game CSV capture last 120 frames avg 16.81 ms ≈ 60 fps at vsync cap on this Mac (Saved/Profiling/lair_fps.csv). Input-driven in-PIE chamber walk remains on T-030.
- [x] T-011 — Dragon flight feel: flap/dive/stamina/landing damage (2026-10-28, this commit) — HP pool + landing-damage curve (0 ≤800 cm/s, 5–25 to 2200), flap cost 3, glide 1/s / hover 8/s / dive 0, exhaustion HP burn 2/s, Landed() damage, velocity debug HUD; FDragonFlightTest + existing tests pass, coverage 31.6%. PIE feel NOT verified (running editor is an 11h instance with no MCP listener; human/MCP PIE check + dive-release fix filed as T-030)
- [x] T-029 — Dragon rig + quadruped walk cycle (Meshy V4 source) (2026-10-28, this commit) — V4 rigged in Blender: 33 bones (quadruped spine, wing chains, tail.01-06, 4×3 leg chains), voxel-proxy bone-heat weights transferred (posed test renders clean: legs/neck/tail; wing membrane crumples only when lifted — flight posing is T-011); 24f diagonal-gait Walk in UE as SK_Dragon_v4 + SK_Dragon_v4_Anim (frames verified mid-stride both phases). Also added Scripts/run-loop.sh (fires board loop only once T-029 is [x]). Known limit: skinning verified at gait/test amplitudes; extreme poses untested.
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
