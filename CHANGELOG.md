<!-- Contract (AGENTS.md -> Documentation protocol): one entry per task with a
     user-visible effect, format `YYYY-MM-DD — T-### — <what changed, user-visible
     effect>`; internal-only tasks get one line ending `(no user-visible change)`. -->
# CHANGELOG — Under the Mountain

2026-11-04 — T-139/T-126/T-114 — ABP_Dragon graph authored in-editor by the ABPAuthor commandlet (6-state locomotion, 30 transitions, FullBody/UpperBody slots, chest layered blend, root-bone bank/pitch): the dragon now animates through the graph in-game (course log clip=ABP, states cycling, zero bind frames); 10 new animation families imported (hit-heavy, defeat, sleep/wake, fire pose, claw, bite/eat, tail whip, wing buffet); nat-broadleaf/bush/grass foliage imported and placed (120 props)

2026-11-04 — T-138/T-135/T-131 — Sandbox mode (title + utm.sandbox, SANDBOX badge, no spawns/audit/notoriety), distinct PYRRHIC WIN end screen with inside/outside resolution score, and incursion cadence (90–150 s gaps, 20 s wave warning, 4-min wave cap)

2026-11-04 — T-128/T-088 — Graphics quality presets (Ultra default, Low/Medium/High relief) + settings model/widget (volume, sensitivity, inverts, camera, infinite-stamina toggle) with persisted config

2026-11-04 — T-069/T-070/T-076/T-102 — Smart target lock, treasure pickup/carry/drop/deposit, claw attack (25 dmg, shield-block matrix), and pad remap RT Surge / LT Brake / RB Fire with right-face tap/hold multiplex

2026-11-04 — T-080/T-084/T-092/T-090/T-091 — Magic pool charging true-sight, sleep time-skip with notoriety decay + hoard risk, day/night world clock, wildlife food values, and wading water behaviour (systems live; world actors for wildlife/water still owed on their tasks)

2026-11-04 — T-074/T-089/T-132/T-133/T-134 — Thieves can hurt the dragon (tells, HP, defeat slink-away), waypoint navigation replaces hard-coded chamber steering, star-tier gear + raider loot, dwarf carrier/trapper/guard roles with crack routing, and witness-based notoriety (unwitnessed wilderness eats are free)

2026-11-04 — T-081/T-136/T-071 — Artifact catalogue with registry-truth effects (Seeing Stone, scales, flame/power rings, claws), gameplay config defaults pinned to shipped values, and an audio director seam (music/SFX slots per verb; friend-music masters still to ingest)

2026-11-04 — T-127 — Scent ribbons retuned: density + metallic gold sheen scale with value, end-on white-bar faded (Niagara user-vars still to author in-editor)

2026-11-04 — T-119/T-137 — Stale dragon-import scripts re-pointed at the live v6 sources, appearance comments corrected, README refreshed to the real tree (no user-visible change)

2026-11-04 — T-122 — World map rebuilt: the valley village moves out from under the terrace slabs that roofed it (now open sky at x=50000) and Villages N/E move onto local flats; terrain re-pressed under the whole flight zone with zero poke-throughs; all 9 settlements verify sunlit in-game and every capture shows readable sunlit pixels — the world reads as a normal medieval landscape

2026-11-04 — T-123 — Dragon mesh surgically repaired: chest-side + tail-spine holes closed (1,308/1,308 named loops), wings verified bit-identical, textures read unified in lair + sunlight; same rig, clips, and materials, zero Meshy spend

2026-11-04 — T-125 — ABP_Dragon agent lane: animation brain completed in C++ (hybrid-aware state sampling, published locomotion state, auto-detects the graph with single-node fallback); one-shots play through graph slots; owner build sheet for the graph filed as T-139 (no user-visible change yet — dragon animates exactly as before until the graph lands)

2026-11-04 — T-124 — God mode test slice + flight stamina retune: utm.testing.infinitestamina cvar (default off) disables every stamina cost/drain and the exhaustion HP burn while HP damage still applies, with a TESTING badge on the HUD while on; hover drain 8/s → 3/s and Ascent/Surge reserve 20 → 10, so the dragon stays airborne (~34 s sustained hover from full, was ~12.5 s)

2026-11-03 — T-138 — Sandbox mode (rules-off free flight) tasked; gamepad remap unblocked (Option A), music credit recorded (no user-visible change)
2026-11-03 — T-129 — Comprehensive review filed: final freeze, judge launch path, cadence/notoriety/witness tasks added; owner manual suite in meatsuite.md (no user-visible change)

2026-11-03 — T-124 — Final sprint seeded: god mode + flight, animation, dragon repair, world rebuild, scent, and graphics tasks filed from owner playtest feedback (no user-visible change)

2026-11-04 — T-118 — Camera rig hardening: capture forcing no longer touches the player save, unknown modes log loudly, chest/head_end pinned, flying Eyes arm==0 pinned (no user-visible change)

2026-11-04 — T-117 — Proof-spawn capture cvar hardened: fires once when possessed, warns on malformed specs (no user-visible change)

2026-11-04 — T-116 — Proof-fps readout fixed (%lld frame count) + short-window runs warn instead of reporting 0.0 fps (no user-visible change)

2026-11-04 — T-112 — Title screen renders again (gold title + START RUN + QUIT, focus error gone); START RUN reaches gameplay with the full HUD (no user-visible change beyond the restored title)

2026-11-04 — T-109 — Deviation-log sweep #3: audits folded, 13 adversarial findings filed as T-116–T-119 (plus 1 inline README fix), successor T-120 filed (no user-visible change)

2026-11-04 — T-101 — Removed two stray editor lock files swept in with the goat work (no user-visible change)

2026-11-04 — T-020 — Packaged macOS build: Scripts/package.sh produces a runnable .app; fresh launch reaches gameplay (title → playing, thieves stealing, newest-clip flight); 1280×720 windowed measures ~59 fps lair / ~56 fps village — just under the 60 gate, perf pass filed as T-115

2026-11-04 — T-111 — Three dragon cameras: V/R3 cycles Eyes → Near → Far opening on Near — first-person at the eyes (mesh hidden), near chase above the wings, far scenic chase

2026-11-04 — T-113 — Newest owner dragon flies: all 9 animation clips (fly, glide, hover, walk, takeoff, land, idle, roar, hit) play on the v6 rig, so flight/hover/landing read as posed wings instead of bind pose; retired the superseded v4/v5 dragon assets

2026-11-03 — T-110 — Playable dragon is now v6 (new 39-bone rig, 4K body/wing PBR, tint still recolours the body only); dragon holds bind pose until its animation clips land

2026-11-03 — T-086 — Hoard mound visibly shrinks in 3 steps as thieves steal (75/50/20% of start)

2026-11-03 — T-083 — XP pool: inside kills +10, lootless scares +5, wave gold +1/25, shown on the HUD, no levels yet

2026-11-03 — T-079 — Flying into walls now costs HP on the shared landing curve and kills momentum by severity; brushes stay free

2026-11-03 — T-073 — First-session contextual prompts teach each verb once, plus an H overlay listing every binding (generated from the shipped scheme)

2026-11-03 — T-072 — Game-flow shell: title → playing → pause → won/lost screens, Esc pauses, restart reloads a clean run, quit works

2026-11-03 — T-108 — Doc honesty pass: glide 0/s, shipped (not prototype) chassis, glide-free mercy note, GC test covers all 21 input members (no user-visible change)

2026-11-03 — T-107 — True-sight markers now exist only while true-sight is active: no more one-hidden-actor-per-treasure while off (no user-visible change)

2026-11-03 — T-106 — Content scripts back up the map before destroying anything and refuse to save duplicates (no user-visible change)

2026-11-03 — T-105 — The landing-damage curve lives in Stats alone now: wall-impact severity and the integrator's floor-contact routing read the same numbers instead of a mirrored copy (no user-visible change)

2026-11-03 — T-104 — A wave with a missing thief now fails loudly: an error names the wave and the missing slot instead of silently clearing it (no user-visible change)

2026-11-03 — T-066 — The lair is a mountain now: a 100 m great chamber with a 25 m open crown, dragon-passable east portal and west cave, two thief-only cracks the dragon cannot fit through, and the hoard on its own mound clear of the landing disc

2026-11-03 — T-065 — Three dragon cameras: head-anchored chase by default, wide scenic and first-person on V (gamepad R3), persisted across sessions; pad stick aims the nose and every pitch axis inverts separately

2026-11-03 — T-064 — The dive is now readable: the HUD shows DIVING (with the recovery), HOVER, BRAKING, or GLIDING while airborne, and the debug readout names the same states in player terms

2026-11-03 — T-103 — Dwarf trap slow now works in flight: a trapped dragon climbs, turns, and thrusts worse until the effect expires, then recovers exactly (no user-visible change)

2026-11-03 — T-034 — Sweep #2: plan gate sentence folded past-tense, 9 audit findings filed as T-103–T-108, successor sweep T-109 filed (no user-visible change)

2026-11-03 — T-063 — Right-mouse hold looks around without turning the dragon and double-click latches steering to the view (any move input cancels); gamepad D-pad Up latches and unlatches the same way

2026-11-03 — T-062 — Flight is the hybrid model now: the dragon flies by pitch and momentum at speed, translates and hover-holds at low speed, takes off with Space, brakes with Shift (S flares too), lands with F assist, and gliding costs no stamina

2026-11-03 — T-061 — Scent ribbons can be hidden with N (gamepad D-pad Down) and shown again with a second press; the debug readout names the state, and true-sight markers stay on their own T-key layer either way

2026-11-03 — T-100 — New raw assets staged in the repo (not in-game yet): dragon v6 wing/fold model with 4K textures, three foliage sets (broadleaf, bush, grass), and the animation script retargeted to the v6 rig; engine import and placement still to come (no user-visible change)

2026-11-03 — T-075 — Flight-controls spike measured all three schemes on a scripted live-world course and provisionally picks Scheme C (hybrid) for the T-062 overhaul: real climbs, controlled turns, hover-hold and clean assisted landings, all behind a default-off console toggle (no user-visible change)

2026-11-02 — T-058 — Exhausted dragons no longer walk in mid-air: falling uses flight poses (wings out, legs tucked) instead of the ground Walk clip; wingtips extended further (span ~1.8) on the perpendicular line

2026-11-02 — T-058 — Wing membrane cut to the arm: the sail no longer webs back to the hips (chord pulled to the wing bones), and hind legs trail streamlined along the tail instead of splaying

2026-11-02 — T-058 — Bird wings, not bat sails: the membrane is detached from the hips and tail so each wing is an independent arm on the armature-diagram gull line (shoulder high, straight out and down to the tips), flapping as one rigid straight wing

2026-11-02 — T-058 — Wings now flap straight like a bird's: one rigid fully-extended wing per side, tips pointing straight out and never folding back to the body through the whole stroke (the previous clip measured 100+ degrees of bend at the wrist — fixed in the rig data and locked by a test)

2026-11-01 — T-058 — The wings are actually bigger now, in the skeleton itself: longer wing bones and a larger membrane replace the first pass's pose-stretch, so the span (~1.7× the old tips-beside-hips reach) is the dragon's real anatomy in every pose, folded or flying

2026-11-01 — T-058/T-068 — Flight finally looks like flight: wings spread to full span and beat around it, all four legs tuck streamlined, glides hold still instead of slow-flapping, hovers beat deep with legs ready, the body pitches with the dive and banks into turns, and takeoff/landing/roar have real animation — plus two new verbs, wing buffet (X: dazes thieves in front of you) and tail slam (Z: 20 damage behind you, through a shield)

2026-11-01 — T-019 — A real HUD: HP and stamina bars bottom-left, gold counter, notoriety star pips and wave state top-right on translucent panels — and the intro choice popup got the same modern treatment (centred panel, gold title, ember Chase button). Every action works on gamepad or keyboard/mouse alone, intro choice included

2026-11-01 — T-018 — The game has a first minute now: you wake on your hoard as the camera pulls back, a gnome-thief grabs your gold, and the game stops to ask — let it go (it will talk: +1/2 a star of notoriety) or chase it down. Ignoring the question answers it for you, so it can never soft-lock

2026-11-01 — T-016 — It is a game now: thief waves roll in on a stagger and loot your hoard while notoriety stars climb for every witnessed act (escapes, public kills, roars, burnt buildings), the hoard drains as loot walks out, and surviving the final wave rich wins — ignoring a wave is allowed, and the world keeps score anyway

2026-11-01 — T-057 — The standalone-launch crashes are diagnosed and the broken path is retired in the docs: the unstaged game binary cannot find its project or ICU data when launched directly (one chain: descriptor → ICU assert → segfault, crash-report proven); `Scripts/play.sh` is the supported way to run the built game until packaging (no user-visible change)

2026-11-01 — T-056 — One-command playtest: `Scripts/play.sh` builds the game and opens a standalone window on the Lair running your current code — no editor, no stale builds, no MCP state to fight

2026-11-01 — T-055 — The earth catches fire: hold your breath on crates, barrels, carts or fences and they ignite, burn, and stay burnt rubble with a scorch mark; burning props hurt anything standing in them, and sustained fire on the ground leaves scorch discs

2026-11-01 — T-054 — Death and pain read now: killed thieves fall into a ragdoll settle and flinch when hit, and goats are real animated Meshy goats that walk their pasture and tip onto their side when killed (no more spheres with boxes for heads)

2026-11-01 — T-053 — The towns have people now: villagers wander and idle in the valley village and Village N and scatter when the dragon gets close, and every farmstead has goats — 19 new inhabitants at no measured frame cost (about 74 fps in the village)

2026-11-01 — T-052 — The exterior has a sky: sun with shadows, atmosphere, a fly-through cloud deck and height fog — plus weather: type `Weather` in the console (or set utm.weather.state) to trade clear skies for dim blue rain at about 1 ms of frame cost

2026-11-01 — T-067 — Animation spike closed: T-059's survey is integrated in docs/ANIMATION-SPIKE.md and the adopted v1 clip set stands confirmed (later trot/gallop gains a named CC0 retarget candidate); implementation is T-068 (no user-visible change)

2026-11-01 — T-059 — CC0 animation survey landed (docs/ANIMATION-SOURCES.md): Quaternius quadruped locomotion is the one credible CC0 retarget source, its low-poly dragons the only winged candidate; no CC0 flight/combat source matches our wing, so hero clips stay Blender-authored — report only, nothing adopted (no user-visible change)

2026-11-01 — T-051 — Fire breath reads as fire now: a real Niagara particle stream and a flickering orange light replace the orange cone mesh (same damage underneath; about 73 fps measured in the village with the breath lit)

2026-11-01 — T-050 — Settlements across the world: two castles (a north-west courtyard fort and a south-east ridge fort), two new villages (a northern house ring, an eastern inn-led road bend) and four farmsteads now dot the terrain kilometres apart — and the original village's houses stand upright for the first time (a rotation bug had them pitched since they were placed)

2026-11-01 — T-094–T-099 — Stretch goals scoped from the demo-story audit: castle garrisons, archers, attackable castles, settlement-scale burning, notoriety mobilisation, and a mage-tower decision spike filed in TASKS.md Later with sequencing in docs/STRETCH-GOALS.md; nothing scheduled, M1 gate unchanged (no user-visible change)

2026-11-01 — T-049 — The world is huge now: a real 5 km × 5 km landscape of fields, ridged ranges and valleys rings the lair mountain (about 4.7 minutes of flight edge to edge), generated by a new editor commandlet — flying out of the peak no longer ends at the edge of a greybox

2026-11-01 — T-048 — unreal-mcp is boring now: the editor auto-starts the MCP listener from shipped project config (it used to depend on machine-local Saved/ state), run-editor.sh refuses a second project editor and confirms the listener is up — three fresh launches verified (no user-visible change)

2026-11-01 — T-082 — The lair's hoard is filled to spec: 33 treasures worth 1,800 gold of bulk treasure spread across every chamber (was 3 items / 310), so scent ribbons now vary in width room to room and wave scoring has something to measure

2026-11-01 — T-078 — Fire breath now burns goats (and anything else damageable in the cone), not just thieves: held breath kills a goat in about two seconds and the body stays eatable

2026-11-01 — T-077 — Landing on or charging into a thief or goat now crushes it (40 damage, per GAMEPLAY.md §5.4): drops and ground charges at 300 cm/s or faster kill what's under the dragon, gentle touchdowns don't

2026-11-01 — T-060 — The C-key scale-colour picker is now one draggable colour pad plus a hue strip (drag to browse shades, the dragon recolours live and the choice saves as before); the three separate R/G/B sliders are gone

2026-10-31 — Owner review processed — Sandbox guarantee written into PLAN/GAMEPLAY (PT-LOOP is a benchmark, never rails; unscripted "go rogue" demo capture is supported), music is friend-composed and will be ingested — never generated — under T-071, the dragon pipeline is stated everywhere as Muse multi-images → Meshy mesh/textures → Blender rig/animation → Unreal, and a Meshy credit forecast + spend guardrails now own the remaining ~450 efficient / ~625 quality-safe estimate (no user-visible change)

2026-10-31 — T-075/T-067 — Flight and animation spike decisions adopted from owner annotations + delegated best-judgement calls: hybrid momentum/precision flight with dive-as-posture, Brake, Landing Assist, segmented stamina, no speed/skim stamina regen, Infinite Stamina testing setting; lean ABP_Dragon architecture, exact v1 clip set incl. reusable Sleep/Wake, CC0-only external animation, and family-specific death presentation; GAMEPLAY.md §2 target amended, no behaviour changed yet (no user-visible change)

2026-10-31 — T-075/T-067 — Flight and animation spike findings published: hybrid momentum/precision flight is the recommended prototype direction, and a UDragonAnimInstance + ABP_Dragon montage/additive architecture is the recommended animation direction; no flight or animation behaviour changed, and prototype feel/owner picks remain gated in docs/FLIGHT-SPIKE.md and docs/ANIMATION-SPIKE.md (no user-visible change)

2026-10-31 — T-075–T-092 filed — Full-scope audit gaps filed at owner direction (no scope cut): a flight-controls spike ahead of T-062, claw/crush verbs, fire-damages-goats fix, flight collision damage, magic pool, the full artifact set + wearables, bulk treasure to spec, XP, sleep, tribute, hoard mound scaling, save system, settings, real navigation (kills the hard-coded chamber steering), wildlife, water, and a day/night clock; ground-idle gap folded into the T-067 animation spike (no user-visible change)

2026-10-29 — T-071–T-074 filed — Owner approved four demo-gap additions from repo review: an audio pass (the game is silent), a game-flow shell (title/pause/restart/quit), onboarding prompts + help overlay, and enemies that can hurt the dragon; F6 mouse-steered flight clarified on T-063 (RMB free look, double-click move/steer by best UX, movement keys cancel) (no user-visible change)

2026-10-29 — T-057–T-070 filed — Owner Path A playtest feedback filed as 14 tasks: standalone-launch crash (SIGSEGV/descriptor/ICU), flight anim + controls + cameras rework, mountain-shaped lair overhaul, animation spikes, lock-on targeting + treasure pickup, one-picker tint, scent hotkey; docs/PLAN.md addendum records the scope changes, no work started (no user-visible change)

2026-10-28 — T-045 — Fixed the wall of "duplicate PrimaryAssetID" plugin warnings on editor launch (asset-manager scan was registering every file in the project) (no user-visible change)

2026-10-28 — T-046 — The village is real now: all 28 Meshy world models imported in one automated pass (no import dialogs) and the houses, inn, tower, rocks, and pines stand where the greybox village was

2026-10-28 — T-047 — Wing-animation diagnosis: anim state changes are now logged in the Output Log so a live flight proves which state the wings are in; if your wings still look frozen, restart the editor first — an editor left open across code builds flies the old pawn (no user-visible change)

2026-10-28 — T-015 — Combat is in: hold LMB to breathe fire (it drains stamina and melts thieves), R to roar loot out of a thief's hands, E next to a corpse or goat to eat and get your stamina back

2026-10-28 — T-017 — You can fly out of the mountain: the peak shaft opens onto a plateau, terraces drop to a valley village with goats wandering the pasture — all one map, no loading, ~62 fps over the village

2026-10-28 — T-044 — The thieves have faces now: burglar, adventurer, and dwarf move through the lair on their Meshy models with walk and run animations, no longer grey spheres

2026-10-28 — T-043 — The dragon now flaps: a new flight animation plays while flying (faster in a dive, slow and held in a glide) instead of the ground walk cycle in mid-air

2026-10-28 — T-014 — Thieves are in the lair: a burglar, a dwarf trapper, and an adventurer sneak to treasure, grab it, and run for the exit — catch the dwarf's traps and your wings slow to half speed until they wear off

2026-10-28 — T-041 — Honesty/dead-code cleanup from the sweep: removed the invisible placeholder body and other dead code, and corrected comments that described the game as it no longer is (no user-visible change)

2026-10-28 — T-040 — Scent ribbons now stretch the full dragon-to-treasure distance even across the lair, instead of fading out after a few metres

2026-10-28 — T-039 — Dragon tint tests can no longer overwrite your saved scale colour (they use their own save slot), and a failed or corrupt tint save now reports itself in the log (no user-visible change)

2026-10-28 — T-038 — Scent/treasure lifetime fixes: scent ribbons and markers can no longer outlive their world or registry, moved treasure now keeps its scent endpoint, and failed treasure registration reports itself as failed (no user-visible change)

2026-10-28 — T-037 — Build and level-generation scripts now fail loudly instead of reporting success after a masked format error, failed level check, or failed asset operation (no user-visible change)

2026-10-28 — T-036 — A dragon that empties its stamina mid-dive now keeps diving instead of being forced out of flight; glide/hover exhaustion still burns HP and ends flight

2026-10-28 — T-035 — Fixed a GC ownership bug: the dragon pawn’s input actions, mapping context and tint-picker widget are now reflection-tracked so the garbage collector cannot collect them under the pawn (no user-visible change)

2026-10-28 — T-004 — Sweep #1: plan/readme brought back in line with the shipped game; 8 audit findings filed as T-035–T-042 (no user-visible change)

2026-10-28 — T-033 — Fixed dead dragon controls: input actions are now built before any binding can store them (they were bound as nulls, so no key ever fired); WASD/mouse/Space/F/Shift/T/C and gamepad work in PIE

2026-10-28 — T-027 — Dragon scale-colour picker: press C (or gamepad right face) in play for RGB sliders that retint the dragon's scales live — crimson wing membranes and spine stay crimson — and the choice persists across restarts; the dragon also gains its real textured material (M_DragonScales from the Meshy V4 maps — it previously rendered in WorldGridMaterial)

2026-10-28 — T-031 — Scent ribbons are now Niagara: NS_ScentRibbon sweeps nose-to-treasure per registered item, width set by its gold value, true-sight dimming via particle alpha (replaces the cube-geometry ribbons)

2026-10-28 — T-032 — The playable dragon is now the real dragon: SK_Dragon_v4 renders on the pawn (placeholder ball hidden), standing in the capsule at 2x scale with the T-029 walk cycle playing while moving; evidence Content/Meshy/Dragon/dragon_pawn_pie.png

2026-10-28 — T-013 — Treasure + scent: treasures placed in the lair (goblet, cup, chest, Seeing Stone) register on spawn and project scent ribbons whose thickness scales with gold value; Seeing Stone true-sight toggle (T / gamepad left face) dims scent and reveals teal treasure markers; ribbon is cube geometry pending Niagara upgrade T-031

2026-10-28 — T-012 — Lair level built: 3 chambers + hoard room + peak shaft, dressed with gold piles, crates, and torch lights (Content/Maps/Lair/Lair.umap); scripted traversal 14/14 and 60 fps at vsync cap measured on this Mac

2026-10-28 — T-011 — Flight feel systems: flap/glide/hover/dive stamina economy, hard-landing HP damage, velocity debug HUD (PIE feel check filed as T-030) (no user-visible change)

2026-10-28 — T-029 — Dragon rigged and walking: 33-bone quadruped rig on Meshy V4, diagonal-gait walk cycle in UE (SK_Dragon_v4 + AnimSequence); Scripts/run-loop.sh added (no user-visible change)

2026-10-28 — T-029 — Meshy V4 dragon: rock-free, single regular eyes, 310k tris textured (980k pre-remesh master) imported as SM_Dragon_v4; rig/walk blocked on owner decision (no user-visible change)

2026-10-28 — T-028 — Dragon re-rigged in Blender: rock base removed, quadruped rig with wing + tail bones (79k tris) replaces the broken humanoid rig in UE (no user-visible change)

2026-10-28 — T-028 — Ultra-detail dragon source generated: dragon_ultra.glb/fbx (309,716 tris, 4K textures) in Content/Meshy/Raw/Dragon for the Blender re-rig (no user-visible change)

2026-10-28 — T-010 — Dragon 3D asset in engine: Meshy gold/crimson dragon (82,933 tris) imported to Content/Meshy/Dragon; auto-rig verdict humanoid/unusable — Blender re-rig filed as T-028 (no user-visible change)

2026-10-28 — T-022 — Real C++ line coverage: coverage.sh measures 31.1% (llvm-cov), floor ratcheted to 29% and enforced by the gate (no user-visible change)

2026-10-27 — T-026 — Gameplay spec published: docs/GAMEPLAY.md with implementable numbers/tables for loop, stamina, scent, waves, thieves, artifacts, intro, win/lose (no user-visible change)

2026-10-27 — T-025 — Concept art pack approved: gold/crimson dragon (multi-view), lair, village, 3 thieves, hoard in docs/concepts/ + docs/ART-DIRECTION.md; player scale-colour picker filed as T-027 (no user-visible change)

2026-10-27 — T-025/T-026 filed — Art direction locked to Style B (realism); concept pack + gameplay spec queued (no user-visible change)

2026-10-27 — T-003 — Playable walking skeleton: dragon pawn (walk/fly/flap/dive, stamina) spawns in the greybox lair in PIE on Mac

2026-10-27 — T-023 — unreal-mcp proven live against the editor (Hermes connects; toolsets listed) (no user-visible change)

2026-10-27 — T-024 — Gate builds made deterministic cold (-nocache); UBA + editor-token foot-guns recorded (no user-visible change)

2026-10-27 — T-002 — UE 5.8.4 project + gate scripts live; editor/game builds and automation smoke test green on this Mac (no user-visible change)
2026-10-27 — T-001 — Design plan, refined idea, and M1 backlog published (docs/PLAN.md, idea.md) (no user-visible change)
2026-10-27 — T-000 — Project scaffolded from templates; process law (AGENTS.md) and task board live (no user-visible change)
