# Round 01
Evidence: critic/pack/round_01_overview.png, critic/pack/round_01_details.png
IMPORTANT CONTEXT: the reference is the owner's FAILURE screenshot (in-game
dive, backlit). The target is the owner's verbal acceptance in
observations.md (tips straight out, fully extended, bird flap), i.e. the
INVERSE of the reference wing silhouette. Colour/texture/lighting/framing
differences vs a desktop screenshot are artifacts and are answered as such.
Finding format: - [critical|major|minor] where: reference shows X -> render shows Y -> cause -> fix

## blind review (fresh subagent, critic.py prompt; first attempt died on quota, retry landed)
blind: shape 3, color 1, texture 2, detail 2, lighting 1
- shape: diagonal banked small dragon in window vs large symmetrical head-on glide; long tail up-right vs no tail visible; narrow angular V/X wings vs broad flat horizontal wings with drooping tips.
- color: near-black silhouette vs uniform magenta; gold sky vs black background; desktop frame colours vs none.
- texture: smooth silhouette vs heavily sculpted scales; plain membranes vs veined sculptural membranes; photographic desktop vs black void.
- detail: indistinct head vs large horned face; UI/desktop text vs none; profile legs/tail vs tucked forelegs + chevron chest.
- lighting: sun behind with rays vs no source; backlit shadow side vs even front light; warm gradient vs uniform black.
Answers: every colour/texture/detail/lighting difference is agreed-but-irrelevant (clay Workbench render vs desktop screenshot; not the deliverable surface). Shape #1/#2 are viewpoint/state (dive+bank+below vs level front) — answered in critic 1. Shape #3 is the actual contract: the reference V/X fold is the defect to eliminate, and even the pre-fix render already departs from it toward flat/horizontal; measured bone bends (below) decide, not this pack's framing.

## 1 viewpoint
- [major] whole frame: reference shows desktop + game window, dragon ~16% wide, rolled, from below in Dive -> render shows level front glide at 76% wide -> cause: different state/camera by construction (pack GLOBAL FRAMING/EXPOSURE lines are this artifact) -> fix: none in Blender; final acceptance needs owner in-game capture from chase cam in level flight.

## 2 silhouette
- [critical] wings: reference shows narrow V/X fold, tips near body axis -> pre-fix render shows broad flat wings but evaluated bone bends measured 30-57/100-113/115-125 deg (elbow/wrist/finger) with level reach only 0.52 of 0.85 chain -> cause: per-joint cumulative Euler guesses + v5 lengthening corrupted by connected edit-bone measurement (elbow shipped 0.148 not 0.288) -> fix: direction-solver wing poser (one straight target line per side) + precomputed-joint v5 rebuild. Post-fix measured: bends 0.0 deg, level reach 0.85/0.85, glide span 1.72.

## 3 parts
- [minor] tail/head: reference silhouette merges them -> render front view hides tail behind body -> cause: viewpoint -> fix: judged on top/side renders (t068_v6_*_top/side), where tail/head lines are present and straight.
- checked: legs tucked in both; owner accepts tuck. No invented parts (same 33-bone rig/mesh).

## 4 colour
- [minor] body: reference near-black vs render magenta clay -> cause: Workbench TEXTURE color-type on untextured FBX round-trip + backlight -> fix: none here; in-game material bug (WorldGridMaterial) already fixed/verified separately in UE (M_DragonScales). Colour is not judged this round.

## 5 material
- checked: membrane/scale material read is not decidable from a backlit silhouette vs clay render; deferred to owner in-game capture. No material change this round.

## 6 light
- checked: sun-behind-subject vs studio clay light — incomparable by construction; GLOBAL EXPOSURE line answered as artifact. No light change this round.

## 7 integrity
- [major] wing chain: pre-fix evaluated pose zigzags (bends above) and v5 bone lengths disagreed with displaced verts -> cause: connected edit-bone measurement during lengthening -> fix: precomputed joints + WING_LEN_CHECK asserts in lengthen_wings.py (all 8 segments exact), solver bends 0.0 deg post-fix.
- checked: no floating/intersecting new parts; same mesh/rig; folded width 0.485 unchanged.

## scores
scores: view 3, silhouette 8, parts 7, colour 2, material 5, light 2, integrity 8
(Scores judge the POSE CONTRACT from observations.md, not pixel-matching a
desktop screenshot; colour/light stay low by construction and cap nothing —
they are answered artifacts above. Silhouette/integrity scored on post-fix
measured bends/reach + v6 renders, not the pre-fix pack render.)

## plan (any score <= 5: a rebuild of the worst component or a batch of >= 8 details; round 1 also fixes the global look)
- Rebuild (done this round): direction-solver wing poser replacing per-joint Euler guesses; single flap angle for the whole straight wing; v5 lengthening rebuilt from precomputed joints with length asserts.
- Verify: probe_straight bends/reach on exported FBXs; v6 front/top/side renders at up/mid/down/glide; UE bend test fails on old clips, passes on new.

## decision (after rendering the candidate: kept or reverted, and why)
- KEPT: v6 renders (t068_v6_flyup/flymid/flydown/glide) show one straight rigid wing sweeping as a unit, tips lateral and clear of the body in top view; measured bends 0.0 deg and reach 0.85/0.85 at level. Proceed to UE import + tests; final look still owes owner in-game capture.
