# Round 02
Evidence: critic/pack/round_02_overview.png, critic/pack/round_02_details.png
Candidate render: t068_v6_flymid_front.png (post-solver). Same failure-
reference context as round 01: target is observations.md acceptance, not
pixel-matching the desktop screenshot.

## blind review (fresh subagent, critic.py prompt)
blind: shape 2, color 1, texture 1, detail 2, lighting 1
- shape (load-bearing): "wings - reference shows one long wing raised steeply upward and the other angled downward, render shows both wings stretched horizontally at the same height". AGREED and intended: horizontal straight span at mid-stroke is the owner contract; reference asymmetry is dive+bank+fold.
- shape pose/head, all color/texture/detail/lighting differences: same desktop-vs-clay artifacts answered in round 01 (magenta clay, black void, front light, no UI/desktop). Agreed, irrelevant to pose contract.

## 1 viewpoint
- [minor] level front vs reference below/banked dive -> by construction; final viewpoint proof owes owner chase-cam capture. GLOBAL FRAMING/EXPOSURE/SHADOW lines are the same desktop artifact, answered round 01.

## 2 silhouette
- checked: mid-stroke silhouette is one wide horizontal line, aspect 3.26 (pack), tips are the widest points. Measured bends 0.0 deg, reach 0.85/0.85 (probe_straight on exported FBX). Meets observations acceptance (collinear <=10 deg, span >=95%).

## 3 parts
- checked: same mesh/rig parts; legs tucked (accepted); tail judged on top view (round 01).

## 4 colour
- checked: clay magenta vs backlit silhouette — not judged (round 01 answer stands; UE material fixed separately).

## 5 material
- checked: not decidable this surface; no change.

## 6 light
- checked: studio clay vs sun-backlit — artifact, no change.

## 7 integrity
- checked: WING_LEN_CHECK exact on rebuilt v5; solver bends 0.0; UE bend test red on old clips (43.7/109.2 deg), green after reimport (29/29).

## scores
scores: view 3, silhouette 9, parts 8, colour 2, material 5, light 2, integrity 9

## plan (any score <= 5: a rebuild of the worst component or a batch of >= 8 details; round 1 also fixes the global look)
- No pose rebuild: silhouette/integrity meet acceptance. Remaining <=5 scores are clay-vs-desktop artifacts whose fix is the owner in-game capture, not Blender work. Round 3 checks the downstroke phase, then gate.

## decision (after rendering the candidate: kept or reverted, and why)
- KEPT: independent blind grader reads the candidate wings as "stretched horizontally" — the owner contract — and measurements agree (0.0 deg, full reach).
