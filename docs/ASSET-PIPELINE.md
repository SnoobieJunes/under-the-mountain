# ASSET-PIPELINE.md — Meshy → Unreal (and Blender fallback)

Historical/procedural — PLAN.md is normative for what ships; this file is the
how for assets. Written 2026-10-27 (T-001); exercised first in the M1 asset tasks.

## The pipeline (every game asset)

1. **Design image first.** Muse (via Hermes `meshy_text_to_image`) generates a
   concept from the task's prompt. Owner approves the image before any 3D —
   a rejected image costs ~5 credits, a rejected model ~30.
2. **3D generation.** `meshy_image_to_3d` from the approved image (textured),
   or `meshy_multi_image_to_3d` when multi-view exists. Direct
   `meshy_text_to_3d` is the fallback for props where silhouette is enough.
3. **Remesh to budget.** `meshy_remesh`: characters ≤ 30k tris, props ≤ 5k,
   treasure items ≤ 2k. Hero dragon is the exception (≤ 80k, tuned in M1).
4. **Rig + animation (characters only).** `meshy_rig` then animation presets
   (`meshy_animate`, 600+ clips) for humanoids — thieves are humanoid, this
   path is proven. Quadruped/winged rigging is NOT assumed: see dragon gate.
5. **Download — always save locally, same session (owner rule, 2026-10-28).**
   Every Meshy output is saved to this machine before the session ends —
   never leave an asset only behind a signed Meshy link (links expire ~3 days
   after the task finishes; saved files are permanent). `meshy_download_model`
   → `Content/Meshy/Raw/<asset>/` (FBX for rigged characters, GLB for static
   props), plus everything else the task produced that a link could lose:
   the preview/thumbnail rendering (`<asset>_preview.png`), base-color
   textures, and for rig tasks the free walk/run animation files (withSkin
   GLB + FBX and armature GLB, fetched from the rigging task's URLs — rig
   tasks are not saved by `meshy_download_model` itself). Verify every saved
   file exists with non-zero size, and name the saved paths in the report.
   Raw files are untouchable once committed (AGENTS.md → LOOP untouchables)
   — re-download instead of editing.
6. **Import to Unreal** via unreal-mcp / editor import: FBX → Skeletal Mesh
   (characters) or Static Mesh (props), GLB via Interchange. Imported assets
   live beside Raw as `.uasset`; materials rebuilt as project materials,
   never shipped as raw Meshy material instances.
7. **Verify in PIE.** Screenshot + a 10-second PIE run; poly/texture check
   against the budget above. The importing task's Accept names this step.

## The dragon gate (Day 1 of M1, task T-010)

Meshy auto-rig handles humanoid/biped/quadruped body plans; a winged dragon
is none of those cleanly. So: generate dragon → rig attempt → import → wing
bones usable in UE? **Pass:** animate via Animation Blueprint + code-driven
flap (banking/dive pose from flight velocity). **Fail:** export the mesh to
Blender (Blender MCP), hand-place wing/shoulder bones, skin, return FBX to
UE; flight animation stays code-driven either way (procedural flap amplitude
from flap input, glide pose when diving). No third option, no silent hope.

## How Muse works the editor

Hermes hosts `unreal-mcp` (HTTP, `127.0.0.1:8000`, enable in `hermes mcp list`).
Flow per unreal-mcp skill: `list_toolsets` → `describe_toolset` → `call_tool`.
Used for: level/actor placement, Blueprint/Niagara/Material creation,
Live Coding recompiles after C++ edits, PIE control. Rules: save before and
after bulk edits, never mutate during PIE, treat any non-success return as a
stop. C++ itself is written as files in `Source/` (reviewable diffs); MCP is
for editor state, not a code-delivery channel.

## Credits discipline

Check `meshy_check_balance` before a generation batch; image-approve-then-3D
keeps spend at ~35 credits/hero asset. Log batches >100 credits in
goose-deuce.md as `[decision]` with the balance before/after.
