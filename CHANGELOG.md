<!-- Contract (AGENTS.md -> Documentation protocol): one entry per task with a
     user-visible effect, format `YYYY-MM-DD — T-### — <what changed, user-visible
     effect>`; internal-only tasks get one line ending `(no user-visible change)`. -->
# CHANGELOG — Under the Mountain

2026-10-28 — T-012 — Lair level built: 3 chambers + hoard room + peak shaft, dressed with gold piles, crates, and torch lights (Content/Maps/Lair/Lair.umap); scripted traversal 14/14 and 60 fps at vsync cap measured on this Mac

2026-10-28 — T-011 — Flight feel systems: flap/glide/hover/dive stamina economy, hard-landing HP damage, velocity debug HUD (PIE feel check filed as T-030) (no user-visible change)

2026-10-28 — T-029 — Dragon rigged and walking: 33-bone quadruped rig on Meshy V4, diagonal-gait walk cycle in UE (SK_Dragon_v4 + AnimSequence); Scripts/run-loop.sh added (no user-visible change)

2026-10-28 — T-029 — Meshy V4 dragon: rock-free, single regular eyes, 310k tris textured (980k pre-remesh master) imported as SM_Dragon_v4; rig/walk blocked on owner decision (no user-visible change)

2026-10-28 — T-028 — Dragon re-rigged in Blender: rock base removed, quadruped rig with wing + tail bones (79k tris) replaces the broken humanoid rig in UE (no user-visible change)

2026-10-28 — T-028 — Ultra-detail dragon source generated: dragon_ultra.glb/fbx (309,716 tris, 4K textures) in Content/Meshy/Raw/Dragon for the Blender re-rig (no user-visible change)

2026-10-28 — T-010 — Dragon 3D asset in engine: Meshy gold/crimson dragon (82,933 tris) imported to Content/Meshy/Dragon; auto-rig verdict humanoid/unusable — Blender re-rig filed as T-028 (no user-visible change)

2026-10-28 — T-022 — Real C++ line coverage: coverage.sh measures 31.1% (llvm-cov), floor ratcheted to 29% and enforced by the gate (no user-visible change)

2026-10-27 — T-026 — Gameplay spec published: docs/GAMEPLAY.md with implementable numbers/tables for loop, stamina, scent, waves, thieves, artifacts, intro, win/lose (no user-visible change)

2026-10-27 — T-025 — Concept art pack approved: gold/crimson dragon (multi-view), lair, village, 3 thieves, hoard in docs/concepts/ + docs/ART-DIRECTION.md; player scale-colour picker filed as T-027 (no user-visible change)

2026-10-27 — T-025/T-026 filed — Art direction locked to Style B (realism); concept pack + gameplay spec queued (no user-visible change)

2026-10-27 — T-003 — Playable walking skeleton: dragon pawn (walk/fly/flap/dive, stamina) spawns in the greybox lair in PIE on Mac

2026-10-27 — T-023 — unreal-mcp proven live against the editor (Hermes connects; toolsets listed) (no user-visible change)

2026-10-27 — T-024 — Gate builds made deterministic cold (-nocache); UBA + editor-token foot-guns recorded (no user-visible change)

2026-10-27 — T-002 — UE 5.8.4 project + gate scripts live; editor/game builds and automation smoke test green on this Mac (no user-visible change)
2026-10-27 — T-001 — Design plan, refined idea, and M1 backlog published (docs/PLAN.md, idea.md) (no user-visible change)
2026-10-27 — T-000 — Project scaffolded from templates; process law (AGENTS.md) and task board live (no user-visible change)
