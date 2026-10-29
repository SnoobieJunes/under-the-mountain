# WORLD-ASSETS.md — Exterior world asset pack (Meshy)

**Date:** 2026-10-28 · **Status: 3D COMPLETE 2026-10-28 (owner approved concepts same day) — goat rig FLAGGED, see §5**
**Scope honoured:** files only under `Content/Meshy/World/`, `Content/Meshy/Raw/World/`, and this report. No Source/, Maps, TASKS.md, AGENTS.md, Scripts/, and no editor import were touched.
**Style:** Style B grounded realism per `docs/ART-DIRECTION.md` — muted earthy stone/timber/slate, tarnished gold `#AF8658`, crimson `#6E1423–#8A1F2D` accents only, 2K texture mindset, no text/watermarks.
**Concept method:** Muse subscription via `muse_turn` (chunked, files-on-disk verified), Muse Image `muse-image-1.0` direct Meta API (T-025 recipe). Reference-sheet framing: single object, neutral grey `#AEABA4`, even light — chosen so each concept can feed Meshy image-to-3D directly after approval.

---

## 1. Concept inventory (27 images, 52 MB, all verified non-zero on disk)

Contact sheets: `Content/Meshy/World/concepts/contact-village-castle.jpg`, `contact-nature-destruct.jpg`

| ID | Concept file | Bytes | Planned 3D asset | Tri budget |
|---|---|---|---|---|
| VIL-01 | vil-house-a.png | 2,146,900 | House A — timber cottage | ≤25,000 |
| VIL-02 | vil-house-b.png | 2,454,686 | House B — stone cottage | ≤25,000 |
| VIL-03 | vil-house-c.png | 2,586,320 | House C — jettied 2-storey | ≤25,000 |
| VIL-04 | vil-inn.png | 1,944,808 | Inn/tavern (blank sign, proven) | ≤25,000 |
| VIL-05 | vil-market-stall.png | 1,905,465 | Market stall | ≤8,000 (prop) |
| VIL-06 | vil-well.png | 1,675,204 | Stone well | ≤8,000 |
| VIL-07 | vil-fences.png | 2,640,242 | Village fence run | ≤8,000 |
| VIL-08 | vil-cart.png | 2,079,417 | Loaded cart | ≤8,000 |
| CAS-01 | cas-keep.png | 2,285,161 | Castle keep (modular) | ≤25,000 |
| CAS-02 | cas-wall.png | 2,251,230 | Wall segment, flat tiling ends | ≤25,000 |
| CAS-03 | cas-tower.png | 2,287,433 | Round tower (modular) | ≤25,000 |
| CAS-04 | cas-gate.png | 2,227,196 | Gate segment (modular) | ≤25,000 |
| LIV-01 | liv-goat.png | 1,508,859 | Goat — rig + verdict (see §3) | ≤18,000 |
| DES-01a | des-crates-intact.png | 1,978,938 | Crates intact | ≤8,000 |
| DES-01b | des-crates-rubble.png | 2,171,923 | Crates rubble | ≤8,000 |
| DES-02a | des-barrels-intact.png | 1,800,303 | Barrels intact | ≤8,000 |
| DES-02b | des-barrels-rubble.png | 2,306,166 | Barrels rubble | ≤8,000 |
| DES-03a | des-cart-intact.png | 1,467,964 | Cart intact (destructible) | ≤8,000 |
| DES-03b | des-cart-rubble.png | 2,343,616 | Cart rubble | ≤8,000 |
| DES-04a | des-fence-intact.png | 1,245,682 | Fence run intact | ≤8,000 |
| DES-04b | des-fence-rubble.png | 1,742,526 | Fence run destroyed | ≤8,000 |
| NAT-01 | nat-rock-a.png | 2,350,661 | Rock formation A — boulder cluster | ≤8,000 |
| NAT-02 | nat-rock-b.png | 1,978,698 | Rock formation B — spire | ≤8,000 |
| NAT-03 | nat-rock-c.png | 1,997,938 | Rock formation C — slab/overhang | ≤8,000 |
| NAT-04 | nat-deadtree-a.png | 1,835,740 | Dead tree A — tall gnarled | ≤8,000 |
| NAT-05 | nat-deadtree-b.png | 1,308,971 | Dead tree B — split trunk | ≤8,000 |
| NAT-06 | nat-pine.png | 2,176,391 | Pine | ≤8,000 |

Spot-checked visually (2026-10-28): goat = exactly 4 legs / 1 head / 2 horns, legs separated, clean silhouette; keep, inn and house A = single objects, neutral background, muted palette, blank banner/sign, no people/text/watermark.

## 2. Planned 3D phase (NOT started — waits for owner approval of §1)

Per asset, after approval:

1. Meshy `meshy_image_to_3d`: `model_type: smart-topology`, `ai_model: meshy-t2.5`, textured, `target_polycount` = budget above (buildings 25,000; props 8,000; goat 18,000), `target_formats: ["fbx","glb"]` (FBX for Unreal, GLB for preview/verification).
2. Download FBX + GLB + base-colour textures the same session to `Content/Meshy/Raw/World/` (raw, never hand-edited); working copies in `Content/Meshy/World/`. Nothing left only behind a signed link (links die 3 days after task finish).
3. Verify every file non-zero; count actual triangles (`glb_stats.py`), record task IDs, paths, sizes in this report. Smart Topology lands near, not on, target and may return separate parts — reported as-is, merged on import by the main session.
4. Scale: Meshy exports metres, Y-up. Unreal import must keep **Convert Scene Unit** on (or scale ×100) or assets arrive 100× too small. No editor import in this pack — files + report only.

**Cost estimate (Meshy credits):** 27 × 15 (smart-topology image-to-3D textured) = 405, + goat rig 5 = **410 credits total**, downloads free. That exceeds the default 200-credit per-chat spend gate, so the 3D phase will run in approved chunks (village / castle / destructible / nature+goat), each confirmed before spending.

## 3. Known risks, flagged up front

- **Goat rig (LIV-01):** Meshy auto-rig is built around humanoids and this project's own foot-gun list says auto-rig is unreliable for quadrupeds. Plan: generate goat, attempt `meshy_rig` (5 cr), then run the rig verdict — `glb_stats.py` skinned-mesh check (`skinned_mesh_nodes > 0`) plus legs/head usability. If it fails, it is **flagged here, not fixed in Blender**.
- **Ground patches:** several building concepts sit on a small mud/moss patch; image-to-3D may bake that patch into the mesh. Main session may need to trim it at import; noted per asset when the 3D lands.
- **Smart Topology smoothing:** fine engraving/bevels soften and thin hanging parts can drop (measured in this project on earlier assets). Acceptable at gameplay distance for buildings/props; castle trim is the watch item.
- **Destructible pairs:** intact/rubble were concepted with matched materials but are separate generations — silhouettes will not align 1:1. They read as before/after at gameplay distance, not as a fracture simulation.

## 4. Approval gate (closed)

Owner approved the concept set 2026-10-28 ("approved go"); §5 ran immediately after.

## 5. 3D results (2026-10-28)

Method: Meshy `meshy_image_to_3d`, `ai_model: meshy-t2.5`, `model_type: smart-topology`, textured @ 2K, `target_formats: ["fbx","glb"]`, `target_polycount` per budget. Meshy MCP tools were not bound in this session, so the plugin's pinned server (`@meshy-ai/meshy-mcp-server@0.6.5`, cached npx package) was driven over stdio JSON-RPC — same tools, same key, no second MCP registration.
Downloads (all same session, nothing left only behind a signed link): FBX + GLB + `<name>_base_color.png` in `Content/Meshy/Raw/World/`, working copies of all three plus `<name>_preview.png` (Meshy preview render) in `Content/Meshy/World/`. Triangle counts are ACTUAL, counted from the GLBs with `glb_stats.py`.
**Credits: 27 × 15 + 5 rig = 410 spent. Balance 910 → 500 (proven by `meshy_check_balance` before/after).**

| Asset | Task ID | Actual tris | Budget | FBX bytes | GLB bytes |
|---|---|---|---|---|---|
| vil-house-a | 01a185a8-a738-73df-81cc-ac24498b57c0 | 23,546 | 25,000 ✓ | 6,324,956 | 4,372,280 |
| vil-house-b | 01a185a9-f85d-71c9-a272-96a59dae6425 | 24,548 | 25,000 ✓ | 6,256,684 | 4,204,932 |
| vil-house-c | 01a185aa-d3a0-7670-9b03-477d8557c9a8 | 24,201 | 25,000 ✓ | 6,299,036 | 4,382,808 |
| vil-inn | 01a185ab-5ec2-778e-a07d-eb1db872f7fa | 19,417 | 25,000 ✓ | 6,128,604 | 4,270,448 |
| vil-market-stall | 01a185ab-f3d1-74cd-8af4-df078cb8a158 | 7,161 | 8,000 ✓ | 6,429,996 | 3,690,064 |
| vil-well | 01a185ac-93d9-715e-a81d-eb0cfb12bbd0 | 7,900 | 8,000 ✓ | 6,994,332 | 4,017,912 |
| vil-fences | 01a185ad-161b-776e-b5f3-28f8f9c216d5 | 7,790 | 8,000 ✓ | 4,996,940 | 3,649,324 |
| vil-cart | 01a185ad-8c96-724b-9ac6-13aa391a3f6e | 7,387 | 8,000 ✓ | 5,497,676 | 3,468,232 |
| cas-keep | 01a185ae-4213-734b-b371-78ee781eafaf | 22,361 | 25,000 ✓ | 5,713,980 | 4,462,260 |
| cas-wall | 01a185ae-dfb6-75a2-8547-1cc013dc3f10 | 23,360 | 25,000 ✓ | 7,148,524 | 4,886,992 |
| cas-tower | 01a185af-6229-775a-91fa-b549d1be21a5 | 24,750 | 25,000 ✓ | 6,921,964 | 4,670,472 |
| cas-gate | 01a185b0-0cac-70da-9c3c-a58377187e8f | 23,661 | 25,000 ✓ | 5,842,892 | 4,415,676 |
| des-crates-intact | 01a185b1-3897-72c6-880d-59ba1ba2e62b | 7,671 | 8,000 ✓ | 4,828,108 | 3,428,280 |
| des-crates-rubble | 01a185b1-e2bd-76aa-b003-19ddd47b1e81 | 6,563 | 8,000 ✓ | 4,370,652 | 2,904,844 |
| des-barrels-intact | 01a185b2-6be8-74bf-90d5-227f0e51de82 | 7,347 | 8,000 ✓ | 4,359,644 | 3,112,684 |
| des-barrels-rubble | 01a185b2-fc5d-7782-92ec-1c117f36bcba | 7,142 | 8,000 ✓ | 4,516,476 | 3,119,532 |
| des-cart-intact | 01a185b3-72a7-77a7-a51c-7309cf412247 | 8,248 | 8,000 ⚠ +3% | 6,157,740 | 3,622,888 |
| des-cart-rubble | 01a185b3-e7e1-7243-a6ea-e99f95724f9e | 7,426 | 8,000 ✓ | 4,599,724 | 2,908,552 |
| des-fence-intact | 01a185b4-5687-7636-aa56-8fcb9d5a32c4 | 8,451 | 8,000 ⚠ +6% | 6,018,268 | 3,172,092 |
| des-fence-rubble | 01a185b4-af93-76b9-9f04-12c8c0c71a6f | 7,848 | 8,000 ✓ | 5,451,788 | 3,728,648 |
| nat-rock-a | 01a185b5-5942-72b1-b89c-f91f490254c8 | 8,275 | 8,000 ⚠ +3% | 7,194,524 | 3,959,516 |
| nat-rock-b | 01a185b5-efb1-71fb-b50a-8c0b3722533b | 7,924 | 8,000 ✓ | 6,169,772 | 3,825,616 |
| nat-rock-c | 01a185b6-59f4-7321-9dd5-c956916fc6eb | 8,293 | 8,000 ⚠ +4% | 7,314,988 | 4,085,964 |
| nat-deadtree-a | 01a185b6-c83c-738b-9bc2-5b1cb97dc865 | 7,782 | 8,000 ✓ | 5,178,380 | 3,794,732 |
| nat-deadtree-b | 01a185b7-29fd-72f8-9a4d-5526f284ca5e | 7,227 | 8,000 ✓ | 3,159,548 | 2,239,336 |
| nat-pine | 01a185b7-894f-709c-965e-6a8e4a7b916b | 6,775 | 8,000 ✓ | 4,280,092 | 2,711,292 |
| liv-goat | 01a185b7-eab6-71be-81b9-ae035e270fc2 | 14,291 | 18,000 ✓ | 5,296,124 | 3,923,720 |

⚠ overruns are Smart Topology landing near, not on, target (4 assets, +3–6%). Not remeshed: a remesh pass (5 cr each) would land near target again and risks texture/topology loss for ≤451 tris; flagged for the main session to accept or remesh at import.

Verified: Raw = 28 FBX / 30 GLB / 27 base-colour (399 MB); World = same working copies + 27 previews (322 MB incl. concepts); **zero zero-byte files**.

**Not game-ready, as-is:** no LODs, no collision, no hand retopology; scale/pivot may need fixing. Meshy exports metres, Y-up — Unreal FBX import must keep **Convert Scene Unit** on (or ×100). No editor import was run; integration is the main session's step.

### Goat rig verdict — FLAGGED, quadruped rig FAILED (not fixed, per brief)

- Rig task `01a185b8-a570-768f-a44a-c06328bb1e6d` (5 cr, `height_meters: 1.0`) reported SUCCEEDED. Saved locally same session (rig downloads are signed links, fetched immediately): `liv-goat_rigged.glb` (4,572,924 B), `liv-goat_rigged.fbx` (4,195,036 B), `liv-goat_walk.glb` (clip `Armature|walking_man|baselayer`), `liv-goat_run.glb` (clip `Armature|running|baselayer`) in both Raw and World.
- **Skinned mesh: present.** `glb_stats.py` on `liv-goat_rigged.glb`: `skinned_mesh_nodes: 1`, 1 skin, 24 joints, 14,291 tris.
- **But the skeleton is a biped, not a quadruped.** Joint names verbatim: `Hips, Spine02, Spine01, Spine, neck, Head, head_end, headfront, LeftShoulder, LeftArm, LeftForeArm, LeftHand, RightShoulder, RightArm, RightForeArm, RightHand, LeftUpLeg, LeftLeg, LeftFoot, LeftToeBase, RightUpLeg, RightLeg, RightFoot, RightToeBase`. Front legs are driven by a human arm chain (Shoulder/Arm/ForeArm/Hand), hind legs by the leg chain; there is **no tail joint** and only a 2-joint neck/head. The walk clip is literally named `walking_man`.
- **Verdict: legs/head not usable as a quadruped rig.** The mesh is bound and will deform, but gait and posing will follow human arm/leg articulation, not goat anatomy. Flagged here exactly as briefed — no Blender fix attempted. The unrigged goat (`liv-goat.fbx/.glb`, 14,291 tris) is clean for a hand-built quadruped rig in the main pipeline.

## 6. Blender pass (2026-10-28, owner: "blend them yes — fix the goat's walking")

### 6a. Goat de-cursed — proper quadruped rig (replaces the §5 verdict's rig, not the mesh)

Owner overrode the earlier do-not-fix brief. Delegated via `muse_turn` per muse-harness orchestration: Muse's sandbox cannot run Blender (Metal GPU-detection segfault, verified-empty return, script + log staged in /tmp), so Muse authored `/tmp/goat_rig.py` and the parent executed and verified it (harness rule: driving a stalled chunk's finished artifact to completion is parent verification work).

- **Rig:** 21 bones, quadruped names only — `root, pelvis, spine, chest, neck, head, tail_01–03`, `leg_F_L/R` + `leg_B_L/R` each `_upper/_lower/_foot`. No human bone names. Fitted to the measured bbox; goat faces −Y after glTF import.
- **Skinning:** Blender Bone Heat failed on this Meshy mesh (`failed to find solution`, 0/17,971 verts). Fallback shipped: distance-to-bone-segment weighting with region gating (leg bones may only claim verts <0.075 from a leg segment and low on the body; belly fur stays with spine/pelvis/chest). 17,971/17,971 verts weighted. Two parent iterations: belly sawtooth largely eliminated; chest/shoulder fur-card tearing remains — those black fur-card gaps are present in the **unrigged source** render too (Meshy fur shells), amplified at the shoulder joint. Legs: no twist/collapse, correct knee/hock directions in all 4 checked frames.
- **Walk:** `goat_walk_loop`, keyframed quadruped gait (diagonal pairs F_L+B_R / F_R+B_L half-cycle offset, body bob 2× stride, head counter-bob, tail sway), 24 fps, frames 1–24 with f24 = f1, **in place** (no root travel).
- **Files:** `Content/Meshy/World/liv-goat_quad.blend` (4.0 MB), `.glb` (4.2 MB), `.fbx` (1.2 MB), walk renders `liv-goat_quad_walk_f01/f08/f15/f22.png`. The cursed Meshy biped files (`liv-goat_rigged.*`, `liv-goat_walk/run.glb`) are kept in Raw for reference — do not ship them.
- **Verified:** `glb_stats.py liv-goat_quad.glb` → 14,291 tris, `skins: 1, joints: 21, skinned_mesh_nodes: 1, clips: ["goat_walk_loop"]`. Visually reviewed in 4 walk frames (downloaded ✓, reopened-validated ✓, visually reviewed ✓ — **not** artist-approved, **not** engine-tested).

### 6b. Optimized copies for game testing (all 27 assets + quad goat)

Plugin `blender_handoff.py` (Blender 5.2.0), one run per asset into `Content/Meshy/World/blender/`: `<name>_optimized.blend/.glb/.fbx` (28 sets, 342 MB, zero zero-byte files). No decimation except the four §5 over-budget props, now at budget: des-cart-intact 8,248→8,000, des-fence-intact 8,451→8,000, nat-rock-a 8,275→8,000, nat-rock-c 8,293→7,999. Textures already 2048², packed in every output; `--max-texture 2048` changed nothing.

**Every output reopened in a fresh Blender process and passed** (`verify.pass`, triangles > 0, images have pixel data); `lost: []` everywhere. `liv-goat_quad_optimized.*` preserved armature + action 1/1 in all three formats (re-checked with `glb_stats.py`: skin/joints/clip intact).

State, stated plainly: these are **optimized copies for game testing — reopened-validated, not game-ready** (no LODs/collision/hand retopology, not tested in Unreal). GLB stays metres/+Y-up; FBX is −Z forward/Y-up with embedded textures.
