# Round 04
Evidence: critic/pack/round_04_overview.png, critic/pack/round_04_details.png
Candidate: detached-membrane + gull-slope candidate (t068_v6_glide_front/top).
Owner decisions this round (structured input): membrane detach fully, wing
line = Image #2 gull slope, merge into branch 22 only after owner OK.

## blind review (fresh subagent, critic.py prompt)
blind: shape 3, color 1, texture 2, detail 2, lighting 1
- shape: reference diagonal diving silhouette, one wing raised one lowered -> render symmetrical front glide, both wings spread horizontally. AGREED/intended (owner contract inverts the failure reference).
- All other differences: desktop-vs-clay artifacts, answered rounds 01-03.

## 1 viewpoint
- checked: level front + top judged together; owner chase-cam capture remains final proof. No Blender fix available for viewpoint gap.

## 2 silhouette
- checked: glide front shows gull slope (shoulder apex, straight line out/down to tips); top view shows trailing edge ending at the flank with a clear gap to hips/tail. Bends 0.0 deg; glide reach 0.83 horizontal at -15 deg dihedral (0.85 x cos15 = 0.82, exact).

## 3 parts
- [minor] inner trailing edge scallops where chest-blended verts retained weight -> cause: root blend kept deliberately -> fix: none this pass; judge in owner capture (reads as feathering at this scale).
- checked: legs tucked again after wing-dominant-only detach (19,189 verts; thigh regression from first detach attempt caught in renders and fixed before import).

## 4 colour
- checked: clay artifact; UE material verified M_DragonScales separately.

## 5 material
- checked: not decidable this surface; no change.

## 6 light
- checked: studio vs backlit artifact; no change.

## 7 integrity
- checked: lengthen asserts pass; detach assert passes (no wing-dominant vert keeps >0.01 hip weight); bends 0.0; folded width 0.485 unchanged.

## scores
scores: view 3, silhouette 9, parts 8, colour 2, material 5, light 2, integrity 9

## plan (any score <= 5: a rebuild of the worst component or a batch of >= 8 details; round 1 also fixes the global look)
- Pose contract met in Blender evidence. Remaining low scores are surface/view artifacts whose only fix is the owner's in-game capture gate. Proceed to UE import + tests + gate; STOP before merge (owner approval gate).

## decision (after rendering the candidate: kept or reverted, and why)
- KEPT for UE import: detached bird-like wings on the Image #2 gull line, straight at every sampled phase, hips clear in top view. Merge explicitly NOT done — awaits owner "good" on a worktree capture.
