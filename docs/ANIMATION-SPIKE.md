# [2026-10-31 T-067] Dragon animation spike

Status: rig, pipeline, architecture, inventory, and cost findings complete;
working decisions adopted on 2026-10-31. Explicit owner answers are preserved
in substance below, and every "idk" or blank was decided by the agent under
the owner's best-judgement instruction. No new animation has been authored or
observed in PIE during this spike. T-059's CC0-only survey and the three-pose
rig diagnosis remain evidence gates.

## Executive finding

The dragon mesh and walk cycle prove the asset route can look strong:
Muse multi-images -> Meshy -> Blender -> Unreal. Animation is not on the same
footing. The runtime is still a two-clip, single-node switch with hard cuts,
a frozen bind pose at rest, and a slow-motion flap pretending to be a glide.

The way to AAA-feeling creature motion is not simply "make more clips." It is:

1. fix Fly's spread/tuck immediately (T-058);
2. replace single-node playback with a lean AnimBP whose parent class is a
   C++ AnimInstance, with state/blend logic, montage slots, additive poses, aim
   offset, and selective Control Rig secondary motion;
3. author loops programmatically where mathematics is good, hand-pose hero
   one-shots in Blender with render approval, and use runtime procedural
   layers for banking, tuck/fold, aim, foot placement, and tail/neck lag;
4. repeat the proven hero pipeline — Muse multi-images → Meshy mesh/textures
   → Blender rig/animation → Unreal — and spend zero Meshy credits on dragon
   rig/animate/motion specifically, because that route is biped-only;
5. external animation is CC0-only by owner decision; otherwise make our own.

Adopted architecture appetite: lean first — C++ AnimInstance parent, thin
AnimBP state/montage graph, additive fold/tuck/aim/bank layers — then selective
Control Rig for tail lag and foot placement once captures prove the need. This
keeps gameplay decisions in testable C++ and avoids buying a large binary graph
before the clips themselves read correctly.

## Current playback truth

The constructor puts `SK_Dragon_v4` into `AnimationSingleNode` in
`Source/UnderTheMountain/Private/Dragon/DragonCharacter.cpp:100`.
`SelectAnimState` at `DragonCharacter.cpp:155` returns only Idle, Walk, Fly,
or Glide.

On a state change, `Tick` at `DragonCharacter.cpp:718`
does one of:

- Idle: `Mesh->Stop()`, leaving the source crouch/perch bind pose frozen;
- Walk: loop the 24-frame walk;
- Fly: loop Fly at 1.0x, or 1.6x only if diving at entry;
- Glide: loop the same Fly clip at 0.25x.

There is no blend, no per-frame play-rate update, no montage interrupt, and no
pose layer. Fire, roar, eat, landing, and damage call rules only; none calls
animation. The head never aims fire. The mesh never pitches or banks.

The dragon has exactly two clips: Walk and Fly. There is no idle, takeoff,
landing, hover, roar, fire pose, eat, hit, defeat, run, or turn clip. Thieves
have walk/run only and dead thieves stop upright in bind pose. In-game goats
are still sphere/cube static meshes; the rigged goat is not wired in.

Verification so far proves wing tracks move, not that the silhouette reads as
flight. T-047 correctly diagnosed a likely stale-editor cause, but the owner's
T-058 report is about pose quality, which track sampling cannot settle.

## Rig reality

`SK_Dragon_v4` has 33 bones:

- axial: pelvis, spine, spine2, chest, neck, head, head_end;
- wings: shoulder, elbow, wrist, and one finger per side;
- legs: four 3-bone chains;
- tail: six bones.

The wing chains are folded along the body in bind. `Scripts/create_flight_anim.py:44`
documents that geometry and applies a constant unfold before flapping about
another axis. There is no jaw, tongue, eye, toe/claw chain, scapula, or wing
membrane/finger-fan chain. Head aim has only neck + head.

Skinning is voxel-proxy bone-heat transfer. The recorded envelope is clean for
legs, neck, and tail at gait/test amplitudes; wing membrane can crumple when
lifted, and extreme poses are untested. No corrective shapes exist.

The rig has enough bones for spread, flap, glide, hover, tuck, and tail work.
Its limits are bind pose, skin weights at extremes, a one-bone wing finger,
and no jaw. Those are pose/weight problems first, not a reason to re-rig blind.

### T-058 diagnosis

Fly already applies unfold offsets of 55/30/15 degrees and flap swings, but
the legs are unkeyed. `Scripts/create_flight_anim.py:113` resets every bone
and then keys only wings, pelvis, chest, and the first three tail bones.

Before changing weights or the rig, render three poses from the `.blend`:
bind, current Fly extremes, and a test full-spread pose. That separates four
possible causes: insufficient unfold, wrong visual plane from gameplay
cameras, membrane collapse, and untucked legs overwhelming the silhouette.

Expected cheapest fix: retune `create_flight_anim.py` from those renders,
key a continuous four-leg tuck and slight streamline, re-export, and extend
the pose test to leg bones and wingtip spread. **Adopted:** if the render
proves membrane collapse, do the focused Blender weight-paint pass rather than
accepting a smaller spread. Re-bind in a spread pose only if renders prove
tearing after those steps.

## Production routes

### A. Programmatic Blender keyframing

This is the proven loop route. The walk demonstrates a credible diagonal gait;
Fly uses a 24-frame sine flap with two-frame downstream lag, pelvis bob,
chest counter-pitch, and tail sway. It is deterministic, reviewable as Python,
and regenerable headlessly.

Best for: idle breathing, walk/trot improvements, Fly v2, held glide with
micro-corrections, hover cycle foundations, and repeated mechanical loops.

Ceiling: sine mathematics can look procedural without weight, anticipation,
settle, and asymmetric overlap. It is a poor primary route for roar, tail
slam, bite, landing, and hit acting. Typical cost: 0.5-1 session per loop,
including import and tests.

One reproducibility hole: no committed script was found that regenerates the
walk action from scratch. Its source apparently lives in the `.blend`, unlike
Fly. Any v1 work should either recover/document that action's generation or
treat the `.blend` as the guarded source.

### B. Hand-posed Blender keyframes with render gates

Pose extremes and breakdowns in Blender, render contact sheets from gameplay
angles, get the silhouette approved, then in-between and tune curves.

Best for: takeoff, landing, roar, tail slam, wing buffet, claw, eating, hit,
defeat, sleep/wake. This is the hero one-shot route.

Cost: roughly 0.5 session for a simple flinch; 1-2 sessions per hero action
with review rounds. The gate matters: approve 3-4 poses before spending time
on interpolation. Poses must be checked for shoulder, hip, and membrane skin
failure at the extremes, not after export.

### C. Meshy — mesh/textures yes; dragon rig/animation no

Owner review clarification (2026-10-31): we are **repeating**, not replacing,
the process that produced the current dragon. Meshy supplied the mesh and PBR
textures from the approved Muse multi-images. It did not supply the shipped
rig or dragon animations: the Meshy auto-rig returned a 24-joint humanoid with
no wing/tail bones, after which Blender produced the 33-bone quadruped rig,
skin, Walk, and Fly. The same pattern is proven on the goat (Meshy mesh →
rejected biped rig → Blender quadruped rig/walk).

Meshy's current tool surface is explicit: rigging expects a textured humanoid
biped; animation operates on that rig; text-to-motion applies to rigged
bipeds. Conclusion: keep spending Meshy credits on hero **meshes/textures**
when a new asset needs them, and on humanoid rigs for thieves/villagers.
Budget zero Meshy credits for dragon **rig/clips/motion** unless Meshy later
ships a verified quadruped route, which would need a fresh gate. Blender is
both better here (it can express wings/tail/four legs) and cheaper (0 credits).

### D. External clips + UE IK Retargeter

Quadruped locomotion is the plausible win: real authored idle/walk/trot/gallop
and lie-down motion can beat procedural sine mathematics. Flight and combat
are much weaker because few sources share a folded-bind, four-joint,
one-finger wing.

Retarget costs include two IK Rigs, chain mapping, rest-pose offsets, scale,
root/pelvis translation conventions, and licence review. **Adopted licence
bar: CC0 only.** T-059 should survey only CC0 FBX/BVH sources with named bones
and documented rest poses, preferably at least three wing joints for any flight
claim. No CC-BY and no paid packs under this decision; if CC0 supply is thin,
T-059 reports the shortfall honestly and we author our own.

Estimated setup: 0.5-1 session per source, then minutes per usable clip.
Licence and style mismatch are the real gates, not file conversion.

### E. Runtime procedural layers

AnimBP and Control Rig are not substitutes for clips; they provide motion no
single clip should bake:

- state blends and one-shot montages;
- wing fold/spread and leg tuck additives;
- mesh/body pitch and bank additives;
- head/neck fire aim;
- tail and wing-tip lag;
- neck/head stabilisation;
- ground foot IK and pelvis height;
- speed/state-driven flap rate and amplitude.

This is the layer that turns authored poses into a responsive creature. The
33-bone load is small. The main risk is unreviewable binary graph sprawl;
**adopted:** keep the graph thin and put decisions in the C++ AnimInstance
parent now. Add Control Rig selectively later for tail lag and foot IK when
captures show a concrete placement/secondary-motion problem.

## Recommended runtime architecture

Replace single-node playback with `ABP_Dragon` parented to a C++
`UDragonAnimInstance`.

The C++ AnimInstance reads movement/combat facts and publishes variables:
ground speed, airspeed, vertical speed, flying/grounded, flight band, flap
phase/rate/amplitude, fold alpha, tuck alpha, aim pitch/yaw, bank, and landing
or hit triggers. The graph consumes those values; it does not invent gameplay
rules.

### Locomotion states

- Ground: Idle, Walk/Trot blend, turn/settle additives.
- Takeoff: directed transition into Air, driven by the flight model's Ascent.
- Air: Fly, true Glide hold, Hover, Dive/swept posture, precision-low-speed
  posture from the flight model.
- Land: flare/touch/settle, then Ground.
- Forced descent: exhausted glide posture until touchdown.

Use 0.2-0.35 second starting blends for major transitions, then tune from
captures. Replace the current overloaded word "Glide": animation, stamina,
and movement must name the same physical state after T-075.

### Montage slots

- Full body: tail slam, wing buffet, heavy hit, defeat, hard landing.
- Head/neck/upper body: roar, bite/eat, light hit.
- Looping action layer: fire-breath pose while firing.

AnimNotifies should mark gameplay-owned events: roar effect point, slam/buffet
damage windows, eat consume point, foot plants and wingbeats for audio. Rules
remain in combat/movement components; notifies only synchronise presentation.

### Always-on procedural layers

- FoldAlpha: folded wings on ground to spread wings in air.
- TuckAlpha: legs folded in flight, deployed for hover/landing.
- Bank/pitch additives from velocity, yaw rate, and flight-path angle.
- Aim offset for head/neck so Niagara fire can originate from a head socket.
- Tail spring/lag, wing-tip follow-through, chest/neck counter-motion.
- Ground foot IK and pelvis adjustment.

### Wingbeat curves

Do not hard-code "dive = flap faster." **Adopted dive posture:** wings partly
swept/tucked with little or no flap. Under the adopted momentum model:

- climb/hover: highest amplitude; rate follows climb demand;
- cruise: moderate rate and amplitude;
- fast level flight: interpolate towards a held glide;
- dive: wings partly swept/tucked, little or no flap;
- precision low speed: deep, readable beats tied to rise/sink commands.

Space/Ascent should synchronise to a downstroke where practical. Final curves
wait for the T-075 movement pick.

## Clip inventory and production order

Estimates include authoring, import, and wiring, but exclude the one-time ABP
build. They are session estimates, not measured throughput.

### Adopted v1 — needed for the game to read correctly

1. **Ground idle** — breath, weight, tail/head life. Programmatic. 0.5.
2. **Walk** — exists. Preserve/regenerate from a documented source. 0-0.5.
3. **Fly v2** — true spread, leg tuck, streamlined body. Programmatic. 0.5-1.
4. **Glide hold** — spread wings with small corrections, not slow flap.
   Programmatic. 0.5.
5. **Hover/precision flight** — steeper body, deep beats, legs ready.
   Programmatic base plus hand polish. 0.5-1.
6. **Takeoff** — crouch, leap, first downstroke. Hand posed. 1.
7. **Land** — flare, touch, settle; hard variant reuses timing/notifies.
   Hand posed. 1.
8. **Roar** — chest expansion, neck/head, wing mantle. Hand posed. 1.
9. **Fire-breath pose + aim offset** — procedural aim plus authored pose.
   0.5-1. Must pair with T-051's Niagara stream and a head/mouth socket.
10. **Hit light/heavy** — hand posed. ~0.5 each; triggers depend on T-074.
11. **Defeat/collapse** — hand posed. 1; presentation depends on T-074/T-072.
12. **Sleep/Wake set** — Sleep Idle, Wake, and Lie Down as reusable game
    animations. Hand posed from shared key poses. 1-1.5. This answers the
    owner's "what is this asking?" plainly: the game opens with the dragon
    waking on its hoard, so we invest in a real wake-up—but the same clips are
    reused by T-084 sleep instead of building a one-off cinematic.
13. **Enemy/livestock readability** — thief fall/hit and wired goat skeletal
    locomotion/death under T-054. About 1 session each family.

### Combat expansion

- Wing buffet: hand posed, 1-1.5 sessions.
- Tail slam: hand posed, 1-1.5 sessions.
- Claw swipe: hand posed, 1 session.
- Bite/eat loop: mixed programmatic/hand polish, 0.5-1.
- Crush: reuse hard-land with a damage notify, ~0.25 once Land exists.

**Adopted:** build these as full gameplay verbs in the animation pass, not
presentation-only stubs. Montage notifies must wire to real damage/status
rules through the combat seams (claw's rules remain T-076; wing buffet/tail
slam rules ride with T-068; crush remains T-077). Damage arcs, radii,
durations, stamina, counters, and notifies need GAMEPLAY numbers before final
timing, so locomotion/architecture lands first and combat timing locks after
those numbers exist.

### Later life and locomotion depth

- Trot/gallop and start/stop/turn, preferably retargeted if T-059 finds a
  clean source.
- Additional sleep/wake variants beyond the adopted reusable v1 set.
- Treasure carry pose for T-070.
- Perch/sit, shake, drink, scratch, and other idle life.
- Swim/wade only after T-091 decides dragon-in-water behaviour.

The AnimBP (on its C++ AnimInstance parent), montage slots, fold/tuck additives, aim, and tail
secondary work is estimated at 2-3 sessions. The dragon v1 set plus that
architecture is roughly 7-10 sessions. Combat expansion adds roughly 5-7.

## Visual acceptance gates

AAA feel cannot be proven by bone-track tests alone. Every hero pose/clip
needs rendered evidence from the actual gameplay cameras:

1. Blender contact sheet: front, side, rear chase, and top views at pose
   extremes; check wing span, membrane, leg tuck, neck/head line, and tail.
2. **Adopted review economy:** batch key poses by clip family into one
   contact-sheet owner gate before in-betweening, then one final Unreal
   capture gate per family. The agent iterates internally against renders and
   numeric track tests between those two owner touchpoints. This keeps two
   real quality gates without spending owner/token budget on two separate
   review rounds for every individual clip.
3. Unreal capture in lair and village light, at default and close cameras.
4. State-transition capture: ground -> takeoff -> fly -> glide -> brake ->
   hover -> land, with no pops, stale rates, or bind-pose frames.
5. Wingbeat/velocity relationship captured across climb, cruise, glide,
   dive, and precision flight.
6. Fire pose checked against the Niagara stream origin and aim direction.
7. Automation proves state selection, clip presence, bone spread/tuck tracks,
   notifies, and montage routing; captures prove the look.

## Death presentation decision

Adopted because one method does not fit every actor:

- **Dragon defeat:** authored collapse/slink-away only. The player character
  keeps camera and narrative control; GAMEPLAY already says the dragon does
  not die on loss. No dragon ragdoll.
- **Thieves:** authored hit/death fall first, then ragdoll settle once the body
  is effectively down. This gives a readable kill read plus natural final
  resting poses on slopes. It requires maintained PhysicsAssets per thief
  skeleton family and a capture check for floor penetration/flailing.
- **Goats:** authored side-fall, no ragdoll in v1. A goat corpse is an eat
  target; it must stay where the player killed it instead of rolling away or
  settling somewhere the eat interaction cannot find it.

## Rig investment ladder

Do these in order; stop at the first level that passes rendered gates:

1. Pose within the proven skin envelope.
2. Focused weight-paint correction around wing shoulder/armpit and hips.
3. Corrective morph/shape driven by shoulder angle if membrane volume still
   collapses.
4. Add membrane/finger detail bones only if fan/camber is visibly required.
5. Re-skin or re-bind in a spread pose only after renders prove the earlier
   levels cannot pass. This is the highest-risk option because it can trade
   flight quality for ground-fold damage and forces revalidation of Walk.

## Sequence

1. Run the three-pose T-058 diagnosis and fix Fly/tuck.
2. Keep T-059 report-only, CC0-only, and judge it mainly for locomotion value.
3. Build the C++ AnimInstance parent + lean AnimBP graph + montage/additive skeleton.
4. Author the adopted v1 loops/transitions and Sleep/Wake set, using the
   batched contact-sheet gate.
5. Add aim and bank/pitch layers against the adopted T-075 movement bands;
   add Control Rig tail/foot work only where captures prove the need.
6. Add combat/hit/defeat montages as full verbs when their gameplay events and
   numbers land.

## Decision record

Owner annotations were incorporated on 2026-10-31. Their substance is:

1. **V1 scope:** no owner objection recorded. Adopt the v1 list above as
   T-068's scope, including hover/takeoff/land. Fly v2 remains T-058 and
   enemy/goat work remains T-054 to avoid hiding two tasks inside one.
2. **Combat verbs:** no owner objection recorded. Build wing buffet, tail
   slam, claw, bite/eat, and crush as full verbs with montage notifies wired
   to rules, not silent presentation stubs.
3. **Architecture:** no owner objection recorded. Lean C++ AnimInstance parent
   + thin AnimBP now; selective Control Rig second.
4. **Sleep/wake:** owner did not understand the question. It meant: should we
   spend real animation effort on the dragon waking at the start, or fake it
   with an idle pose and camera move? Adopted answer: spend it, but make the
   clips reusable by the sleep system rather than a one-off cinematic.
5. **Dive posture:** swept/tucked wings with minimal flap.
6. **External animation licence bar:** CC0 only; otherwise make our own.
7. **Membrane crumple:** approve focused weight-paint correction if the
   three-pose render proves collapse; do not accept a visibly smaller spread
   merely to avoid that pass.
8. **Death presentation:** family-specific choice above — dragon authored,
   thieves authored-to-ragdoll, goats authored.
9. **Review budget:** cheaper without a quality cut = batched gates. One
   contact-sheet gate per clip family before in-betweening, agent iteration
   against renders/tests in between, and one final Unreal capture gate.

What remains empirical rather than preferential: whether the current skin
passes full spread, how far programmatic loops can go before hand polish is
needed, whether any CC0 locomotion source is worth retargeting, and the final
wingbeat curves once flight exists in PIE.

## Unverified register

- Current Fly/Glide look and transition pops in PIE.
- Whether insufficient unfold or membrane crumple dominates T-058.
- Walk action regeneration source.
- Exact same-clip Fly/Glide phase behaviour under UE5.8 single-node playback.
- Any future Meshy quadruped animation support beyond the current tool specs.
