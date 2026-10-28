# Thief assets — Meshy generation report

Date: 2026-10-28

## Context

Meshy thief assets generated 2026-10-28 from owner-approved concepts in `docs/concepts/`, per `docs/GAMEPLAY.md` §5.

Settings for all models: `meshy_image_to_3d`, ai_model `meshy-t2.5`, model_type `smart-topology`, `should_texture=true`, pose_mode `t-pose`, target_formats `[fbx,glb]`; then `meshy_rig`.

Credits: 15 generation + 5 rig = 20 per model, 100 total. Balance before 1010, after 910 (verified via `meshy_check_balance`).

Status: no Unreal import done — files are downloads, not engine-tested, not visually reviewed.

## Rig verdict method

`glb_stats.py` on the rigged GLB: usable iff `skinned_mesh_nodes>0`.

All rigs have the identical joint set: 24 joints (Hips, Spine, Spine01, Spine02, neck, Head, head_end, headfront, L/R Shoulder/Arm/ForeArm/Hand, L/R UpLeg/Leg/Foot/ToeBase), clip `Armature|clip0|baselayer`, textures embedded in the GLB plus `*_base_color.png` saved in Raw.

## Models

### 1. Burglar

- Generation task: `01a18381-33a5-7460-a6f3-ecc42af8929e`
- Rig task: `01a18382-adb7-70b2-920d-9dcac9f3cff8`
- Tris: target 18000, actual 17710
- Rig verdict: USABLE — 24 joints, `skinned_mesh_nodes=1`, arms/legs/head present
- Source: `docs/concepts/thief-burglar.png` directly
- Rig height: 1.65m
- Note: the concept is a lean human; `docs/GAMEPLAY.md` §5.1 says hobbit-like — concept governed.

### 2. Adventurer

- Generation task: `01a18384-62d8-7519-b97b-dd8641221d7d`
- Rig task: `01a18385-fd43-76a2-a5a8-43cef522e4e1`
- Tris: target 20000, actual 20027
- Rig verdict: USABLE — same joint set as above
- Source: `docs/concepts/thief-adventurer.png`
- Rig height: 1.8m
- Discrepancy: the brief asked for shield + sword; the approved concept has a sheathed sword + bow/quiver and no shield — no shield was generated, concept governed. Owner decision needed if a shield is wanted.

### 3. Dwarf Guard (left dwarf)

- Generation task: `01a18388-3d8f-7767-8a3a-f9082fae204f`
- Rig task: `01a18389-82cf-77c1-b863-52f58ae9e59a`
- Tris: target 15000, actual 15480
- Rig verdict: USABLE — same joint set as above
- Source: crop `Content/Meshy/Thieves/_crops/dwarf-guard.png` from `thief-dwarf-party.png`
- Rig height: 1.25m

### 4. Dwarf Trapper (centre)

- Generation task: `01a1838b-629d-73ea-adfd-3982cc9750ad`
- Rig task: `01a1838e-1115-76d9-bf02-a54bae862243`
- Tris: target 15000, actual 15665
- Rig verdict: USABLE
- Source: crop `dwarf-trapper.png`
- Rig height: 1.25m

### 5. Dwarf Carrier (right dwarf)

- Generation task: `01a1838f-cfb6-7045-b2ad-954808260989`
- Rig task: `01a18392-2a6f-739f-b760-d885fda84fc3`
- Tris: target 15000, actual 15435
- Rig verdict: USABLE
- Source: crop `dwarf-carrier.png`
- Rig height: 1.25m

## Dwarf party role mapping

Per `docs/GAMEPLAY.md` §5.2:

- Carrier carries loot.
- Trapper handles traps + lantern/rope.
- Guard blocks, with axe.

## File layout

Per model `<slug>` in `burglar`, `adventurer`, `dwarf_guard`, `dwarf_trapper`, `dwarf_carrier`:

- Raw (never hand-edited): `Content/Meshy/Raw/Thieves/<slug>.fbx`, `<slug>.glb`, `<slug>_base_color.png`, `<slug>_rigged.fbx`, `<slug>_rigged.glb`
- Working copies: `Content/Meshy/Thieves/<slug>_rigged.fbx` and `.glb` (byte copies of the rigged Raw files)

Dwarf crops are in `Content/Meshy/Thieves/_crops/`, with neighbour edge fragments painted out for the trapper and carrier crops.

## Caveats

- `pose_mode t-pose` re-posed the models from dynamic concept poses, so held props (axes, lantern, satchel, bow) may be fused or shifted — visual review in engine/Blender is needed before use.
- Walk/run rig clips and preview renderings saved locally 2026-10-28 (owner rule, docs/ASSET-PIPELINE.md step 5): each rig's walking/running withSkin GLB+FBX and armature GLB in `Content/Meshy/Raw/Thieves/<slug>_walking.*` / `_running.*` (plus armature GLBs), working GLB/FBX copies in `Content/Meshy/Thieves/`, and `<slug>_preview.png` in both. Animation GLBs verified (`walking_man` / `running` clips, skinned). Nothing is left only behind a signed link.
- Signed links expire ~2026-10-31; saved files are permanent.
- Scale is metres, Y-up; Unreal import needs Convert Scene Unit.
