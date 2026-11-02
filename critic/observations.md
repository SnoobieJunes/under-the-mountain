# Observation sheet
Reference: owner screenshot 2026-11-02 (failure reference, worktree build, in-game).
Owner acceptance in words: wingtips must not touch the dragon, must point
straight out, legs tucked is fine, wings must fully extend, fully extended
wingspan flapping like a bird.

## 1. Orientation and view
- Game window centered on a macOS desktop; dragon is the small dark silhouette
  at the center of the game viewport.
- Head/tail ends are not reliably separable in the silhouette. Body axis reads
  diagonal (rolled roughly 40-50 degrees in screen space), not level.
- Visible surfaces: backlit underside/silhouette only. No top surface detail,
  no readable head, legs tucked/indistinct against the body.
- Apparent wing tips: one long thin tip points screen-up (approx x 0.515,
  y 0.315 of the full screenshot), one points down-right (approx 0.545/0.665),
  one triangular membrane points screen-left (approx 0.415/0.495). The tips do
  NOT form one straight lateral line through the body; the silhouette is an
  X/dart/chicken fold with surfaces crossing near the body center.
- Camera: low, looking up into bright sky/sun. Strong perspective: 4-5 white
  scent ribbons converge on the dragon. Debug readout (zoom tile) reads Dive,
  speed ~1058, vSpeed ~1051 — this frame is a dive, not level cruise.

## 2. Frame
- Game window approx left 0.20, top 0.17, right 0.795, bottom 0.835 of the
  desktop screenshot. Dragon bbox approx left 0.40, top 0.31, right 0.56,
  bottom 0.68 (width:height ~0.44 — tall/narrow because rolled, the opposite
  of a wide straight wingspan read).

## 3. Parts (largest first)
- Sky/sun background fills the viewport; pale gold center-right, dark umber left.
- Dragon body: small dark central mass, roughly 15% of viewport width.
- Wings: two main membranes + one long finger-like tip; folded/swept so their
  projected span inside the viewport is far below a straight tip-to-tip line;
  trailing surfaces hug/cross the body instead of extending laterally.
- Legs: tucked, not separately readable (owner: tuck itself is fine).
- Tail/head: merged into silhouette; no clear neck/head line.
- UI: intro choice bar at top ("Let it go / CHASE IT"), title text, and a
  small yellow-green debug readout at upper-left of the viewport.

## 4. Colour map
- Dragon: near-black warm umber silhouette, essentially one value.
- Sky: cream to pale gold, blown near-white center-right; dark brown at left.
- Scent ribbons: white. HUD bars: light grey/white; debug text yellow-green.

## 5. Material map
- No scale texture, normal detail, or specular read is visible on the dragon
  at this exposure; it reads flat matte black. (Separate UE probe after this
  screenshot proved slot 0 was WorldGridMaterial; fixed to M_DragonScales and
  verified in a fresh process — texture state must be re-captured, not assumed
  from this sheet.)

## 6. Light
- Key behind the subject (sun/sky), camera side in near-total shadow.
- No contact shadow (airborne). Scent ribbons are the only bright lines
  crossing the subject.

## 7. Background
- In-game: sky gradient only in this frame. Outside the window: macOS desktop
  icons/widgets (ignore for pose work).

## 8. Signature details (failure markers to eliminate)
1. No straight tip-to-tip line at any point of the silhouette.
2. One wingtip stands vertical like a blade instead of lateral.
3. Opposite wing hangs/sweeps down along the body.
4. Membrane triangles cross near the body center (chicken-fold read).
5. Body rolled diagonal; span reads tall/narrow rather than wide.
6. Wingtips visually close to the body axis compared with membrane size.
7. Dive state in the readout — level-flight flap still needs its own capture.

## Acceptance numbers for the fix (from owner words, made measurable)
- Straight-out: at mid-stroke, shoulder->elbow->wrist->finger are collinear
  within 10 degrees per joint (posed, evaluated in Blender, both sides).
- Fully extended: tip-to-tip span at mid-stroke >= 95% of the rig's maximum
  geometric span (sum of segment lengths + shoulder separation), and each tip
  is the farthest point of its wing from the body axis (no tip folded back).
- Tips clear: every point of the outer wing (wrist+finger segments) stays at
  least one body half-width away from the body surface line in top view
  through the whole Fly cycle (sampled frames 1/7/13/19/24).
- Bird flap: flap rotation concentrated at the shoulder; outer joints keep
  the straight line (only small lag), so the whole straight wing sweeps
  up/down instead of folding at elbow/wrist mid-stroke.
