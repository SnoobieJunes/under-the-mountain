# [2026-10-31 T-075] Flight-controls spike

Status: CLOSED 2026-11-03 (T-075): prototype flown and measured on all three
schemes in the live Lair (scripted course, per-tick CSV at
docs/course-evidence/scheme{1,2,3}.csv); PROVISIONAL production pick is
Scheme C for T-062 (owner feel-check there may still veto). Decisions below
stand as amended by the two 2026-11-03 goose-deuce entries (impulse latch,
additive blend) and the course-results section at the end of this file.
Explicit owner answers are preserved; every "idk" or blank was decided
by the agent under the owner's best-judgement instruction.

## Working decision for the prototype

Build Scheme C — pitch/momentum flight at speed, fading to precision thrust
and hover authority at low airspeed — on a custom dragon movement mode.
Prototype it A-first, then enable C's low-speed band, with B available by
disabling momentum and forcing precision authority on the same chassis.

This is now the selected prototype architecture, not a production claim. The
chimney, cave, and goat-stop legs below can still falsify the blend or the
starting tuning values.

## Decisions adopted

| Decision | Adopted prototype rule | Source |
|---|---|---|
| Stamina display/costs | One continuous 0–100 pool, HUD-segmented into five 20-point reserves. Ascent and Surge each require and spend one full 20-point reserve; continuous costs can spend fractions. [2026-11-04 T-124: starting values, since retuned to ten 10-point reserves + hover 3/s — docs/GAMEPLAY.md §2 is normative.] | Owner: "idk whats better" → agent decision: preserves one survival economy while making impulse costs readable. |
| Fast-flight/skim stamina regen | **None.** Speed never creates stamina. Skimming only gets the physical benefit of ground effect: less induced drag/sink and better speed retention. **[2026-11-04 FLIGHT-VIGOR, owner override: SUPERSEDED — fast flight (≥1800 cm/s) now refills +3/s; skimming alone still creates nothing, it only preserves the speed the vigor band pays for. GAMEPLAY.md §2 is normative.]** | Owner questioned the premise; agent rejected WoW-style resource creation for this game's food economy (explained below). |
| Dive | Dive is nose-down posture, not a powered button. Left Shift becomes Brake. | Owner: "idk whats better UX" → agent decision. |
| Precision authority | Automatic by airspeed, with Brake as an explicit override into the precision band. Starting bands: precision ≤650 cm/s, blend 650–1600, momentum >1600. | Agent decision under delegation. |
| Gamepad | Follow the flight-game trigger convention: RT Surge, LT Brake/precision; fire moves to RB. Full starting map below. | Owner: "what do other games do, pick for now" → agent decision. |
| F / landing | F stops being a magic flight toggle and becomes Landing Assist near the ground; gamepad top face is the equivalent. | Owner asked for a close-to-ground slow/land button. |
| Interior speed | No invisible interior speed cap. T-066's great chamber is sized from measured turning diameter; narrow caves make precision flight the natural choice. | Owner: chamber needs changing → agent geometry rule. |
| Pitch/camera defaults | Mouse/stick up = nose up by default; separate invert settings for mouse flight pitch, gamepad flight pitch, and camera pitch. At stall, the camera stays readable/level-damped while the mesh nose sinks. | Owner: make it a setting / good UX → agent defaults. |
| Top speeds | Prototype starts at 2400 cm/s level cruise terminal, 3200 Surge ceiling, and 4000 steep-dive terminal; test 3200/4000/4800 dive values on the course. | Owner: decide by rigorous standards → agent starting values + falsification method below. |
| Zero stamina | Unpowered glide/descent and landing always remain available; only draining actions are blocked or forced out. Settings gets **Testing → Infinite Stamina (God Mode)**, default off. | Owner explicit. |

## What ships today

`ADragonCharacter` uses stock `UCharacterMovementComponent` `MOVE_Flying`.
Only three movement values are tuned: `MaxFlySpeed=1800`,
`MaxWalkSpeed=600`, and `BrakingDecelerationFlying=300` in
`Source/UnderTheMountain/Private/Dragon/DragonCharacter.cpp:136`.
There is no custom movement mode and no flight component.

The input model is:

- WASD/stick calls `AddMovementInput` on a yaw-only basis at
  `DragonCharacter.cpp:385` (`HandleMove`). Looking up or down does not move
  the dragon up or down.
- Space hard-sets vertical velocity to 650 cm/s at `DragonCharacter.cpp:404`
  (`HandleFlap`). It is the only climb control, so sustained flight is
  repeated hopping.
- Left Shift adds acceleration along camera forward in the Tick dive path at
  `DragonCharacter.cpp:801`.
  It can accelerate upward; it is a camera-forward rocket, not a gravity dive.
- F directly toggles walking/flying with no takeoff or landing transition.
- The mesh is fixed at yaw -90 degrees on an upright capsule. No code writes
  flight pitch or bank to the mesh.
- The only camera is a 700 cm spring arm attached to the capsule.

There is no dedicated sink, brake, throttle, or hover-hold verb.

## Root causes

1. **Look direction and travel direction disagree vertically.** This is the
   direct chimney failure: aiming up the shaft produces no climb.
2. **Lift is a repeated velocity overwrite, not continuous thrust.** Each flap
   decays, so altitude costs repeated Space presses plus the 8/s low-horizontal-
   speed hover drain.
3. **All horizontal directions are equally fast.** The dragon can strafe or
   reverse like a drone, so turning, banking, and planning a flight path have
   little purpose outdoors.
4. **Velocity has no visible body language.** The capsule stays upright, the
   mesh does not pitch into a dive or bank into a turn, and no speed-FOV cue
   exists.
5. **The current dive contradicts GAMEPLAY.** GAMEPLAY says dive converts
   altitude to speed, but the implementation injects free energy along the
   camera vector.
6. **The camera fights enclosed spaces.** A 700 cm boom is wider than half the
   reported 7 m shaft. Geometry is not the primary fault; controls and camera
   are.

## Reference model: Dragonflight dynamic flight

The transferable mechanics are structural, not cosmetic:

- The nose defines the flight path.
- Pitching down turns altitude into airspeed.
- Pitching up spends airspeed to climb.
- Discrete impulses add energy: Surge Forward for forward speed and Skyward
  Ascent for an upward launch/takeoff.
- Vigor is spent in whole charges. Blizzard describes ground recharge and
  recharge at high speed in its
  [Dragonriding overview](https://worldofwarcraft.blizzard.com:443/news/23818251/?utm_source=mobileweb-news&utm_campaign=web-eu-mobilewebnews&utm_medium=internal&utm_content=-1).
  Secondary guides corroborate the original one-charge-per-30-seconds grounded
  and one-per-15-seconds at high speed values; exact live-game tuning is not a
  design dependency here.
- In WoW, fast flight and (with the relevant talent) near-ground skimming can
  replenish Vigor, rewarding a dive/skim/climb cycle. **We do not adopt that
  resource rule:** WoW Vigor is an arcade flight-permission token, while our
  stamina is the dragon's bodily energy inside a food-survival loop. Speed
  creating calories would make skilled outdoor flight partly self-sustaining
  and weaken eating as a reason to take risks. We adopt the real physical
  part instead: within roughly a wingspan of a surface, ground effect reduces
  induced drag and sink, so a skim preserves speed and costs fewer future
  impulses — it never adds stamina.
  **[2026-11-04 FLIGHT-VIGOR, owner override: this rejection is
  superseded. The shipped economy now adopts the dive/fast-flight
  refill (+3/s at ≥1800 cm/s) as the continuous-flight skill loop;
  the skim half of the rejection stands (ground effect alone still
  creates nothing). GAMEPLAY.md §2 is normative.]**
- A dependable brake makes very high speed safe enough to use near terrain.

Our design already describes this skill loop. `docs/GAMEPLAY.md:116`
calls dive-for-speed and glide-to-travel the flight skill loop. Adopting a
momentum model fulfils that text more literally than the current code does.

## Candidate A — pure pitch/momentum flight

Model airspeed as an energy state directed by the dragon's nose. Gravity
projects onto the flight path: nose-down accelerates, nose-up decelerates,
and drag creates pitch-dependent terminal speeds. Turning is yaw-rate limited;
the velocity vector follows the nose with controllable lag. Below stall speed,
the dragon sinks and loses turn authority.

Controls:

- Mouse/left stick: nose pitch and turn; right stick is camera on gamepad.
- Space/bottom face: Skyward Ascent equivalent — takeoff or up-forward impulse.
- Surge input: forward impulse and a temporary higher speed ceiling.
- Left Shift/LT: hard aerodynamic brake towards controlled glide.
- Dive is a posture (nose down), not a powered button.
- RMB hold sails: mouse steers the nose rate-limited; release keeps the
  nose. Double-click RMB latches steer-to-look; any movement input cancels
  mouse-steering. **[2026-11-04 FLIGHT-VIGOR: replaces the free-look
  decoupling below — RMB no longer freezes the nose to roam the camera.]**

Strengths: best open-world sensation, natural speed hierarchy, bank/pitch are
trivially derived from one velocity vector, and the wide T-049 world gains a
real travel skill.

Weaknesses: poor authority for chimney, caves, hoard landings, eating stops,
and backing away from a wall. It also needs a custom movement integrator;
fighting `PhysFlying` from pawn Tick is how the present dive became a rocket.

Estimated prototype cost: about 3 sessions, plus 1 to productionise. Estimate
only; no prototype has been built.

## Candidate B — 3-axis thrust with hover-hold

Retain `MOVE_Flying`, make movement camera-relative in full pitch, add held
rise/sink, and damp to a stable hover when commanded. This is the literal
reading of "move in all 3 axes and hold."

Strengths: simplest and safest implementation; chimney and precision landings
are straightforward; RMB free-look and double-click-to-move are trivial.

Weaknesses: velocity is commanded rather than earned. There is no reason to
dive for speed, trade altitude, skim terrain, or plan an energy line. At a flat
1800 cm/s cap, a minutes-wide world becomes a commute. Hover also becomes the
resting state GAMEPLAY prices at -8/s specifically to discourage.
[2026-11-04 T-124: -8/s was the then-current §2 hover price; T-124
retuned hover to -3/s — §2 is normative.]

Estimated cost: 1-2 sessions. This is the comfort option, not the AAA-feel
option.

## Candidate C — speed-blended hybrid (recommended)

Use A as the high-speed model and fade in B's authority at low airspeed. The
blend is by airspeed, not by indoor/outdoor trigger volumes, so one consistent
system works in the world and inside T-066's mountain.

- High band: nose-steered momentum, pitch/altitude exchange, Surge, Ascent,
  brake, stall sink, speed-FOV.
- Low band: strafe, direct rise/sink, hover-hold, fine position correction.
- Transition band: authority and drag interpolate continuously; HUD/debug
  output exposes the active band during tuning.
- Brake takes the dragon down through the blend into a controllable stop and,
  while held, explicitly forces precision authority even before airspeed has
  fully decayed.
- At low speed, Space-hold can mean rise; at speed, Space remains Ascent. That
  contextual ambiguity must be tested explicitly.
- RMB hold sails (mouse steers the nose, rate-limited; release keeps the
  nose) **[2026-11-04 FLIGHT-VIGOR — was: free-look remains camera-only]**.
  Double-click RMB means "go where I am
  looking": re-aim the nose in the high band, latch movement towards the view
  in the low band. Any movement key cancels, per the owner's clarification.
- Camera arm shortens and pitch decouples in the precision band, allowing the
  player to look down at a goat, treasure, or landing point while hovering.
- **Landing Assist (F / gamepad top face)** replaces the old flight toggle.
  Near the ground it arms a landing: brake into precision, flare, cap descent
  and horizontal drift, then settle only if speed/angle are inside the safe
  envelope. If the approach is too fast or steep it brakes and reports the
  unsafe approach rather than teleporting the dragon into a safe landing.

Why C wins for this game: pillars 1 and 2 happen through patrol, stops,
landings, and enclosed spaces. Pure momentum cannot serve those interactions.
Pure thrust cannot make the open world feel like dragon flight. C keeps the
skill outdoors and the control indoors.

Primary risk: an uncanny blend where strafe or turn authority appears to
switch off at an invisible threshold. The blend therefore needs a broad fade,
continuous authority curves, and a visible tuning readout.

Estimated prototype cost: about 4 sessions, plus 1-2 to productionise.

## Recommended implementation shape

Create `UDragonMovementComponent : UCharacterMovementComponent` and a custom
movement mode whose `PhysCustom` path owns flight integration. Keep CMC for
capsule collision, floor detection, walking, falling, and `Landed()`.

Keep the following as pure, testable functions over a parameter struct:

- pitch/gravity/drag energy integration;
- high/low authority blend weight;
- Surge and Ascent impulses;
- brake and stall behaviour;
- landing severity from speed **and impact angle**;
- mesh pitch from flight-path angle and bank from yaw rate and airspeed.

The pawn should stop integrating flight in Tick. Stats remains the stamina
writer. Trap slow must scale the movement parameter struct rather than only
`MaxFlySpeed`.

Prototype all comparisons on one chassis behind `utm.flight.scheme`:
current, A, C, and B approximated by disabling momentum while forcing the
low-speed precision authority at all speeds. Add airspeed,
flight-path angle, authority band, charges/stamina, and per-run CSV capture.

## Adopted prototype inputs and starting values

### Keyboard/mouse starting map

| Input | Momentum band | Precision band / ground |
|---|---|---|
| Mouse | Aim nose (pitch + turn) | Camera; movement remains camera-relative |
| RMB hold | Sail: mouse steers nose, rate-limited (release keeps nose) [2026-11-04 FLIGHT-VIGOR, was free look] | Same sailing steer |
| Double-click RMB | Latch steer-to-look | Latch move-to-look; WASD cancels |
| W | Tap = Surge | Hold = move forward |
| S | Flare / bleed speed | Move backward |
| A/D | Bank/turn | Strafe |
| Space | Ascent impulse | Takeoff; hold to rise |
| Ctrl | Flare assist | Hold to sink |
| Left Shift (hold) | Brake | Brake; forces precision authority |
| F | Landing Assist near ground | Tail Slam once grounded |
| LMB / R / E | Fire / Roar / reserved air interact | Fire / Roar / Eat-context |
| Q / X | Claw / Wing Buffet | Claw / Wing Buffet |

C remains the tint key. Ctrl is the only keyboard sink key in the prototype;
do not silently dual-bind C.

### Gamepad starting map

Flight games commonly put acceleration/deceleration on the analog triggers
(Ace Combat is the clearest reference) and keep both thumbs on sticks. We use
that grammar, with dragon verbs on shoulders/face buttons:

| Input | Airborne | Grounded / precision note |
|---|---|---|
| Left stick | Pitch + turn at speed | Move/strafe in precision band; walk on ground |
| Right stick | Camera / free look | Camera / free look |
| RT | Surge Forward | Surge only once airborne |
| LT (hold) | Brake; forces precision authority | Brake/settle |
| RB (hold) | Fire breath | Fire breath |
| LB | Roar | Roar |
| LB+RB | Wing Buffet | Wing Buffet |
| Bottom face | Ascent; hold to rise in precision | Takeoff Ascent |
| Right face | Tap = Claw; hold = sink in precision | Tap = Claw; hold = Eat/context-interact |
| Top face | Landing Assist | Tail Slam |
| Left face | Seeing Stone true-sight toggle | Same |
| D-pad Up | Steer-to-camera (T-063 equivalent) | Cancel with left-stick movement |
| D-pad Down | Scent show/hide (T-061) | Same |
| R3 | Cycle camera (T-065) | Cycle camera |
| L3 | Reserved for smart lock-on (T-069) | Reserved |

Fire moves from LT to RB because LT is now the analog brake. This is a
prototype layout to validate on the course, not a claim that one layout fits
all pads; remapping remains settings work.

### Speed and band starting values

These are starting points for measurement, tagged in GAMEPLAY as
`[tuning: PT-STAM]`, not proven feel values:

- precision authority full at ≤650 cm/s (6.5 m/s);
- authority blend fades from 650 to 1600 cm/s;
- level-flight terminal speed 2400 cm/s;
- Surge temporarily raises the ceiling to 3200 cm/s;
- steep-dive terminal starts at 4000 cm/s (40 m/s), with 3200/4000/4800 tested;
- starting yaw-rate targets: 90°/s in precision, fading to 55°/s at dive speed.

Why 4000 as the starting dive value: at 40 m/s the dragon moves about
0.67 m per 60 fps frame, crosses a 3 km world in roughly 75 seconds at full
dive speed, and—with a 55°/s yaw-rate limit—needs an about 42 m turn radius
(83 m diameter). Those three facts make it fast enough to shrink a sprawling
world, still spatially readable, and directly useful to T-066: the great
chamber's clear flight volume should start around 100 m across if unrestricted
momentum turns are welcome there. If the course shows unreadable terrain,
excess contacts, or camera sickness, 3200 wins; if 4000 still feels slow at
world scale and passes contacts, test 4800.

## Stamina consequences

A momentum model changes the economy even if all food numbers remain.

Adopted mapping:

- Treat stamina as the existing 0-100 pool, displayed in five 20-point
  segments [2026-11-04 T-124: starting values, since retuned to ten
  10-point segments — docs/GAMEPLAY.md §2 is normative]. Fire, roar, and other continuous costs continue drawing fractions
  from the same pool; Ascent and Surge require and spend a full 20-point
  segment so their cost reads at a glance.
- Unpowered glide and descent cost nothing **[2026-11-04 FLIGHT-VIGOR:
  at cruise-band speeds; slow flight now pays −1/s and hovering −3/s —
  GAMEPLAY.md §2's signed band table is normative]**.
- Hover and sustained low-band climb continue to drain; hover remains the
  loiter tax.
- **No high-speed or skim stamina regeneration.** Ground effect only reduces
  induced drag/sink and preserves speed. Stamina returns by grounded regen,
  eating, and other explicit food/economy rules—not by going fast.
  **[2026-11-04 FLIGHT-VIGOR, owner override: SUPERSEDED for high speed —
  ≥1800 cm/s refills +3/s (the dive is the refill verb; ~10 min of
  continuous flight is the skill loop). Skimming still creates nothing
  by itself. Zero-stamina force-fall/HP burn applies only while the
  signed rate drains (< 0).]**
- At zero stamina, the dragon can still glide, descend, brake, use Landing
  Assist, and land. Only draining actions are blocked or forced out; unpowered
  flight never burns HP by itself.
- **Testing → Infinite Stamina (God Mode)** ships in Settings, default off. It
  disables stamina costs/drains and exhaustion HP burn only; it does not make
  HP invulnerable. The HUD must show a testing badge while it is on so playtest
  captures never silently lie about the economy.

GAMEPLAY §2 now carries these as the target model with prototype tuning tags.
Its "one exterior livestock run refills at least half a pool" and lair-camping
checks still must be retested with the prototype's measured circuit costs.

Landing damage must use descent angle as well as speed: a fast shallow flare
should not be punished like a vertical lawn-dart.

## Prototype course — the decision gate

The owner flies every leg on keyboard/mouse and gamepad. The current scheme is
the baseline. Record CSV, input counts, and a one-line owner verdict per leg.

1. **Chimney:** hoard room to peak, return, stop on the hoard. Record time,
   wall contacts, Space presses, stamina, and HP. Target: no more than three
   climb impulses and zero damage on a competent run.
2. **Slalom:** five gates around pillars/houses. Record time, contacts, and
   whether bank/pitch reads without explanation.
3. **Village circuit:** peak to village, rooftop/terrain skim, climb home.
   Record lap time, net stamina, minimum altitude, and whether the skilled
   momentum/skim line costs materially fewer impulses than a lazy climb/hover
   line. Stamina must never increase from speed or skimming; the reward is
   preserved speed and reduced sink, paid for by public exposure.
   **[2026-11-04 FLIGHT-VIGOR, owner override: course criterion updated —
   stamina now DOES increase at ≥1800 cm/s (+3/s); judge the circuit on
   whether the dive-to-refill loop, not impulse count alone, carries it.]**
4. **Dive-bomb and eat:** cruise, dive at a goat, brake into eat range, eat.
   Record overshoots, stop attempts, and landing HP. Pure A is expected to be
   weakest here.
5. **Cave precision:** enter a side opening, turn around, stop at a specified
   treasure, back out. This is C's falsifier.
6. **Zero-stamina recovery:** start empty outdoors and indoors. No soft-lock;
   record whether recovery feels earned or like punishment.
7. **Five minutes free flight:** no objective. Record which scheme the owner
   continues using unprompted and why.

Decision rule: C must retain A's circuit feel while passing chimney, goat,
and cave tests. If A alone passes precision tests acceptably, simplify to A.
If C's blend is perceptible as a mode switch after two tuning passes, C fails
even if its metrics look good.

## Decision record

Owner annotations were incorporated on 2026-10-31. Their wording is not
preserved as design text; its substance is:

- "idk" on charge presentation → one continuous pool with 20-point HUD
  segments and 20-point impulse costs [2026-11-04 T-124: starting
  values, since retuned to ten 10-point — docs/GAMEPLAY.md §2].
- "That doesn't make sense. why?" on speed/skim regen → explained as WoW's
  arcade Vigor abstraction, then rejected for stamina; only physical ground
  effect (less drag/sink) carries over.
- "idk whats better UX" on dive → posture wins; Shift becomes Brake.
- Gamepad delegated → RT/LT become Surge/Brake following flight-game trigger
  convention; fire moves to RB.
- F concern about ground crashes → F becomes contextual Landing Assist, not a
  magic walking/flying toggle.
- Chamber uncertainty → do not hide an interior cap; size T-066 from the
  measured turning diameter, starting from the 100 m clear-volume envelope.
- Pitch uncertainty → nose-up default with separate mouse/gamepad/camera
  invert settings; stall camera stays level-damped.
- Top speed delegated to rigorous standards → 4000 cm/s starting dive terminal
  with 3200/4800 course falsifiers and the turn-radius calculation above.
- Zero-stamina mercy confirmed, plus Settings → Testing → Infinite Stamina
  (God Mode), default off and visibly badged.

What remains genuinely undecided is empirical, not preferential: blend width,
terminal speeds, yaw rates, Landing Assist envelope, impulse costs, and whether
Scheme C's blend can be made invisible. The course decides those.

## Next build step

Implement the A chassis plus C blend behind `utm.flight.scheme` on a prototype
branch, using the adopted inputs and starting values above. Add the pure-math
tests and CSV instrumentation with it. Do not productionise animation rates,
final GAMEPLAY numbers, or T-079 collision damage until the owner has flown the
course and the winning values are recorded on T-075.

## Course results — T-075 close, 2026-11-03 (all provisional for T-062)

Chassis: `UDragonMovementComponent`/`PhysCustom` behind `utm.flight.scheme`
(0 shipped inert / 1 A / 2 C / 3 B). Runner: `Scripts/fly-course.sh [scheme]`
(`utm.flight.course` scripted legs: liftoff, climb, cruise, 360 turn, brake,
hover, assisted landing; per-tick CSV + `COURSE` stdout lines; raw evidence at
`docs/course-evidence/scheme{1,2,3}.csv`). Integrator verified exact to
0.44 cm/s by full-step simulation against flown CSV (an apparent energy leak
was analyst error: single-tick deltas misread against the documented
first-order steer lag).

| Leg | A (momentum) | C (hybrid, amended) | B (forced precision) |
|---|---|---|---|
| Liftoff | 1021 cm | 1021 cm | 68 cm |
| Climb / 5 s | +1602–1827 cm (320–365 cm/s) | +1533 cm (307 cm/s) | +64 cm |
| Turn diameter | 8–16 m, entry-sensitive; max-rate spiral bounded by the 14° steer floor | 14 m controlled | 0 m |
| Brake from cruise | 15–51 cm | 247 cm in 0.8 s | 0 (never moving) |
| Hover drift / 5 s | sinks ~1000 cm (no hover by design) | 0–39 cm | 0 cm (perfect) |
| Assisted landing | clean, HP 100 (assist is precision equipment — caveat) | clean, HP 100 | timeout (cannot reach landing states) |

Amendments from measurement (goose-deuce.md 2026-11-03, both `[decision]`):
impulse authority latch 1.5 s (takeoffs evaporated at W 0.83); additive C
with hover capture (multiplicative blend dragged sub-1600 coasts to hover);
steer floor bounding lag to ~14° (killed the 1268→80 turn spiral). Starting
tuning values all held — no GAMEPLAY.md movement-number changes.

Engineering case for the unflown geometry legs (owner flies them in T-062):
307 cm/s climb vs the 40 m shaft ≈ 13 s; 39 cm hover fits the 7 m shaft;
14–16 m turns fit the 100 m crown envelope with margin (use 20 m+ margins —
turns are entry-sensitive). Zero-stamina mercy, dive-as-posture, and the
stamina bands ran honestly on every leg (per-tick stamina in the CSVs).

Known gaps T-062 inherits: pad path partial (D-pad Up latch only; RT-Surge /
LT-Brake remap deferred); RMB free-look + latch implemented, feel-check
pending; mild level-cruise decay ~33 cm/s unexplained but bounded; T-066 owes
the hoard a clear touchdown disc (course wedged twice on center furniture —
SeeingStone + chests — at z=438, velocity running, position frozen).
