# GAMEPLAY.md — Under the Mountain gameplay spec

**Date:** 2026-10-27 · **Task:** T-026 · **Status:** Draft for implementation — §2 flight model amended 2026-10-31 (T-075 prototype decisions)
**Normative plan:** [docs/PLAN.md](PLAN.md) · **Vision:** [idea.md](../idea.md) ·
**Art:** [docs/ART-DIRECTION.md](ART-DIRECTION.md) (Style B)
**Scope tags:** `M1` = hackathon build, `M2` = open-world expansion,
`M3` = escalation, `M4` = systems depth (per PLAN.md phases).

## How to read the numbers

Every gameplay number carries one basis tag. No untagged numbers ship.

| Tag | Meaning |
|---|---|
| `[derived: <reason>]` | Follows from a stated constraint (physics, readability, plan). Changing it means changing the constraint. |
| `[tuning: <test>]` | Placeholder value. The named test is the authority that replaces it; until that test runs, the value is unproven. |

Tuning tests named in this doc:

| ID | Test | Pass / output |
|---|---|---|
| `PT-LOOP` | Timed PIE playtest, fresh player, full session, think-aloud off; log timestamps of first flight / first catch / first eat. This is a **benchmark route for tuning**, never a required player path — the player may ignore it entirely (sandbox rule, §1) | Median timings replace §1 cadences |
| `PT-STAM` | PIE stamina probe: scripted walk/flap/dive circuits + free play; log time-to-empty and deaths at zero stamina | Drain/restore values in §2 |
| `PT-SCENT` | Scent readability probe in lair at gameplay camera distance; player ranks ribbons by value, timed | Thickness brackets in §3 (target: correct rank order without UI) |
| `PT-WAVE` | Wave survival playtest per wave × Notoriety star; log hoard lost, dragon HP, clear time | §4 composition/gear, §8 thresholds |
| `PT-THIEF` | Single-thief arena encounters (one type at a time), counter attempted per tell; log catch/escape rate | §5 speeds, grabs, counters |
| `PT-INTRO` | Cold-start playtest, no instructions given; log whether choice + stamina + scent are understood unaided | §7 beat timings/prompts |
| `AT-*` | Unreal Automation test under `Source/UnderTheMountain/Tests/` (hermetic, injected subsystems per AGENTS.md) | Invariants (no-distance rule, lose conditions, decay math) |

**DataAsset convention:** all values below live in `Content/Gameplay/` as
Primary DataAssets (`DA_*`) or DataTables (`DT_*`), read through the
subsystems in PLAN.md. No value in this doc may be hard-coded in C++.

| Section | DataAsset(s) | Owning system (PLAN.md) |
|---|---|---|
| §1 Loop timings | `DA_SessionLoop`, `DT_IncursionCadence` | `UNotorietySubsystem`, wave spawner |
| §2 Stamina/food | `DA_DragonStats`, `DT_FoodValues` | `UDragonStatsComponent`, `UDragonMovementComponent` (T-075 target) |
| §3 Scent | `DA_ScentConfig`, `DT_ScentBrackets` | `UScentComponent`, `UTreasureSubsystem` |
| §4 Waves | `DT_Waves`, `DT_NotorietyGear` | `UNotorietySubsystem`, wave spawner |
| §5 Thieves | `DA_Thief_Burglar`, `DA_Thief_DwarfParty`, `DA_Thief_Adventurer`, `DT_Counters` | `AThiefCharacter`, `UThiefBrain` |
| §6 Artifacts | `DA_Artifact_*` (one per row), `DT_ArtifactIndex` | `UDragonStatsComponent`, `UTreasureSubsystem` |
| §7 Intro | `DA_IntroBeats` (sequence + prompt table) | Intro level script, `UNotorietySubsystem` |
| §8 Win/lose | `DA_HoardThresholds` | `UHoardSubsystem` |
| §9 Sleep/tribute | `DA_SleepConfig`, `DT_Tribute` (`M2`) | `UNotorietySubsystem`, `UHoardSubsystem` |

---

## 1. Core loop timings — `DA_SessionLoop`, `DT_IncursionCadence` (`M1`)

### Sandbox guarantee (owner decision 2026-10-31)

This game is a **sandbox with systemic pressure, not a scripted session**.
The minute-by-minute table below is a benchmark cadence for tuning and demo
pacing — it is never a rail. After the intro doorway the player may ignore
incursions and waves, abandon the hoard, raid villages, chase wildlife, test
the flight model, sleep, or simply fly the world. Nothing teleports the
player back, locks a verb behind the next beat, or pauses the world until an
objective is obeyed. The simulation keeps running and answers in kind:
thieves keep looting an undefended hoard, public kills raise Notoriety,
witnessed livestock kills matter, waves arrive and resolve by their caps,
and win/lose is scored from world state rather than route compliance. Going
rogue is a supported play style with consequences, not a failure to follow
instructions. The hackathon video may therefore be unscripted live sandbox
play; no demo-only scripted route may be required to make the game respond.

Target benchmark session: one sitting a judge can finish start-to-end
`[derived: PLAN.md success criterion — fresh player finishes intro, flies,
defends/loses in one session]`. Working length `[tuning: PT-LOOP]`:

| Parameter | Value | Basis |
|---|---|---|
| Full session (intro → win/lose) | 18–25 min | `[tuning: PT-LOOP]` |
| Intro (§7) | 2–3 min | `[tuning: PT-INTRO]` |
| Free play between waves | 90–150 s | `[tuning: PT-LOOP]` — long enough for one exterior food run (§2) and return `[derived: flight zone round trip must fit inside the gap or eating is never safe]` |
| Waves per session | 6 | `[tuning: PT-WAVE]` — see §4; wave 6 is the final wave (§8) |
| Incursions between waves | 1–2 single thieves | `[tuning: PT-LOOP]` |

### Minute-by-minute benchmark loop `[tuning: PT-LOOP]` (all rows — cadence reference, not a required route)

| Clock | Beat | System state |
|---|---|---|
| 0:00–0:30 | Wake on hoard (§7 beats 1–2) | Scent on; no HUD numbers yet |
| 0:30–1:30 | Gnome steals, roar, choice offered (§7 beats 3–5) | Notoriety still 0★ |
| 1:30–3:00 | Choice resolves: let-go (stay in lair) or chase (first flight, first stamina drain, first eat prompt) | Notoriety per §7 branch table |
| 3:00–5:00 | First free patrol: follow scent, learn lair chambers; scripted burglar incursion #1 (single, slow tell) | Teaches patrol → catch inside lair = free kill |
| 5:00–6:00 | **Wave 1** (§4) | First scored defence |
| 6:00–8:00 | Recovery gap: eat (wildlife outside or caught thief), re-check hoard by scent | Stamina lesson lands here if skipped in intro |
| 8:00–9:30 | Incursion (type rotates per §4 cadence) + **Wave 2** | — |
| 9:30–12:00 | Gap → **Wave 3**; first dwarf-party traps appear | Counter variety (§5) fully in play |
| 12:00–15:00 | Gap (exterior raid becomes viable: livestock/village vista) → **Wave 4** | Notoriety choices now player-driven |
| 15:00–18:00 | Gap → **Wave 5**; gear scaling (§4) visible if Notoriety ≥2★ | — |
| 18:00–21:00 | Final gap (last eat/repair window, announced by scent surge §3) → **Wave 6 (final)** | Win/lose evaluated at wave end (§8) |
| 21:00–25:00 | Resolution + score (hoard retained, Notoriety, kills inside/outside) | Session end; `M2` continues into free roam instead `[derived: PLAN.md M2]` |

### Cadence rules

| Rule | Value | Basis |
|---|---|---|
| Incursion spawn interval (free play) | every 60–120 s, randomised in range | `[tuning: PT-LOOP]` |
| Max concurrent incursions (outside waves) | 2 | `[tuning: PT-LOOP]` `[derived: lair has 3 chambers + hoard room (PLAN.md M1); >2 simultaneous undetected thieves makes patrol information, not skill, the bottleneck]` |
| Wave warning lead time | 20 s (scent surge + audio sting, no text banner) | `[tuning: PT-LOOP]` |
| Wave duration cap | 4 min, then survivors flee | `[tuning: PT-WAVE]` `[derived: a wave that cannot end soft-locks the session loop]` |
| Thief entry points | peak entrance + crack(s) used in §7 | `[derived: PLAN.md M1 lair layout; intro must teach the real entry points]` |

Patrol is the default state: between beats the player walks/flies chamber to
chamber following scent (§3). Nothing in the loop ever reveals *where* a
thief is — only that treasure (possibly carried) exists. `[derived: PLAN.md
pillar 1]`

World clock (T-092 decision, 2026-11-04): one 24 h cycle is 600 real
seconds (25 s per game hour) in `UWorldClockSubsystem`/`FWorldClock`,
starting at dawn 06:00. Time-of-day is one queryable bucket — Night
20:00–05:00, Dawn 05:00–08:00, Day 08:00–17:00, Dusk 17:00–20:00 — that
cadence/sky/schedule code reads behind that seam only. Sleep (§9) jumps
the clock to the next dawn. `UnderTheMountain.World.Clock` pins the
phases, wrap and skip math.

---

## 2. Stamina / food economy — `DA_DragonStats`, `DT_FoodValues` (`M1`)

Resources: HP, magic, stamina `[derived: idea.md/PLAN.md pillar 3]`. Magic is
spent only by artifact/heal effects in `M1` (§6); its pool is specced here so
the HUD and DataAsset exist from day one.

| Pool | Max (L1) | Basis |
|---|---|---|
| HP | 100 | `[derived: percent-scale readability; all damage below is % of pool]` |
| Stamina | 100 | `[derived: same percent scale as HP so drain rates read as %/s]` — one continuous pool, HUD-segmented into ten 10-point reserves for flight impulses `[derived: T-075 decision record, docs/FLIGHT-SPIKE.md; retuned five×20 → ten×10 in T-124, 2026-11-04]` |
| Magic | 100 | `[derived: same percent scale; costs in §6]` |
| Magic regen (T-080 decision, 2026-11-04) | +0.5/s over time, paused while a magic drain is active; no food/hoard source in `M1`; sleep does not refill it | Base rate from §6's Magic Pendant line ("base regen 0.5/s"; the pendant's +1/s is artifact work, T-081). Time-only keeps the food loop about stamina/HP and makes true-sight's 3/s a real ~33 s budget from full |
| Intro "full power" preview (§7) | stats shown at max-artifact values, then reset to L1 on loss/first session end | `[derived: idea.md — start at full power as tutorial, lose down to L1]` — preview numbers are the §6 artifact sums, not a second stat block `[tuning: PT-INTRO]` |

### Flight model (target model — T-075 prototype decisions, 2026-10-31)

The flight model is a speed-blended hybrid with a fixed-wing core
(**[2026-11-04 FLIGHT-PHYSICS]**). At speed the dragon flies by lift
and momentum: lift `L = 980 × (v/1600)² cm/s²` (clamped at
2450 cm/s², only `cos(bank)` of it opposing gravity) holds level
cruise at 1400–1800 cm/s altitude-neutral with no thrust, pitching
down converts altitude to airspeed, pitching up spends airspeed to
climb, and **dive is a posture, not a powered button**. Below
600 cm/s lift cannot hold the dragon — slow momentum flight
genuinely stalls and sinks — but it also cannot turn: turn
authority fades to zero below 600 cm/s, so flight never yaws or
spins in place. Steering is a coordinated banked turn: the left
stick (X = bank to ±40° at 90°/s roll rate, Y = pitch command at
60°/s) commands bank/pitch, yaw follows from
`g·tan(bank)/max(v,600)` capped at the yaw-rate target, and
velocity weathervanes onto the nose. At low airspeed the model
fades to precision thrust: camera-relative translation, rise/sink,
strafe, and hover-hold for the lair, caves, landings, and eating.
Brake explicitly forces the precision band.
**[2026-11-04 PLANE-MODEL, owner override — supersedes the
coordinated-turn steering above:] the dragon now handles like a
plane.** The flight frame is a full quaternion (nose pitch + wing
roll, never clamped): left-stick Y pitches the nose and X rolls
the dragon as *rates* (pitch 75°/s, roll 120°/s at full
deflection, both scaled by turn authority so slow flight is
mushy, never spinny). Lift runs perpendicular to the wings, along
the dragon's own up: banked lift bends the velocity so heading
change *emerges* from the bank (the weathervane carries the nose
onto the velocity), and inverted lift pulls down — sustained
banked turns, ±60° climbs/dives, and a full loop-de-loop flown by
holding pitch are all reachable; there is no upright lock and no
bank/pitch clamp. A held pull raises angle of attack (lift up to
2× along the dragon's up; a push unloads it), which is what
generates the centripetal force a loop demands. The old
`g·tan(bank)/v` formula remains only as an analytic comparison in
tests, not a steering law. Right stick keeps camera orbit *and*
its X adds a small direct yaw to the dragon (30°/s full,
authority-scaled) — both owner readings at once. Precision/hover,
Landing Assist, walk and the vigor bands below are unchanged.
Proven in-engine by the scripted course (`Scripts/fly-course.sh`:
banked-360, loop, climb60/dive60 legs) and by the full-step sims
in `UnderTheMountain.Dragon.Movement`.
**[2026-11-04 FLIGHT-VIGOR, owner override — supersedes the previous
no-regen rule and the FLIGHT-SPIKE.md rejection it came from:] speed at
or above 1800 cm/s DOES regenerate stamina (+3/s, the vigor band
below)**; ground effect still only reduces induced drag/sink so a
skilful low line preserves speed and needs fewer paid impulses. The
flight skill loop is now dive-for-speed → fast line refills → spend on
Surge/Ascent/climb, priced so ~10 minutes of continuous flight is a
pilot-skill loop, not a food stop.

| Parameter | Target starting value | Basis |
|---|---|---|
| Precision band | full authority ≤650 cm/s | `[tuning: PT-STAM]` |
| Authority blend | 650–1600 cm/s continuous fade | `[tuning: PT-STAM]` `[derived: no perceptible mode switch]` |
| Level terminal speed | 2400 cm/s | `[tuning: PT-STAM]` |
| Surge ceiling | 3200 cm/s while Surging | `[tuning: PT-STAM]` |
| Steep-dive terminal | 4000 cm/s starting value; test 3200/4000/4800 | `[tuning: PT-STAM]` `[derived: docs/FLIGHT-SPIKE.md turn-radius/world-crossing calculation]` |
| Yaw-rate target | 90°/s precision fading to 55°/s at dive speed — now a **cap** on the coordinated-turn rate, not the turn law itself | `[tuning: PT-STAM]` |
| Lift | `L = 980 × (v/1600)² cm/s²`, clamp 2450 cm/s², directed along the dragon's own up (perpendicular to the wings) in `PhysCustom`: level cruise is altitude-neutral, banked lift steers, inverted lift pulls down. Held pitch pull scales it up to 2× (angle of attack; push unloads to 0×). Momentum weight only; precision/hover and Landing Assist unchanged | `[2026-11-04 PLANE-MODEL]` `[derived: coefficient from gravity at the 1600 cruise reference — proven by the 10 s full-step sim in `UnderTheMountain.Dragon.Movement` (|Δalt| < 50 cm, in fact 0); AoA scale is what lets a held pull close a loop]` |
| Coordinated turn → emergent banked lift | Heading rate emerges from banked lift: lift's horizontal component `L·sin(bank)` is the centripetal acceleration, so heading rate ≈ `L·sin(bank)/v` (≈27°/s at 45° bank, 1800 cm/s); the nose follows via the weathervane. The old `g·tan(bank)/v` yaw law survives only as a test-side analytic comparison | `[2026-11-04 PLANE-MODEL]` `[derived: lift perpendicular to the wings — banking tilts the lift vector, the flight path bends]` |
| Turn authority | Smoothstep (squared) 0 at ≤200 cm/s → 1 at ≥600 cm/s (~0.02 at 300); below 600 flight cannot yaw-spin; precision/hover band is the separate unchanged model | `[2026-11-04 FLIGHT-PHYSICS]` |
| Stall sink | 300 cm/s max (was 600) | `[2026-11-04 FLIGHT-VIGOR]` a stall sags, not drops, while the pilot dives or spends an impulse to recover |
| RMB hold (sailing) | Mouse steers through the same bank/pitch commands: queued mouse-X commands bank, mouse-Y commands pitch, both consumed at the bank/yaw and pitch rates; release keeps the nose (no camera snap-back); double-click RMB still latches steer-to-look (which now banks toward the view), L3 is its pad twin | `[2026-11-04 FLIGHT-PHYSICS]` replaces both RMB free-look (frozen nose) and direct rate-limited nose yaw |
| Roll / bank | Left-stick X (or A/D, RMB mouse-X) is a **roll rate** (120°/s full, authority-scaled), not a bank-angle target: tap to bank, hold to roll through inverted. Left-stick Y is a pitch rate (75°/s full); pitch is never clamped, so holding it loops. Right-stick X keeps camera orbit and adds direct dragon yaw at 30°/s full. Bank is published to animation from the flight quaternion's roll | `[2026-11-04 PLANE-MODEL]` owner spec: dragon handles like a plane; loops and sustained banked turns must be possible |
| Landing Assist | F / gamepad top face near ground: brake to precision, flare, cap descent/drift, settle only inside the safe speed/angle envelope; unsafe approaches brake and warn instead of landing safely by magic | `[derived: owner feedback — a close-to-ground slow/land button is required UX]` |

### Drain / regen (per second unless stated) — all `[tuning: PT-STAM]`

| Action | Stamina | Notes / derivation |
|---|---|---|
| Idle / standing | 0/s, regen +4/s after 3 s idle | Idle must be free `[derived: idea.md — a careful dragon survives by barely moving and eating burglars]` |
| Walk (ground patrol) | −1.5/s | Full pool ≈ 66 s continuous patrol before food pressure `[derived from value]` |
| Run / charge (ground) | −4/s | — |
| Flight, hovering (near-zero horizontal speed) | −3/s | `[2026-11-04 FLIGHT-VIGOR]` top of the signed speed-band table below; hover outranks speed |
| Flight, slow (airspeed <1200, not hovering) | −1/s | `[2026-11-04 FLIGHT-VIGOR]` slow flight is cheap but not free — climb out or speed up |
| Flight, cruise band (1200–1800) | 0/s | `[2026-11-04 FLIGHT-VIGOR]` unpowered travel: energy already in altitude/airspeed pays |
| Flight, fast (airspeed ≥1800, `VigorRechargeSpeed`) | +3/s | `[2026-11-04 FLIGHT-VIGOR, owner override]` dive/fast line refills; one 10-point Surge refunds in ≈3.3 s at this rate |
| Glide (unpowered, wings held) | by speed band above (was flat 0/s) | `[2026-11-04 FLIGHT-VIGOR]` glide is priced by the airspeed it holds, not by posture |
| Ascent (takeoff / climb impulse) | −10 per use (one HUD segment) | Discrete energy purchase `[tuning: PT-STAM]` `[derived: cost attaches to the impulse input, readable as one reserve; 20 → 10 in T-124, 2026-11-04]` |
| Surge (forward impulse) | −10 per use (one HUD segment) | Discrete energy purchase `[tuning: PT-STAM]` `[derived: 20 → 10 with Ascent in T-124, 2026-11-04]` |
| Brake | 0/s to engage | Safety verb must always be available `[derived: high speed is only usable if stopping is never resource-gated]`; resulting hover then pays hover rate |
| Precision rise / sustained precision climb | −3/s | Uses the hover rate `[derived: low-speed vertical thrust is hovering with a climb bias; 8 → 3 with hover in T-124, 2026-11-04]` |
| Dive (nose-down posture) | +speed, stamina by speed band (+3/s once ≥1800) | Dive converts altitude to speed `[derived: flight physics, idea.md]`; `[2026-11-04 FLIGHT-VIGOR]` crossing `VigorRechargeSpeed` is what refills the pool — the dive is the refill verb |
| Hover | −3/s | Priciest sustained flight state `[derived: hover removes momentum skill; price the loiter, T-075]` `[tuning: PT-STAM — 8 → 3 in T-124, 2026-11-04: 8/s grounded the dragon in playtest]`; `[2026-11-04 FLIGHT-VIGOR]` hover is near-zero horizontal speed only — slow forward flight pays the −1/s slow band, not this |
| Fire breath | −5/s while held, min 10 to start | `[tuning: PT-THIEF]` (counter balance §5) |
| Roar | −10 flat, 6 s cooldown | `[tuning: PT-THIEF]` |
| Claw / tail swipe | −4 per swing | `[tuning: PT-THIEF]` |
| Hard landing (impact speed **and descent angle** above envelope) | HP −5 to −25 scaled by impact severity | Landing damage exists `[derived: idea.md]`; scale `[tuning: PT-STAM]`; a fast shallow flare must not price like a vertical lawn-dart `[derived: T-075 momentum descents]`; curve stored in `DA_DragonStats.LandingDamageCurve`, not constants |
| Wall impact (into-surface speed on the same 800→2200 curve) | HP −5 to −25 scaled by severity (T-079); momentum kept 1−severity (graze untouched, lawn-dart crumple, no wall-slide farming) | Same family as hard landing `[derived: T-079 audit gap]`; curve shared with Stats (single source, T-105); source tagged collision in the log for T-074's enemy-hit event to diverge on |

### Zero stamina

| Rule | Value | Basis |
|---|---|---|
| At 0 stamina | HP drains −2/s only while a draining action continues (signed flight rate < 0); unpowered glide, descent, Brake, Landing Assist, and landing always remain available; a cruise/fast line (rate ≥ 0) at 0 keeps flying and refills `[2026-11-04 FLIGHT-VIGOR]` | HP drain at zero `[derived: idea.md/PLAN.md]`; mercy rule `[derived: owner decision 2026-10-31 — exhaustion must not remove the ability to land]`; rate `[tuning: PT-STAM]` |
| Ascent/Surge below 20 stamina; fire/roar at 0 stamina | Blocked (input acknowledged with audio/visual fail state, never silent) | `[derived: AGENTS.md — errors surfaced, never swallowed]` |
| Testing: Infinite Stamina (God Mode) | Settings toggle, default off: stamina costs/drains and exhaustion HP burn disabled; HP damage unchanged; HUD shows a testing badge while on | `[derived: owner request 2026-10-31 — playtest flight feel without the food loop confounding it]` |
| Death | HP 0 → lose path §8 (slink away, not death) | `[derived: idea.md — the dragon does not die on loss]` |

### Food — `DT_FoodValues`

Ordering wildlife < livestock < people in both restore and Notoriety cost
`[derived: idea.md/PLAN.md pillar 3]`. Eating requires landing + eat
animation 2 s `[tuning: PT-STAM]`; interrupted by damage.

| Food | Stamina | HP | Notoriety | Basis |
|---|---|---|---|---|
| Wildlife (deer, goats wild, fish) | +25 | +5 | 0 | `[tuning: PT-STAM]`; 0 Notoriety `[derived: unowned, unwitnessed]` |
| Livestock (farm goat/sheep/horse) | +50 | +10 | +notoriety points (§4 star thresholds) if witnessed / in settlement radius | `[tuning: PT-STAM]`; witnessed rule `[derived: PLAN.md pillar 2 — public acts raise Notoriety]` |
| People — thief caught **inside** lair | +60 | +15 | 0 | `[tuning: PT-STAM]`; 0 `[derived: kills inside are invisible, idea.md]` |
| People — anyone **outside** lair | +75 | +20 | +notoriety points (§4), largest per-eat gain | `[tuning: PT-STAM]`; ordering `[derived: highest reward carries highest cost]` |
| Magic heal (self, `M1` only via Pendant §6) | 0 | per §6 | 0 | `[derived: idea.md heal-by-magic; no innate heal spell in M1 — keeps food loop mandatory]` |

Design check (must hold after tuning): one exterior livestock run
(≈ §1 gap length) refills ≥ half a pool; pure lair-camping on burglars is
sustainable only if catch rate is high; a skilful momentum/skim line costs
materially fewer paid impulses than a climb-and-hover line on the same route,
but stamina never increases from ground-skimming alone (T-075; a skim
only preserves the speed that the ≥1800 vigor band then pays for —
`[2026-11-04 FLIGHT-VIGOR]`). Verified by `PT-STAM` + automation
`AT-Stamina` (drain/regen/food math against `DA_DragonStats`, no level
required).

---

## 3. Scent readability — `DA_ScentConfig`, `DT_ScentBrackets` (`M1`)

One Niagara ribbon per registered treasure, dragon-nose → treasure, always
on (toggleable only by Seeing Stone state, below). Read through
`UTreasureSubsystem`; ribbons never hold actor refs `[derived: PLAN.md
architecture seam]`.

### The no-information rule (invariant, not tuning)

Ribbons encode **value only**. Forbidden encodings `[derived: PLAN.md
pillar 1 — "no distance or room information, ever"]`, enforced by
`AT-ScentNoDistance` (spawn two equal-value treasures at different
distances/rooms; assert ribbon parameters identical):

- No opacity/length/brightness/speed falloff with distance.
- No room/chamber tint, icon, label, or minimap projection.
- No audio pitch/volume proximity cue on the ribbon itself.
- Ribbon renders through geometry the same at all ranges (depth-fade, if
  any, is distance-to-camera artefact control only and value-invariant).

The only way to locate treasure is to move and watch ribbons converge —
patrol *is* the sensor. A stolen item's ribbon follows the carrier (it is
still registered treasure), which is how a theft is noticed at all.

### Thickness formula + value brackets

Value unit: `gold` (hoard score unit, §8). Ribbon width `W` in cm at spawn
(Niagara ribbon width), piecewise-bracketed rather than continuous
`[derived: players must rank ribbons at a glance across a dark chamber;
continuous sqrt scaling collapses mid-range differences — brackets give
named, learnable sizes. Bracket edges/widths are tuning: PT-SCENT]`.

| Bracket | Treasure value (gold) | Ribbon width `W` | Examples | Basis |
|---|---|---|---|---|
| Trinket | 1–24 | 4 cm | coins, goblet | `[tuning: PT-SCENT]` |
| Piece | 25–99 | 8 cm | jewelled cup, weapon | `[tuning: PT-SCENT]` |
| Prize | 100–399 | 14 cm | adventurer's gear, chest | `[tuning: PT-SCENT]` |
| Artifact | 400–999 | 22 cm | §6 artifacts (most) | `[tuning: PT-SCENT]` |
| Crown-class | 1000+ | 32 cm | crown jewels, Seeing Stone, Mind Crown | `[tuning: PT-SCENT]` |

Within-bracket micro-variation: `W_final = W_bracket × (0.9 + 0.2 ×
(value − bracket_lo)/(bracket_hi − bracket_lo))` `[tuning: PT-SCENT]` —
preserves rank inside a bracket without breaking bracket legibility.
Carried loot stacks: a thief carrying N items shows N ribbons converged on
one carrier `[derived: registry is per-item; merging ribbons would hide
count, which is value information the dragon legitimately has]`, rendered
as a bundle with per-ribbon width unchanged; bundle cap: draw max 12
ribbons per carrier, excess widens the thickest `[derived: Niagara
perf budget at 60 fps on M1 Max, PLAN.md perf contract; exact cap tuning:
PT-SCENT perf pass]`.

### Colour & rendering

| Property | Spec | Basis |
|---|---|---|
| Base colour | Tarnished gold `#AF8658`–`#B98B50` emissive, additive | `[derived: ART-DIRECTION.md §2 hoard-gold family; never saturated #FFD700]` |
| Carried-by-thief state | Same colour, gentle pulse (width ±10%, 0.5 Hz) | `[tuning: PT-SCENT]`; pulse marks *carried*, never *near* `[derived: no-information rule]` |
| Wave warning surge (§1) | All hoard ribbons brighten 20 s before a wave | `[tuning: PT-LOOP]` — telegraphs timing, not location |
| Saturation rule | Scent + seeing-stone teal `#75ADA3` are the only saturated gameplay colours in the lair | `[derived: ART-DIRECTION.md §2 — if everything glows, nothing glows]` |
| Perf | Ribbons pooled; far/world treasures use static ribbons, no per-frame respawn | `[derived: 60 fps M1 Max contract]` — verified in PIE perf pass, budget in `DA_ScentConfig.MaxActiveRibbons` `[tuning: PT-SCENT perf pass]` |

### Seeing Stone true-sight (artifact detail in §6)

| State | Scent behaviour | What true-sight adds |
|---|---|---|
| Stone **in hoard** (owned, not worn) | Normal scent | Nothing — it is treasure, worth gold, stealable |
| Stone **worn/active** | Scent ribbons dim to 30% | True-sight overlay: treasure silhouettes visible through rock within sight radius `R_sight` `[tuning: PT-SCENT]` (start value: current chamber + adjacent, i.e. line-of-sight replaced by chamber graph distance 1 `[derived: full-world x-ray would delete patrol, the core loop]`); silhouette colour pale teal `#75ADA3` `[derived: ART-DIRECTION.md §2]`; costs magic 3/s while active `[tuning: PT-SCENT]` |
| Stone **stolen** (thief escaped with it) | Normal scent returns; stone's own crown-class ribbon now leads *out* of the lair | True-sight lost immediately; recovery = intercept the carrier before world-exit despawn, else lost until re-looted (§6 lose condition) |

True-sight shows **treasure only** — never thieves, traps, or people
`[derived: idea.md — stone lets you "see" treasure, not smell it; seeing
thieves is the thief-glow helm's job, §6]`. `AT-SeeingStone`: with stone
active, assert no thief actor is flagged visible by the overlay component.

Empty-pool behaviour (T-080 decision, 2026-11-04): true-sight is refused
at 0 magic (logged, stays off), drains the §2 magic pool at 3/s while
active with regen paused, and shuts off the tick the pool empties —
it never runs uncharged. `UnderTheMountain.Scent.TrueSightMagic` and
`UnderTheMountain.Dragon.Magic` pin the gate, drain and clamps.

---

## 4. Wave table + Notoriety scaling — `DT_Waves`, `DT_NotorietyGear` (`M1`; 4–5★ tail `M3`)

### Notoriety model

Stars 0–5 `[derived: idea.md/PLAN.md GTA-style stars; 5 = army threshold]`.
Underlying points so partial acts accumulate; stars are the display bucket
`[derived: single livestock eat must not equal a public massacre]`.

| Star | Points to enter | Meaning | Basis |
|---|---|---|---|
| 0★ | 0 | Unknown / myth | `[derived: start state]` |
| 1★ | 10 | Rumours; lone opportunists | `[tuning: PT-WAVE]` |
| 2★ | 30 | Known lair; organised parties | `[tuning: PT-WAVE]` |
| 3★ | 60 | Bounty; equipped adventurers | `[tuning: PT-WAVE]` |
| 4★ | 100 | Dragonslayers join waves (`M3` roster; in `M1` = max-gear adventurers) | `[derived: idea.md dragonslayer threshold; M1 substitution per PLAN.md M3]` threshold `[tuning: PT-WAVE]` |
| 5★ | 150 | Army event (`M3`); in `M1` = final-wave composition at max gear + session flagged "army incoming" at resolution | `[derived: idea.md army; PLAN.md M3]` threshold `[tuning: PT-WAVE]` |

Point gains (single writer: `UNotorietySubsystem` `[derived: PLAN.md]`):

| Act | Points | Basis |
|---|---|---|
| Kill/eat **inside** lair, no escapee witness | 0 | `[derived: idea.md — invisible inside]` |
| Thief escapes alive (any type) | +5 | `[tuning: PT-WAVE]` — they talk `[derived: idea.md let-go consequence]` |
| Thief escapes **with loot** | +8 | `[tuning: PT-WAVE]` (proof of riches > a story) |
| Roar-scare outside lair (thief flees, drops loot) | +4 | `[tuning: PT-WAVE]` |
| Eat livestock (witnessed/settlement) | +3 per animal | `[tuning: PT-WAVE + PT-STAM]` |
| Kill/eat person outside lair | +10 per person | `[tuning: PT-WAVE]` |
| Fire breath hits settlement structure | +6 per structure | `[tuning: PT-WAVE]` |
| Crown-jewel / castle heist completed (`M2`) | +25 | `[tuning: PT-WAVE at M2]` |

No passive decay while awake `[derived: idea.md — decay is by sleeping a
generation, §9]`. `AT-Notoriety`: points→stars bucket math + inside-lair
zero-gain invariant, against injected subsystem with fake lair-volume query.

### Wave compositions — `DT_Waves` (counts `[tuning: PT-WAVE]`, all rows)

Party counts as its member count for concurrency. Composition is the
**0–1★ baseline**; Notoriety modifies per the gear table below, it does not
rewrite counts `[derived: counts test patrol skill, gear tests combat —
decouple the two difficulty axes so PT-WAVE can tune them separately]`.

| Wave | Burglars | Dwarf parties (×3 members) | Adventurers | Total actors | Design intent |
|---|---|---|---|---|---|
| 1 | 2 | 0 | 0 | 2 | Teach catch-inside |
| 2 | 2 | 0 | 1 | 3 | Teach roar-scare |
| 3 | 3 | 1 | 1 | 7 | Teach traps/counters |
| 4 | 3 | 1 | 2 | 8 | Split attention across chambers |
| 5 | 4 | 2 | 2 | 12 | Stamina pressure (eat or fade) |
| 6 (final) | 4 | 2 | 3 | 13 | Full test; see §8 |

Spawn staggering: wave members enter in groups every 15–30 s from mixed
entry points, never all at once `[tuning: PT-WAVE]` `[derived: simultaneous
13-actor spawn is a perf spike and a patrol-information overload, not a
fight]`. Between-wave incursions (§1) draw from the *next* wave's roster at
one-actor scale so new types are previewed solo first `[derived: PT-THIEF
teach order — see one, fight many]`.

### Gear scaling by star — `DT_NotorietyGear` (all values `[tuning: PT-WAVE]`)

Applied per-thief at spawn from current stars; a star change mid-wave does
not re-gear live thieves `[derived: readability — gear the player scouts
must stay the gear they fight]`.

| Star | Thief HP mult | Speed mult | Gear unlocked | Carried loot value (gold, per thief) |
|---|---|---|---|---|
| 0★ | ×1.0 | ×1.0 | None (cloth, sack) | 0–10 |
| 1★ | ×1.0 | ×1.0 | Burglar: smoke puff (§5) | 5–25 |
| 2★ | ×1.25 | ×1.05 | Dwarf traps live (§5); adventurer shield | 25–75 |
| 3★ | ×1.5 | ×1.1 | Fire-resistant cloaks (fire dmg −50%); crossbows (projectile dodge matters) | 75–200, chance of Prize+ item |
| 4★ | ×2.0 | ×1.15 | Traps: pit + net (§5); adventurer spears (reach, anti-landing) | 200–500, chance of artifact-class loot (§6 re-loot pool) |
| 5★ | ×2.5 | ×1.2 | Full kit + healer support in parties (`M3` army roster replaces this row) | 500+, guaranteed Prize+ per party |

Higher Notoriety = more risk **and** more loot carried in `[derived:
idea.md — raiders bring better treasure; this is the greed engine of the
loop]`. Loot drops where the thief dies/drops it, registers in
`UTreasureSubsystem` immediately (scent appears), and only counts to hoard
score once the wave ends with it inside the lair `[derived: prevents
score-farming by chasing drops outside — the chase is the Notoriety choice,
not a scoring exploit]` `[tuning: PT-WAVE]` for the scoring moment.

---

## 5. Thief types — `DA_Thief_*`, `DT_Counters` (`M1`)

Shared StateTree (`UThiefBrain`): `Sneak → Grab → Flee`, interrupt states
`Scared / Trapped / Dead` `[derived: PLAN.md architecture]`. Type is data,
not subclasses. Speeds in cm/s (UE units) relative to dragon: dragon walk
600, run 1100, target flight terminals per §2 (2400 level / 3200 Surge /
4000 steep dive) `[tuning: PT-STAM/PT-THIEF — flight model owns final values;
thief speeds below are ratios to dragon walk so they survive flight
retuning]`. All §5 numbers `[tuning: PT-THIEF]` unless
tagged derived.

### 5.1 Burglar (solo, hobbit-like) — `DA_Thief_Burglar`

| Field | Value |
|---|---|
| HP | 20 (one claw swipe, brief fire) |
| Sneak speed | 220 cm/s, near-silent (footstep radius 300 cm) |
| Flee speed | 520 cm/s (below dragon run — catchable on ground `[derived: burglar is the teach-catch type]`) |
| Grab | 1 item per trip, prefers highest-value reachable; grab time 1.5 s stationary |
| Carried cap | 1 item, any bracket (can take an artifact — the §6 threat) |
| Tell | Scent only: a hoard ribbon starts *moving* (carried pulse §3). No audio tell beyond grab clink at 800 cm. |
| Special | Smoke puff (≥1★): on first sight of dragon, 3 s vision obscure + dash 700 cm/s for 1 s, once per incursion |
| Behaviour | Enters via crack (dragon cannot follow — crack is burglar-sized `[derived: intro §7 uses the same crack]`); flees to crack; drops loot if roared at while fleeing inside lair |

### 5.2 Dwarf party (3 members: carrier, trapper, guard) — `DA_Thief_DwarfParty`

| Field | Value |
|---|---|
| HP | 45 each (guard 70) |
| Sneak / flee speed | 180 / 380 cm/s — slowest type `[derived: party power is traps + numbers, not speed]` |
| Grab | Carrier: up to 3 items (stacked ribbons §3), grab time 2.5 s each; party waits while carrier loads |
| Tell | Lantern light + pick/clink audio radius 1500 cm; trapper pauses to place traps (visible crouch 2 s) |
| Behaviour | Enters via peak entrance; moves as group; if carrier dies, another member picks up dropped loot after 3 s; guard faces dragon and blocks (claw dmg −50% from front) while others flee |

Traps (`DT_Counters.Traps`, placed by trapper, max 3 live per party,
visible on close inspection — glint at 500 cm):

| Trap | Star | Trigger / effect on dragon | Counter-play |
|---|---|---|---|
| Caltrops / snare | ≥2★ | Ground contact: speed −50% for 4 s, HP −3 | Fly/glide over; fire breath clears (burns snare) |
| Net drop | ≥4★ | Ceiling trigger in chokepoint: grounded + flap disabled 3 s, mash-free (timed) | Spot the rigging (glint); roar shakes it loose early if roared within 1 s of trigger |
| Pit (covered) | ≥4★ | Ground contact: fall, HP −8, climb-out 2 s | Flight; hardened scales (§6) halve damage |
| Flash pot | ≥2★ | Thrown when cornered: scent ribbons washed out 3 s (whiteout) | Seeing Stone true-sight unaffected `[derived: §3 — sight ≠ smell]`; wait it out |

### 5.3 Adventurer (solo, armed) — `DA_Thief_Adventurer`

| Field | Value |
|---|---|
| HP | 80 |
| Sneak / flee speed | 260 / 480 cm/s |
| Grab | 2 items, prefers weapons/artifacts; grab time 1 s |
| Tell | Armour clink audio radius 2000 cm; does not hide from scent-carried pulse; visibly armed silhouette |
| Combat | Sword: HP −6/hit melee; crossbow (≥3★): HP −4/projectile, dodgeable in flight `[derived: idea.md projectile dodging]`; shield (≥2★) blocks frontal claw, not tail swipe or fire |
| Behaviour | The scare-able type `[derived: idea.md]`: flees at HP <40% or on roar (below); fights if cornered; heads for peak entrance; outside lair keeps running to world-exit — chasing him is the recurring Notoriety dilemma |

### 5.4 Counter matrix — `DT_Counters` (all values `[tuning: PT-THIEF]`)

Dragon damage baseline: claw 25, tail 20 (arc, bypasses frontal block),
fire 18/s in cone (halved vs fire cloaks ≥3★), crush (land/run over) 40,
eat = kill on grabbed/incapacitated target only.

Crush threshold `[tuning: T-077, invented 2026-11-01]`: crush only lands
at an impact/ground speed of **300 cm/s or more** — half the dragon's
600 cm/s walk speed, so a deliberate charge or a committed descent
crushes and a gentle touchdown or idle body contact never farms kills.
Landing crush covers a 300 cm ground-plane footprint under the dragon;
run-over applies to the contacted thief/goat, debounced to one crush
per 0.5 s of grinding contact.

| Counter | Burglar | Dwarf party | Adventurer |
|---|---|---|---|
| **Roar** (10 stam, §2) | Drops loot, flees 4 s panic (random dir, then crack) | Carrier drops 1 item; party holds (guard braces) — roar alone insufficient `[derived: party = organised]` | **Primary counter**: drops all loot, flees to exit; if outside lair, +4 notoriety pts (§4) and he talks |
| **Fire breath** | Kills fast; inside lair free, but fire near hoard risks nothing (hoard is fireproof `[derived: dragon lives on it; punishing fire use at home would break the fantasy]`) | Kills guard last (shield/brace); clears snares/caltrops; trapper flash pot may interrupt | Effective until 3★ cloaks; forces dodge rolls that drop grab progress |
| **Claw / tail** | One-shot if it connects; smoke puff is the dodge | Tail bypasses guard block; claw vs carrier is the efficient kill | Shield blocks claw frontally ≥2★; tail or flank required |
| **Eat** (on downed/cornered) | Best outcome inside: food +60 stam (§2), loot drops in lair, 0 notoriety | Per member; eating mid-party leaves dragon in 2 s eat anim vs remaining members | Only when HP <40% (fleeing) or trapped; catching him outside = +10 pts/person (§4) |
| **Let flee** | −loot carried, +5/+8 pts (§4) | Per escaping member | Same; adventurer escape also previews next wave gear +1 tier for solo incursions `[tuning: PT-WAVE]` (he reports defences) |

Scared state (all types): drop-loot chance/break runs to exit, no grabs,
duration until exit or 6 s unseen. Trapped state: dragon physically
between thief and exit for 2 s → thief surrenders loot, cowers (eatable,
or releasable — release counts as escape for notoriety once outside).
`AT-ThiefStates`: state transitions + drop rules vs fake dragon transform,
no level.

---

## 6. Artifacts — `DA_Artifact_*`, `DT_ArtifactIndex`

Every artifact named in idea.md (refined list + verbatim brain-dump
"Artifact ideas" + body text). Unnamed categories in idea.md ("other
magical objects", adventurers' generic "magical artifacts and weapons",
zealots' generic magical tribute items) are **not** individually speccable
— they enter as generic Prize/Artifact-bracket loot via §4 loot tables and
§9 tribute, with no wearable effect `[derived: an effect needs a named
item to attach to]`.

### Shared artifact rules (all rows)

| Rule | Spec | Basis |
|---|---|---|
| Wearable power = treasure | Every artifact is registered treasure with a gold value (scent §3) **and** an equip effect. Unequipped: rests in hoard, value only. Equipped: effect active, still scented (ribbon converges on dragon). | `[derived: idea.md/PLAN.md pillar 4 — hoard is score and loadout]` |
| Equip/swap | At hoard mound only, 2 s interact, one item per slot (slots in table). | `[tuning: PT-LOOP]`; hoard-only `[derived: forces return-to-hoard trips = patrol beats]` |
| Stealable state | Only unequipped (in-hoard) artifacts can be grabbed (§5 grab rules). Equipped items cannot be taken off the dragon's body. | `[derived: worn-on-a-dragon is not pickpocketable; the loadout choice is which powers to leave exposed]` |
| Lose condition (default) | Carrier crosses world-exit boundary (crack despawn / peak-exit + 30 s outside) still carrying it → artifact **lost**: removed from hoard value, equip unavailable, enters re-loot pool (may return as ≥4★ carried loot §4, tribute §9, or `M2` heist target). Intercept any earlier = recovered where dropped. | Lose-if-thief-escapes `[derived: idea.md]`; boundary timing `[tuning: PT-THIEF]`; re-loot pool `[derived: permanent loss of a signature power in an 8-day build is unrecoverable-feeling; loss must hurt now, not forever]` |
| Intro state | Session 1 starts with the `M1` set equipped (full-power preview §7); `M2/M4` artifacts present in hoard as value-only until their phase. | `[derived: idea.md full-power start; PLAN.md phasing]` |
| XP/level link | Equipping requires dragon level ≥ artifact tier (§8 XP note); `M1` all specced artifacts are tier 1. | `[tuning: PT-WAVE]` `[derived: idea.md L1 start + levels; gating M1 items behind levels adds grind the session can't hold]` |

Modifiers apply through `UDragonStatsComponent` as DataAsset modifiers
`[derived: PLAN.md architecture]`; `AT-Artifacts`: equip/unequip/lose
transitions change stats + hoard total exactly once, no double-count.

### Artifact index

Values in gold (§3 brackets). All effect magnitudes `[tuning: PT-THIEF]`
(combat) / `[tuning: PT-SCENT]` (sight) / `[tuning: PT-STAM]` (economy)
per row tag; lose condition = shared default unless row says otherwise.

| # | Artifact (`DA_Artifact_*`) | Slot · Scope · Value | Effect when equipped | Where found / carried in |
|---|---|---|---|---|
| 1 | **Seeing Stone** | Eye/held-in-claw socket · `M1` · 1200 (Crown) | True-sight per §3: treasure silhouettes through rock (chamber + adjacent), teal `#75ADA3`, scent dimmed to 30%, magic −3/s active. Treasure only, never thieves/traps. | Starts in hoard (intro preview equipped); re-loot pool; `M2` zealot tribute / crown-jewel heist reward |
| 2 | **Hardened Scales** (scale-plates) | Body · `M1` · 600 (Artifact) | All incoming HP damage −30%; pit/landing damage halved (stacks multiplicatively, cap total reduction 50% `[derived: dragon must stay killable at 4–5★, §4]`). | Starts in hoard; re-loot pool; adventurer ≥3★ carried loot |
| 3 | **Magic Pendant** | Neck · `M1` · 500 (Artifact) | Unlocks heal: convert magic → HP at 2 HP per 1 magic, channeled 5 HP/s, must be grounded; magic regen +1/s (base regen 0.5/s `[tuning: PT-STAM]`). This is `M1`'s only magic heal (§2). | Starts in hoard; zealot tribute (`M2`, §9); re-loot pool |
| 4 | **Enhanced Claws** (claw sheaths) | Claws · `M1` · 450 (Artifact) | Claw damage 25 → 40; claw bypasses adventurer shield frontal block; grab-eat window on fleeing thieves +50% duration. Shares Claws slot with #9 — choose one. | Starts in hoard; re-loot pool; dwarf-party guard carried loot |
| 5 | **Mind-Control Crown** ("magic crown") | Head (exclusive with #7) · `M4` effect, present as treasure from `M1` · 1500 (Crown) | `M4`: dominate up to 2 trapped/surrendered thieves (§5 trapped state) as lair guards — they patrol one chamber each, raise a shout (audio tell, no scent change) and wrestle burglars (grab delay +3 s) but cannot kill adventurers; domination costs magic −2/s per guard, breaks if dragon leaves lair >60 s. `M1`: value + intro-preview cosmetic only. Guard count/duration `[tuning: PT-THIEF at M4]`. | Hoard centrepiece from intro (value anchor §8); `M2` crown-jewel heist can re-loot if lost |
| 6a | **Ring of Power** (magic ring) | Ring (horn/claw band, max 2 rings equipped) · `M1` · 400 (Artifact) | +25 max magic; fire breath damage +25%. | Starts in hoard; adventurer carried loot (any star ≥2★); re-loot pool |
| 6b | **Flame Ring** (magic ring) | Ring · `M1` basic / `M4` deepened · 450 (Artifact) | Fire breath ignites: +6 HP/s burn 3 s after cone leaves target; fire clears traps at double radius. `M4`: unlocks flame-form breath variants (specced at M4). | Starts in hoard; ≥3★ adventurer loot; re-loot pool |
| 6c | **Elemental Rings** (frost/storm/etc., one `DA_Artifact_Ring_<Element>` each) | Ring · `M4` · 450 each (Artifact) | `M4`: breath element swap (frost: slow thieves −40% 3 s, no burn; storm: chain to party members, trap disable). `M1–M3`: treasure value only — do not ship partial element system. Elements enumerated at M4 kickoff; this row reserves the slot + data shape `[derived: PLAN.md M4]`. | `M2` heists/tribute add them to hoard as loot; never in `M1` start hoard (nothing to do with them yet `[derived: no dead inventory in the tutorial session]`) |
| 7 | **Treasure-Sight Helm** ("magic helm that lets you see valuable treasure") | Head (exclusive with #5) · `M1` · 550 (Artifact) | Line-of-sight only (does **not** see through rock — that is the Stone's job, resolving the idea.md overlap): Prize+ bracket treasure in view gets a gold outline + its ribbon rendered solid (others stay translucent), no magic cost. Cheap, narrow Stone alternative. Merge candidate if `PT-SCENT` shows players can't distinguish the two `[tuning: PT-SCENT]`. | Starts in hoard; dwarf-party carried loot ≥2★; re-loot pool |
| 8 | **Strength Armor** ("armor that makes you stronger") | Body-harness (stacks with #2 — harness over plates) · `M1` · 650 (Artifact) | Melee damage +30% (claw/tail/crush); carry-drag: can drag one downed thief + their loot at walk speed (reposition food/loot); stamina cost of run/charge −25%. Offence/mobility counterpart to #2's defence. | Starts in hoard; ≥3★ party/adventurer loot; re-loot pool |
| 9 | **Thief-Glow** ("glow like Sting when thieves are nearby"; refined idea.md calls it the thief-glow helm — specced here as a claw-sheath gem so Head stays a Crown/Helm choice; final mount `[tuning: PT-SCENT]` readability) | Claws (exclusive with #4) · `M1` · 400 (Artifact) | Gem glows pale teal-white when any un-spotted thief is within radius `R_glow` (start: same chamber `[derived: chamber granularity is the finest location info any system may give, §3 rule applied to thieves]`); intensity in 2 bands (adjacent chamber = faint, same chamber = bright), **no direction**. Thief location only — no treasure info. | Starts in hoard; burglar carried loot (irony intended); re-loot pool |

Lose-condition exceptions: #5 Mind Crown lost while guards are dominated
(`M4`) → guards instantly freed and flee (count as escapes for §4 if they
get out). #1 Seeing Stone lost while active → §3 stolen state. All
others: effect simply unavailable until re-looted; hoard total drops by
value immediately at the world-exit crossing, not at grab `[derived: a
recoverable chase must not flicker the score]`, verified by `AT-Artifacts`.

Slot summary (loadout = at most): Head 1 of {#5,#7} · Neck #3 · Body #2+#8 ·
Claws 1 of {#4,#9} · Rings 2 of {#6a,#6b,#6c…} · Stone socket #1.
Max equipped value is therefore a strict subset of hoard value — something
powerful is always left stealable in the pile `[derived: pillar 4 tension;
if everything could be worn, nothing could be lost]`.

---

## 7. Intro beat script — `DA_IntroBeats` (`M1`)

Full-power preview framing `[derived: idea.md — Metroid-style start at max
power on a huge hoard, tutorial = the dilemma]`. Dragon starts buried in
the hoard mound, `M1` artifacts equipped (§6), hoard at §8 start value.
No text walls: max one input glyph + 6 words per prompt, everything else
is staging/audio/HUD `[derived: PLAN.md success criterion — understood
without a tutorial wall]`; prompt wording `[tuning: PT-INTRO]`.

| Beat | Time `[tuning: PT-INTRO]` | Staging | Player learns (by doing) |
|---|---|---|---|
| 1. Wake | 0:00–0:20 | Black → eye opens (amber eye POV pull-back to gameplay cam). Dragon half-buried in hoard; scent ribbons bloom in over 3 s. Gnome silhouetted at hoard edge, back turned, pocketing a goblet. Only prompt: "Move". | Movement; scent exists |
| 2. Rise | 0:20–0:40 | Gold cascades off dragon as she stands. Gnome freezes — he did not see her until now. | Scale/power fantasy; hoard is terrain |
| 3. Roar | 0:40–0:55 | Scripted first roar, voiced: "WHO DARES ENTER MY LAIR? THAT DOES NOT BELONG TO YOU." `[derived: idea.md line, verbatim]` Player presses roar input to trigger it (prompt: roar glyph). Gnome drops the goblet (its ribbon settles — carried pulse stops, §3 taught silently), jabbers apology, backs away. | Roar input; roar = loot drop; carried-vs-rest scent |
| 4. Flight of the gnome | 0:55–1:20 | Gnome bolts empty-handed for the crack, squeezes through (dragon cannot follow — body blocks; this teaches the crack rule §5). He is visible outside through the peak entrance light. | Exits exist; not every thief is catchable indoors |
| 5. The choice | 1:20–1:40 | Input fork, no menu: prompt pair — "Let him go" (stay / press nothing for 10 s, timer shown as gnome's shrinking silhouette) vs "Chase" (take-off glyph; fly out the peak). Both prompts carry a single star icon preview (see branch table). | Choice is physical, not a dialogue tree `[derived: tutorialisation without text — the verbs taught are the choice]` |
| 6a. Let-go branch | resolves by 2:30 | Gnome escapes to world-exit. Scent shows goblet safe. HUD: one star outline fills **half-ghosted** + word "Rumours". First burglar incursion (§1 3:00) is framed by audio: the crack again — he told someone the way in. | Mercy costs information |
| 6b. Chase branch | 1:40–3:00 | Take-off teaches flap (prompt per flap until altitude gained), stamina bar appears first time it drains (diegetic trigger `[derived: HUD elements appear when they first matter]`), dive prompt on descent toward gnome. Kill/eat/roar-scare all resolve the beat. Livestock pen + wildlife placed on the return line with eat prompt as stamina nears empty. | Flight, stamina, eating — taught in one motivated loop |
| 7. Open | from 3:00 | §1 loop begins (scripted burglar either way). All HUD live. | — |

### Exact Notoriety consequences per branch

| Branch outcome | Points | Star after intro | Knock-ons |
|---|---|---|---|
| Let go (6a) | +5 (escapee talks, §4) | 0★ (below 1★ at 10 pts `[derived: §4 thresholds — mercy must not grant a star]`) | Crack incursions unlock immediately; wave table unchanged; gnome may return as burglar in wave 1–2 pool `[tuning: PT-INTRO]` |
| Chase → kill/eat gnome **outside** | +10 (person killed outside, §4) | **1★** | §4 1★ gear from wave 1; burglars get smoke puff one wave early — visible consequence within minutes `[derived: feedback must land inside the session or the choice reads as fake]` |
| Chase → roar-scare outside, he lives | +4 (§4) | 0★ | As let-go, plus he tells a *frightening* story: wave 1 adventurer count −0 (unchanged composition), solo burglar preview arrives 30 s later `[tuning: PT-INTRO]` — keep this branch's delta small `[derived: roar-scare is the middle path everywhere, §5.4]` |
| Chase → fail to catch (he reaches world-exit) | +5 (escapee) | 0★ | Identical to let-go econ; player still got flight/food tutorial `[derived: failing the chase must not soft-skip tutorialisation]` |

Either branch converges on §1 at 3:00 — no branch may gate content the
other can't reach within one wave `[derived: 8-day build; the choice tunes
difficulty/Notoriety, it does not fork level content]`. `AT-IntroBranches`:
each of the 4 outcomes applied to fresh subsystems yields exactly the
points/stars above.

Skippable on repeat sessions (hold-to-skip after first completion,
defaults to let-go state 0★/+0 pts — skipping is not a choice, so it
carries no consequence `[derived: never tax replay]`) `[tuning: PT-LOOP]`.

---

## 8. Win / lose thresholds — `DA_HoardThresholds` (`M1`)

Hoard value unit `gold`; tracked by `UHoardSubsystem` from
`UTreasureSubsystem` registry (items physically inside lair count; carried
out = removed at world-exit, §6 rule).

| Parameter | Value | Basis |
|---|---|---|
| Starting hoard (intro, full power) | 8500 gold incl. artifacts §6 | `[derived: bulk + artifact sum below; if either retunes under PT-WAVE, `DA_HoardThresholds` recomputes start, never hand-entered]` |
| — of which bulk treasure (coins/goblets/chests, Trinket–Prize) | 1800 | `[tuning: PT-WAVE]` |
| — of which named artifacts (§6 start set: Stone 1200 + Scales 600 + Pendant 500 + Claws 450 + Crown 1500 + Ring of Power 400 + Flame Ring 450 + Helm 550 + Armor 650 + Thief-Glow 400) | 6700 | `[derived: §6 table sum, computed]` |
| **Win** (session) | Survive wave 6 (final) **and** retain ≥4250 gold (50% of start) at wave-end scoring | Threshold fraction `[tuning: PT-WAVE]`; absolute number `[derived from start]`; both clauses `[derived: surviving broke is the pyrrhic case the greed loop must punish]` |
| **Pyrrhic / slink-away win** | Survive wave 6 with 1700–4249 gold (20%–50% of start) | Session ends "hoard gutted"; `M2` continues at reduced hoard `[tuning: PT-WAVE]` for the fractions |
| **Lose — plundered** | Hoard <1700 gold at any wave-end scoring | `[derived: 20% of start]`; fraction `[tuning: PT-WAVE]` |
| **Lose — driven out** | HP 0 at any time | `[derived: §2]` |
| Lose presentation | Dragon **does not die**: wounded, slinks to a smaller empty lair, reset to L1, artifacts left behind are lost to re-loot pool | `[derived: idea.md lose = reset, not death]`; `M1` = session ends + restart offer; campaign persistence `M2` |
| Mid-session warning | Hoard mound model scales down in 3 steps at 75% / 50% / 20% of start (6375 / 4250 / 1700 gold at current start) so remaining wealth reads off the world, not only HUD | Step fractions `[tuning: PT-LOOP]`, absolutes `[derived from start]`, readability `[derived: pillar 1]` |

Final wave = wave 6 (§4), announced by scent surge + camera/audio sting;
at 5★ it additionally spawns at max gear and the resolution screen names
the incoming army (`M3` continues from there) `[derived: §4 star table]`.
Wave-end scoring moment is when inside-lair loot commits to hoard total
(§4 loot rule). `AT-HoardThresholds`: boundary values 1699/1700/4249/4250 (at current
start; test reads fractions from `DA_HoardThresholds`, not literals)
classify exactly as above.

XP (minimal `M1`, full in `M2`): +10 per thief killed inside, +5 per
thief scared off lootless, +1 per 25 gold net-gained per wave
`[tuning: PT-WAVE]`; level thresholds and stat growth are `M2`
(`DA_HoardThresholds.XPCurve` reserved, empty in `M1`) `[derived: idea.md
progression; PLAN.md M2 dragon XP/levels]`. `M1` displays XP so the
re-loot/level promise is visible, but levels do not change stats in `M1`.

Rival black dragon boss (idea.md low-Notoriety route) and the 5★ army
battle are `M3`, explicitly not `M1` win/lose paths `[derived: PLAN.md
phases]` — specced at M3 kickoff, not stubbed here.

---

## 9. Sleep / tribute — `DA_SleepConfig`, `DT_Tribute` (`M2` only)

Scoped out of `M1` entirely: no sleep verb, no tribute actors in the
hackathon build `[derived: PLAN.md — sleep time-skip + tribute are M2]`.
Specced now because §4 decay and §6 re-loot already reference them; `M1`
code must only reserve the subsystem seams (`UNotorietySubsystem`
decay entry point, `UHoardSubsystem` external-gain entry point), not
implement behaviour.

### Sleep (time-skip)

| Parameter | Value | Basis |
|---|---|---|
| Trigger | Player-initiated at hoard mound, only between waves (no live thief in lair) | `[derived: sleeping mid-burglary is a forfeit button, not a choice]` |
| Time-skip | One "generation" per sleep; world clock advances, settlements regenerate livestock/herds, carried-loot re-loot pool reshuffles | `[derived: idea.md — dragons sleep a generation]`; exact world deltas specked at M2 |
| Notoriety decay | −2 stars per sleep, floor 0★; points reset to the floor of the resulting star band (§4) | `[tuning: PT-WAVE at M2]` — two stars makes one sleep meaningful from 4–5★ without erasing a massacre in one nap `[derived: decay must be slower than gain or Notoriety is free]` |
| Hoard risk while asleep | Each sleep: lose `Risk%` of **bulk treasure only** (Trinket–Prize brackets), start value 15%, rolled per bracket-stack not per item; unequipped artifacts at risk only on a "deep plunder" roll (5%: one random unequipped artifact taken to re-loot pool; equipped artifacts never taken, §6 stealable-state rule) | `[tuning: PT-WAVE at M2]`; bulk-first `[derived: idea.md — lowering infamy "prolly involves having most of your treasure plundered"; artifacts-at-small-risk keeps the gamble real without save-scum bait]` |
| Dragon state on wake | Stamina full, HP restored 50% of missing, hungry pressure: first food need arrives within one patrol loop | `[tuning: PT-STAM at M2]` |
| Wave state on wake | Wave counter does **not** advance during sleep; next wave spawns at post-decay gear tier | `[derived: sleep trades hoard for safety — if waves also advanced, sleep would be strictly bad and never chosen]` |
| Repeat sleep | Allowed back-to-back; each is a generation (decay + risk roll each time) | `[derived: no cooldown needed — the hoard-risk roll is the cost curve]` |

`AT-Sleep` (`M2`): decay band math (−2★ from each starting star, floor),
risk applies to bulk before artifacts, equipped artifacts never taken
(consistent with §6 stealable-state rule).

Sleep implementation decisions (T-084, 2026-11-04): the time-skip is the
T-092 clock jumping to the next dawn (always > 0 h, a full day from
exactly dawn, so back-to-back sleeps cannot spin in place); bulk risk is
15% **of** each bulk stack's gold, rounded to nearest, with the 5%
deep-plunder roll (injected randomness seam in `FSleepRules`) as the
only random event; decay is applied only through
`UNotorietySubsystem::ApplySleepDecay` (the single writer). Sleep is an
atomic skip, not a lingering state, so it cannot soft-lock: the wake
conditions are enumerated in `SleepRules.h` (skip completes, damage or
thief interruption in any future real-time presentation, or cancelling
before the skip starts). PIE glue, the hoard-mound trigger and the
T-068 Sleep/Wake clips remain `M2` work; `UnderTheMountain.World.Sleep`
pins the state rules now.

### Tribute

| Source | Condition | Pays | Basis |
|---|---|---|---|
| Townsfolk (settlement) | Dragon Notoriety 1–3★ **and** no settlement kill/structure damage since last tribute ("feared but predictable") | Bulk gold 100–300 + livestock left at lair entrance (food §2, no notoriety to eat — it was given) per generation-cycle | `[tuning: PT-WAVE at M2]`; band limits `[derived: 0★ = unknown, nothing to fear; 4–5★ = they fund dragonslayers instead, §4]` |
| Religious zealots | Any Notoriety; begins after first public sighting (≥1★ ever reached) | Magical items: generic Prize+ loot, chance of a **lost artifact returning** from re-loot pool (§6) — the reliable artifact-recovery path | `[derived: idea.md — zealots pay tribute with magical items]` odds `[tuning: PT-WAVE at M2]` |
| Delivery | Procession to peak entrance between waves; tribute registers as treasure only once dragon drags/allows it inside lair threshold | Converted per §4 loot-commit rule `[derived: same scoring seam as wave loot, one code path]` |
| Refusal/attack | Attacking a tribute procession = public kills (§4 points each) + that settlement never tributes again | `[derived: consequences must close doors, per pillar 2]` |
| Cadence | At most one tribute per settlement per sleep-generation (or per 3 waves if never sleeping) | `[tuning: PT-WAVE at M2]` `[derived: prevents tribute-farming replacing the raid/defend loop]` |

Tribute is the low-Notoriety income that balances §4's greed engine:
raiding pays more, faster, at star cost; tribute pays slowly and keeps
stars down. `PT-WAVE at M2` tunes the exchange rate between the two.

---

## 10. World life — wildlife and water (T-090/T-091 decisions, 2026-11-04)

Wildlife is the §2 food row as written (deer/boar/fish: +25 stamina,
+5 HP, **zero** Notoriety — unowned and unwitnessed, so eating wildlife
never charges Notoriety through any witness rule) in the pure seam
`FWorldLifeRules`; it grazes beyond 1500 cm of the dragon and flees at
or inside it (just outside fire range, so the hunt stays a hunt). No
wildlife actor exists yet: no deer/boar Meshy asset exists, and a
recycled goat mesh would lie about the species — the actor (wander/
graze/flee on the goat pattern, fire-damageable via T-078, eatable via
this seam) lands with its asset. `UnderTheMountain.World.Life` pins the
values, ordering and flee boundary.

Water: the dragon **wades** at any depth — there is no swim state.
Grounded in water, fire breath is extinguished and vents as steam for
as long as the dragon stands in it; movement is ×0.8 in shallow water
and ×0.5 at or past 200 cm depth; flying over water is wholly
unaffected. The dragon cannot drown at any depth and may always wade
back out, so water can neither kill nor trap it (named here because
"undesigned is the bug", T-091). Water bodies, volumes and materials
on T-049 terrain remain world-building work; the behaviour decision is
`FWorldLifeRules::DecideWaterBehaviour`, pinned by the same test.

---

## Appendix — DataAsset / test index (implementation checklist)

| Asset | Section | Automation test |
|---|---|---|
| `DA_SessionLoop`, `DT_IncursionCadence` | §1 | — (PT-LOOP) |
| `DA_DragonStats`, `DT_FoodValues` | §2 | `AT-Stamina` |
| `DA_ScentConfig`, `DT_ScentBrackets` | §3 | `AT-ScentNoDistance`, `AT-SeeingStone` |
| `DT_Waves`, `DT_NotorietyGear` | §4 | `AT-Notoriety` |
| `DA_Thief_Burglar/DwarfParty/Adventurer`, `DT_Counters` | §5 | `AT-ThiefStates` |
| `DA_Artifact_*`, `DT_ArtifactIndex` | §6 | `AT-Artifacts` |
| `DA_IntroBeats` | §7 | `AT-IntroBranches` |
| `DA_HoardThresholds` | §8 | `AT-HoardThresholds` |
| `DA_SleepConfig`, `DT_Tribute` (`M2`) | §9 | `AT-Sleep` (`M2`) |

Open tuning debt is exactly the `[tuning: …]` tags above — grep this file
for `[tuning:` to enumerate it. A value loses its tag only when its named
test has run and this file is updated with the result (dated, per
AGENTS.md documentation protocol).







