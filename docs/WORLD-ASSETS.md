# WORLD-ASSETS.md — Exterior world asset pack (Meshy)

**Date:** 2026-10-28 · **Status: CONCEPTS AWAITING OWNER APPROVAL — no 3D generated yet**
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

## 4. Approval gate

Reply with approval (or per-asset cuts/changes) and the 3D phase starts with the village chunk. Without approval, nothing in §2 runs.
