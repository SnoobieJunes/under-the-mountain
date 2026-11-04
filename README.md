# Under the Mountain

Open-world dragon simulator for macOS (Apple Silicon). You are the dragon:
guard your hoard, smell treasure you cannot locate precisely, and decide —
every time — whether a thief is worth the Notoriety. Meta AI Developer
Hackathon 2026, Indie track (due 2026-11-04).

- Design vision: `idea.md` · Normative plan: `docs/PLAN.md` · every gameplay
  number: `docs/GAMEPLAY.md`
- Process: `AGENTS.md` (rules), `TASKS.md` (work queue), `goose-deuce.md`
  (deviation log), `CHANGELOG.md` (history), `docs/ASSET-PIPELINE.md` (how
  assets are made)

## Status (2026-11-04)

Playable in the editor: a textured, rigged dragon with flight animation in a
dressed mountain lair (100 m great chamber + open crown, dragon-passable east
portal and west cave, two thief-only cracks), the signature
scent system (Niagara ribbons, value-only), a scale-colour picker, and live
combat (fire breath, roar, eat). Three Meshy-modelled thief types (burglar,
dwarf party, adventurer) run a data-driven AI state machine in the lair, and
the grand portal flies out onto a real Meshy village with goats in the valley —
all one map, no loading, measured at ~57 fps average on the M1 Max (lair ~59, village ~56; perf pass T-115 filed).

Shipped since that status: Notoriety/waves/win-lose (T-016), the intro
(T-018), HUD + menus (T-019/T-072), help/onboarding (T-073), the packaged
macOS build (T-020; rebuild with `Scripts/package.sh` — `Saved/` is not in
git, so no `.app` is on disk in a fresh clone), and the settings logic
slice (T-128/T-088: quality presets + a settings model/widget in
`Source/UnderTheMountain/Public/UI/`; wiring the screen into title/pause
and proving fps per preset remain on those tasks). Still owed (see
`TASKS.md`): audio (no `Content/Audio/` in the tree yet, T-071), navigation,
day/night, and the remaining board. No claim below rests on a task that
has not closed, except the settings slice, which is labelled as such.

## Install / run / test

Engine: Unreal Engine 5.8.4 source build at
`/Users/auston/Development Projects/UnrealEngine` (Xcode 27.1, macOS, M1 Max
reference machine). Everything runs from the repo root:

```bash
Scripts/setup.sh       # verify engine + toolchain (UE 5.8.4, Xcode 27.1)
Scripts/build.sh       # build the editor target (compiles the game module)
Scripts/run-editor.sh  # open the editor; pass /Game/Maps/Lair/Lair to open the lair
Scripts/play.sh        # quick test: build the game target + standalone -game window (no editor)
Scripts/package.sh     # T-020 packaged macOS build → Saved/Packaged/Mac/UnderTheMountain.app
Scripts/test-fast.sh   # build + automation tests (inner loop)
Scripts/gate.sh        # full pre-commit gate: setup, editor+game builds,
                       # clang-format check, tests, coverage-floor check
Scripts/coverage.sh    # real llvm-cov line coverage (floor: AGENTS.md, 37%)
```

To play: open the editor, load `/Game/Maps/Lair/Lair`, press **Play**. The
title screen opens first (world paused) — press **Start Run** (the intro
wake-up plays, then free play). **Esc** pauses and resumes; the pause,
win, and lose screens offer restart (clean run via map reload), title,
and quit. The dragon spawns at `DragonStart` in the great chamber; the hoard mound, the
crown opening, the caves, and the village are all reachable from there.

**Quick test (no editor): `Scripts/play.sh`** — builds the game target and
launches a standalone game window on the Lair running exactly the code you
just built. Use this to test current code; use the editor when you need to
inspect or change the level/assets, or drive it over MCP.

**Packaged build (T-020): `Scripts/package.sh`** — cooks Lair, builds the
game target, and stages a runnable `.app` at
`Saved/Packaged/Mac/UnderTheMountain.app` (1 GB, self-contained: engine +
pak inside the bundle). Launch it windowed from a terminal:

```bash
Saved/Packaged/Mac/UnderTheMountain.app/Contents/MacOS/UnderTheMountain -windowed
```

The title screen opens first (world paused) — press **Start Run**, same as
in-editor play. Fresh launch reaches gameplay (verified: title → playing,
thieves stealing, newest-clip flight states, clean quit); measured on this
Mac (M1 Max, 1280×720 windowed): ~59 fps lair, ~56 fps village — just under
the 60 fps gate, so a dedicated performance pass is on the board (T-115);
per the owner call 2026-11-04 the game ships at authored quality until then.
**Do not launch the packaged app fullscreen / without `-windowed` yet:**
the first no-flags launch wedged in the macOS fullscreen transition (sampled
stuck in `FMacWindow::UpdateFullScreenState`); windowed is the supported
path until the perf pass looks at it.
**Do not launch the loose `Binaries/Mac/` binary directly** — only the
packaged `.app` above is staged (engine + pak + dylib layout, T-057).

Notes:

- **Two ways to play:** the editor (inspect/MCP path) and the packaged
  `.app` above (judge path). `Scripts/play.sh` remains the quick-test path
  for current code without the editor.
- **Restart the editor after any C++ build.** UE does not hot-swap C++; an
  editor left open across builds flies the old pawn (the cause of the
  "wings frozen in flight" report, T-047).
- **Live-editor MCP:** `unreal-mcp` listens on `127.0.0.1:8000/mcp` and
  auto-starts with the editor (`bAutoStartServer` ships in
  `Config/DefaultEditorPerProjectUserSettings.ini` — T-048; it used to
  live only in gitignored `Saved/` state, so a fresh setup silently had
  no listener). **One instance only:** the first editor to launch binds
  the port and owns MCP; `Scripts/run-editor.sh` refuses to launch a
  second editor for this project while one is running, and waits to
  confirm the listener is up. Verify any time with
  `hermes mcp test unreal-mcp` (expect: Connected, 3 tools). If the
  listener is dead, you are almost certainly talking to a second or
  stale instance — quit all project editors and relaunch once.

## What's in the game

### The dragon

- **Model:** newest owner-drop dragon on the 39-bone v6 rig (owner FBX
  sources in `Content/Meshy/dragon-wings-v6(new)/`, T-113), scaled 2×, with
  `M_DragonScalesV6` — body base × scale-tint — and untinted
  `M_DragonWingV6` wing membranes. All 9 clips play on the pawn (fly,
  glide, hover, walk, takeoff, land, idle, roar, hit); the v4/v5 assets
  were retired in the same task.
- **Flight feel:** flap to climb, glide to travel, dive to convert altitude
  into speed; stamina drains per action (flap 3/beat, glide 0/s, hover 8/s,
  fire 5/s), zero stamina burns HP, and hard landings cost HP scaled by impact
  velocity (damage-free under ~800 cm/s).
- **Combat:** fire breath (hold; 18 DPS cone, −5 stamina/s, needs 10 to
  start), roar (−10 stamina, 6 s cooldown; makes fleeing thieves drop loot),
  and eat (2 s channel; a dead thief restores +60 stamina/+15 HP, a goat
  +50/+10).
- **Help:** press **H** (gamepad Back/Select) any time for the control
  overlay — every binding, keyboard + pad columns, generated from the
  shipped scheme.
- **Scale-colour picker** (C): one draggable saturation/value pad plus a
  hue strip in a code-built widget; the tint is clamped, written to the
  material, and persisted to a SaveGame slot.
- **Settings (logic slice, T-128/T-088):** quality presets
  Low/Medium/High/Ultra (default Ultra) mapping to the `sg.*` scalability
  groups, plus a settings model (resolution/fullscreen, master volume,
  look/flight sensitivity, three nose-up invert toggles, camera choice,
  Testing → Infinite Stamina driving `utm.testing.infinitestamina`) with
  apply/persist/defaults in `Public/UI/SettingsModel.h` and a code-built
  screen in `Public/UI/SettingsWidget.h`. Not yet opened from
  title/pause — see Known limitations.

### The world (one map, no load screens)

- **Lair:** a 100 m great chamber under a 25 m open crown, with a hoard mound
  (+30 m X) clear of the 12 m touchdown disc; a dragon-passable grand east
  portal (800×800) and west cave (500×600), plus two 80×220 thief-only cracks
  the dragon cannot fit. The mound visibly shrinks in 3 steps as the hoard
  is stolen (75/50/20% of start). Built headless —
  293 actors at creation (chamber shell, slopes, crown ring, gold/rock/wood
  materials, 13 point lights, a cool-glow light at the crown, a sun + sky
  outside), with the exterior village set added afterwards.
- **Exterior:** the grand portal exits onto a rock plateau, terraces step down east
  into a valley holding a real Meshy village — 5 houses (3 distinct house
  types), inn, castle tower, well, market stall, cart, crates, barrels, fence
  runs, 4 vista rocks and 4 pines — with **5 goats** wandering the pasture.
  The plateau and terraces are still rock boxes; the Landscape terrain that
  makes the world feel vast is owed (T-049).

### Treasure and scent (the signature mechanic)

Four treasures are registered in the lair — gold goblet (10), jewelled cup
(50), adventurer's chest (250), and the Seeing Stone artifact (1200). Every
registered treasure emits a **golden Niagara ribbon** (NS_ScentRibbon)
sweeping between the dragon's nose and the treasure. Ribbon thickness
encodes value only — 4/8/14/22/32 cm brackets — and *never* distance or room:
patrolling is the sensor. A stolen item's ribbon follows the thief and pulses.
Toggle **true-sight** (T) with the Seeing Stone: ribbons dim to 30% and a
teal through-rock silhouette marker appears for the treasure.

### Thieves

Three thief types are live in the lair on their Meshy models:

- **Burglar** (20 HP, quiet, fastest solo) — grabs one item, flees to the exit.
- **Dwarf party** (45 HP, slow, organised) — the carrier grabs up to 3 items;
  the party places traps that slow the dragon to half speed.
- **Adventurer** (80 HP, armed silhouette) — grabs 2, the scare-able type.

Their brain is a data-driven state machine (Sneak → Grab → Flee, with
Scared/Trapped/Dead interrupts); type is a DataAsset (mesh, anims, HP,
speeds), not a subclass. Roaring a fleeing thief makes it drop loot; fire
kills it; eat finishes the loop. Waves, Notoriety stars, and win/lose are
live (T-016); star-driven gear scaling remains specced in
`docs/GAMEPLAY.md` §4.

## Controls

Both inputs are first-class (code-built Enhanced Input actions + mapping
context):

| Action | Keyboard/mouse | Gamepad |
|---|---|---|
| Move (S flares at speed, A/D turns at speed) | W A S D | Left stick |
| Look (aims the nose in flight) | Mouse | Right stick |
| Takeoff / climb (Ascent; hold to rise in hover) | Space | Bottom face |
| Landing Assist (air) / Tail Slam (ground) | F | Top face |
| Brake (S also flares at speed) | Left Shift (hold) | Left trigger (hold) |
| Surge (momentum band) | W tap | Right trigger |
| Sink in hover | Left Ctrl (hold) | Right face hold (air) |
| Free-look (nose frozen, camera roams) | Right mouse (hold) | — (right stick already looks) |
| Steer-to-look (latch; any move input cancels) | Double-click right mouse | D-pad Up |
| Cycle camera (head chase / wide scenic / first-person) | V | R3 (right-stick press) |
| True-sight (Seeing Stone) | T | Left face (X) |
| Scent ribbons show/hide | N | D-pad Down |
| Scale-colour picker | C | Right face tap (no claw target) |
| Fire breath (hold) | Left mouse (hold) | Right shoulder (hold) |
| Roar | R | Left shoulder |
| Eat / pickup-drop (context) | E / P | Right face hold (ground) |
| Claw | Q | Right face tap (claw target in arc) |
| Smart lock cycle | Tab | L3 (left-stick press) |

## Technical features

- **Native Apple Silicon.** UE 5.8.4 source build, Metal rendering. The 60 fps
  contract is measured, not hoped: standalone `-game` CSV captures on the M1
  Max average 16.81 ms/frame in the lair and 16.12 ms over the village
  (`Saved/Profiling/*.csv`).
- **Testable by design.** One gameplay system per C++ class with dependencies
  injected at seams, so the **52 automation tests** (21 files under
  `Source/UnderTheMountain/Tests/`, counted by `IMPLEMENT_*_AUTOMATION_TEST`)
  run headless and hermetic — no live level, no network, no real clock.
- **Real line coverage.** `Scripts/coverage.sh` measures llvm-cov on the game
  module (module-only rsp-relink instrumentation); `gate.sh` enforces the
  37% floor (AGENTS.md snapshot, the single owner of the number) before
  every commit. Run `Scripts/coverage.sh` for the current measured number;
  this README does not carry a stale one.
- **Headless, re-runnable world generation.** The lair, exterior, treasures,
  thief visuals, flight animation, and the whole 28-model world import are
  built by deterministic UE-Python scripts in `Scripts/` — no per-file import
  dialogs, everything regenerable (T-046).
- **Agentic asset pipeline.** Concept image (Muse Image) → owner approval →
  Meshy image-to-3D (smart topology, budgeted) → rig verdict → download to
  `Content/Meshy/Raw/` (raw, never hand-edited) → Blender 5.2 fixup where
  auto-rig fails → headless import. Per-asset reports: `docs/THIEF-ASSETS.md`,
  `docs/WORLD-ASSETS.md`; procedure: `docs/ASSET-PIPELINE.md`.
- **Data-driven content.** Thief types are DataAssets; treasure values are
  DataAssets read through `UTreasureSubsystem` — the scent system never holds
  actor references.
- **Niagara scent ribbons** — location-based ribbon emitter (240 particles/s,
  0.6 s lifetime); the actor sweeps the emitter nose↔treasure, so the sweep
  *is* the ribbon, and the width pass-through is tested.

## Project structure

```
under-the-mountain/
├── UnderTheMountain.uproject         UE 5.8.4, module + Python/MCP plugins
├── Source/UnderTheMountain/          C++ game module
│   ├── Core/         GameMode (map + spawn wiring)
│   ├── Dragon/       character, stats, combat, appearance (scale tint)
│   ├── Thief/        brain (state machine), character, DataAsset, traps
│   ├── Livestock/    goat actor (wander, HP, death, eat)
│   ├── Scent/        scent math, ribbon actor, true-sight markers
│   ├── Treasure/     DataAsset, registry, WorldSubsystem, treasure actor
│   ├── UI/           code-built widgets (HUD, tint, game flow, settings model/screen)
│   └── Tests/        52 hermetic automation tests (21 files)
├── Content/
│   ├── Maps/Lair/              Lair.umap — lair + exterior, one map
│   ├── Meshy/Dragon/           imported dragon + scales material + evidence
│   ├── Meshy/Thieves/          5 rigged humanoid models + walk/run clips
│   ├── Meshy/World/            28 imported models (27 statics + goat SkeletalMesh)
│   ├── Meshy/Raw/              pristine Meshy downloads — never hand-edit
│   ├── Gameplay/               DataAssets (treasures, thief types),
│   │                           NS_ScentRibbon, scent/true-sight materials
│   └── Dragon/                 M_FireBreath
├── Scripts/
│   ├── setup.sh build.sh run-editor.sh test-fast.sh gate.sh coverage.sh
│   ├── create_*.py             headless generators (lair, exterior, thieves,
│   │                           treasures, dragon material, flight anim, …)
│   └── import_*.py, link_thief_skeletons.py   headless import + skeleton link
├── Config/                   engine / game / input ini
├── docs/
│   ├── PLAN.md                 normative plan (phases M1–M5, gates)
│   ├── GAMEPLAY.md             full gameplay spec (every number, tagged)
│   ├── ASSET-PIPELINE.md       Meshy→Unreal procedure + dragon gate + credit budget
│   ├── FLIGHT-SPIKE.md         T-075 flight model findings + adopted decisions
│   ├── ANIMATION-SPIKE.md      T-067 animation architecture + adopted v1 set
│   ├── ART-DIRECTION.md        Style B decision
│   ├── THIEF-ASSETS.md         thief generation report (task IDs, verdicts)
│   ├── WORLD-ASSETS.md         world pack report (27 models + goat rig)
│   └── concepts/               owner-approved concept art
├── AGENTS.md / TASKS.md / goose-deuce.md / CHANGELOG.md   process files
├── HERMES-OVERLAY.md / LOOP.md                       agent session overlays
└── idea.md                   original design vision (verbatim brain-dump kept)
```

## Models: rig and animation status

| Model | Source | Rig | Has | Missing |
|---|---|---|---|---|
| **Dragon** (`SK_Dragon_v6`, newest owner drop, T-113) | Owner-supplied FBX (mesh + 9 anims + 6 4K PBR textures, `Content/Meshy/dragon-wings-v6(new)/`) → Blender inventory/merge fixup → Unreal | 39-bone rig (spine chain, 4 leg chains, 7 wing bones/side + finger fan, tail), skin weights intact through decimation | All 9 clips on the pawn's paths (`SK_Dragon_v4_*` names, v6 content): walk, fly (flap; 1.6× in dive), glide (wings held), hover, takeoff, land, idle, roar, hit-light — driven by the anim state machine; transitions log `ANIMSTATE` on stdout in headless -game runs | No hit-heavy/defeat/sleep/fire-pose/claw clips (T-068 remainder); live flight feel + wing motion in a human PIE session is owed (T-030) |
| **Burglar, Adventurer, Dwarf** (5 models; the dwarf *party* in-game uses the carrier model; guard/trapper models are in the repo unused) | Meshy image-to-3D + auto-rig | 24-joint humanoid, verified usable (`glb_stats` skinned-mesh check) | Walk + run (Meshy clips, single-node, switched by AI state — T-044) | No death/fall, hit reaction, grab/carry, or trap-placement clips — all owed in T-054 |
| **Goat** (`liv-goat_rigged`) | Meshy → Blender 5.2 | 21-bone quadruped (region-gated distance weighting after Bone Heat failed on the mesh) | In game (T-054): skeletal mesh + walk loop on `AGoatActor`, wandering the pasture | No run clip; the Meshy biped auto-rig was rejected and is flagged in `docs/WORLD-ASSETS.md` §5 — do not ship it |
| **27 world statics** (+ goat skeletal mesh) | Meshy image-to-3D | n/a (static meshes) | 15 distinct assets placed in the current village layout (T-046) | 11 statics imported but **not placed**: castle keep/wall/gate, the destructible props (incl. the 4 intact→rubble pairs), 2 dead trees (T-050/T-055); no LODs or collision yet |

## Assets that still need to be generated / built

| Owed | What it is | Task |
|---|---|---|
| Fire-breath VFX | Currently an orange cone mesh. Owed: pooled Niagara fire stream from the mouth, dynamic point light while firing, heat shimmer; cone reduced to invisible collision role | T-051 |
| Sky + weather | No sun/skyline/clouds in the exterior. Owed: SkyAtmosphere + directional sun, cloud layer, clear/rain states switchable in PIE | T-052 |
| Landscape terrain | The exterior is box-built. Owed: heightmap Landscape ring around the mountain (km-scale ridges/valleys/fields), minutes of flight, no visible edge | T-049 |
| More settlements | At least 2 castle sites, 3 distinct villages, 4+ farmsteads placed deterministically on the T-049 terrain | T-050 |
| Human NPC villagers | Meshy humanoids (rig decision to be recorded) that wander and flee in settlements; more goat herds | T-053 |
| Enemy/unit anim sets | Thief death fall + hit reaction; goat walk/run import + death fall | T-054 |
| Ground fire | Persistent scorch decal; wooden props ignite, burn, and permanently swap to their rubble counterparts (rubble assets already exist) | T-055 |
| Music + SFX | Friend-composed music masters are incoming before the deadline (never generated); ingest untouched masters under `Content/Audio/Raw/Music/`, working loops under `Content/Audio/Music/`, then source dragon/world/UI SFX and ambience | T-071 |
| Game systems (not assets) | Settings screen wiring into title/pause + per-preset fps proof (T-128/T-088 logic slice is in the tree; reachability/proof are not), audio (T-071) | see TASKS.md |

## Known limitations (honest list)

- **No audio yet** (T-071): no `Content/Audio/` in the tree; friend-composed
  music has not landed.
- **Settings screen is a logic slice, not yet reachable in play**: the
  T-128 preset table and T-088 model/widget are in the tree with tests,
  but title/pause do not open the screen yet and per-preset fps has not
  been measured (T-128/T-115 own the proof).
- **No packaged `.app` in a fresh clone**: `Saved/` is gitignored; build
  one with `Scripts/package.sh` (T-020 proved the path; T-129/T-130 own
  the final freeze + double-click smoke).
- Fire VFX, sky/weather, and Landscape terrain remain as tabled above
  where their tasks are still open — check `TASKS.md`, not this list,
  for current status.
- **Live flight feel is unverified in a human PIE session** (T-030, blocked on
  a fresh editor + working MCP listener — see T-048). Every testable layer of
  the flight animation passes automation tests; the Output Log now prints each
  anim-state change so a live flight proves itself.
- **True-sight (T) is bound but has never been exercised by a live key press**
  — the binding follows the proven input pattern; the live input check rides
  on the same T-030 session.
- **MCP listener reliability**: repeatedly dead when a long-lived or second
  editor instance is up (T-048); restart the editor on the current build
  before live-editor work.
