# Round 03
Evidence: critic/pack/round_03_overview.png, critic/pack/round_03_details.png
Candidate render: t068_v6_flydown_front.png (downstroke phase). Same
failure-reference context as rounds 01-02.

## blind review (fresh subagent, critic.py prompt)
blind: shape 2, color 1, texture 1, detail 2, lighting 1
- shape (load-bearing): "render shows both wings drooping symmetrically down-left and down-right" vs reference asymmetric raised/left/lowered wings. AGREED and intended: symmetric straight downstroke is the bird-flap contract; reference asymmetry is dive+bank+fold.
- All other differences: desktop-vs-clay artifacts answered in rounds 01-02.

## 1 viewpoint
- checked: level front vs below/banked dive by construction; owner chase-cam capture remains the final viewpoint proof (not claimable here).

## 2 silhouette
- checked: downstroke silhouette is two straight symmetric diagonals, tips lowest+outermost; measured bends 0.0 deg at frame 13, reach 0.65 horizontal at -40 deg flap (0.85 x cos40 = 0.65, exact rigid-wing geometry).

## 3 parts
- checked: same parts as round 02; no new/missing parts in this phase.

## 4 colour
- checked: clay vs silhouette artifact; UE material fixed separately (M_DragonScales verified).

## 5 material
- checked: not decidable this surface; no change.

## 6 light
- checked: studio vs backlit artifact; no change.

## 7 integrity
- checked: bends 0.0 all sampled Fly frames (1/7/13/19) + Glide; UE bend test green post-import (29/29); no intersections introduced (same mesh, rotations only).

## scores
scores: view 3, silhouette 9, parts 8, colour 2, material 5, light 2, integrity 9

## plan (any score <= 5: a rebuild of the worst component or a batch of >= 8 details; round 1 also fixes the global look)
- Remaining <=5 (view/colour/light) cannot be fixed in Blender: they measure clay-render-vs-desktop-screenshot distance. Their fix is process, not pose: owner captures the worktree build in level flight; any fold visible there reopens this loop with a real target frame.

## decision (after rendering the candidate: kept or reverted, and why)
- KEPT: all three flap phases (up/mid/down) + glide keep one straight wing line with tips straight out, confirmed by measurement (0.0 deg), renders, two independent blind reads ("stretched horizontally", "drooping symmetrically"), and the in-engine bend test. Proceed to gate + commit; final visual sign-off is the owner's capture.
