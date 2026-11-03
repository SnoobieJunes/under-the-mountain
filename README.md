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

## Status (2026-10-31)

Playable in the editor: a textured, rigged dragon with flight animation in a
dressed mountain lair (3 chambers + hoard room + peak shaft), the signature
scent system (Niagara ribbons, value-only), a scale-colour picker, and live
combat (fire breath, roar, eat). Three Meshy-modelled thief types (burglar,
dwarf party, adventurer) run a data-driven AI state machine in the lair, and
the peak shaft flies out onto a real Meshy village with goats in the valley —
all one map, no loading, measured at ~60 fps on the M1 Max.

Still owed (see `TASKS.md`): the full scoped board now runs through T-092 —
flight/animation overhauls (decisions in `docs/FLIGHT-SPIKE.md` /
`docs/ANIMATION-SPIKE.md`), a real fire VFX, sky/weather/day-night, sprawling
terrain and settlements, villagers/wildlife, artifacts and bulk treasure,
audio ingestion for friend-composed music plus SFX, Notoriety/waves/win-lose,
intro, HUD/shell, navigation, saves/settings, the standalone play fix, and the
packaged macOS build. No scope has been cut.

## Install / run / test

Engine: Unreal Engine 5.8.4 source build at
`/Users/auston/Development Projects/UnrealEngine` (Xcode 27.1, macOS, M1 Max
reference machine). Everything runs from the repo root:

```bash
Scripts/setup.sh       # verify engine + toolchain (UE 5.8.4, Xcode 27.1)
Scripts/build.sh       # build the editor target (compiles the game module)
Scripts/run-editor.sh  # open the editor; pass /Game/Maps/Lair/Lair to open the lair
Scripts/play.sh        # quick test: build the game target + standalone -game window (no editor)
Scripts/test-fast.sh   # build + automation tests (inner loop)
Scripts/gate.sh        # full pre-commit gate: setup, editor+game builds,
                       # clang-format check, tests, coverage-floor check
Scripts/coverage.sh    # real llvm-cov line coverage (floor: AGENTS.md, 29%)
```

To play: open the editor, load `/Game/Maps/Lair/Lair`, press **Play**. The
dragon spawns at `DragonStart` in the entry chamber; the treasure rooms, the
peak shaft, and the village are all reachable from there.

**Quick test (no editor): `Scripts/play.sh`** — builds the game target and
launches a standalone game window on the Lair running exactly the code you
just built. Use this to test current code; use the editor when you need to
inspect or change the level/assets, or drive it over MCP.

**Do not launch `Binaries/Mac/UnderTheMountain.app` (or the loose
`Binaries/Mac/UnderTheMountain` binary) directly** — that path is retired
until the packaged build (T-020) stages it properly. Root cause, proven
in T-057: an unstaged Development game binary launched directly cannot
establish its project/engine roots (the descriptor open fails inside the
engine's file layer in every invocation form), so it finds no project,
then no ICU data (which lives under the engine tree), asserts in
`FICUInternationalization::Initialize`, and segfaults on the Mac error
path; the loose binary additionally expects a staged `@rpath` dylib
layout that only packaging creates. `Scripts/play.sh` is the supported
way to run the built game today.

Notes:

- **The editor is the only play path for now.** A packaged `.app` arrives in
  T-020, not here.
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

- **Model:** Meshy V4 hero dragon (fourth-generation Meshy iteration, owner-
  approved concept art as source per `docs/ART-DIRECTION.md`) on the pawn,
  scaled 2×, with `M_DragonScales` — base × scale-tint, with the crimson
  membranes/spines protected in the material graph.
- **Flight feel:** flap to climb, glide to travel, dive to convert altitude
  into speed; stamina drains per action (flap 3/beat, glide 1/s, hover 8/s,
  fire 5/s), zero stamina burns HP, and hard landings cost HP scaled by impact
  velocity (damage-free under ~800 cm/s).
- **Combat:** fire breath (hold; 18 DPS cone, −5 stamina/s, needs 10 to
  start), roar (−10 stamina, 6 s cooldown; makes fleeing thieves drop loot),
  and eat (2 s channel; a dead thief restores +60 stamina/+15 HP, a goat
  +50/+10).
- **Scale-colour picker** (C): one draggable saturation/value pad plus a
  hue strip in a code-built widget; the tint is clamped, written to the
  material, and persisted to a SaveGame slot.

### The world (one map, no load screens)

- **Lair:** entry chamber → chamber 2 → chamber 3 → hoard room (40 m × 40 m)
  with a 7 m × 7 m peak shaft rising 40 m to the summit. Built headless —
  203 actors at creation (190 static meshes, gold/rock/wood materials, 10
  point lights, a cool-glow light up the shaft, a sun + sky outside), with the
  exterior village set added afterwards.
- **Exterior:** the shaft exits onto a rock plateau, terraces step down east
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
kills it; eat finishes the loop. (Waves, Notoriety stars, and the star-driven
gear scaling are specced in `docs/GAMEPLAY.md` §4 and arrive in T-016.)

## Controls

Both inputs are first-class (code-built Enhanced Input actions + mapping
context):

| Action | Keyboard/mouse | Gamepad |
|---|---|---|
| Move | W A S D | Left stick |
| Look | Mouse | Right stick |
| Flap (climb) | Space | Bottom face (B) |
| Toggle flight | F | Top face (Y) |
| Dive (speed) | Left Shift (hold) | Right trigger (hold) |
| True-sight (Seeing Stone) | T | Left face (X) |
| Scent ribbons show/hide | N | D-pad Down |
| Scale-colour picker | C | Right face (A) |
| Fire breath (hold) | Left mouse (hold) | Left trigger (hold) |
| Roar | R | Left shoulder |
| Eat | E | Right shoulder |

## Technical features

- **Native Apple Silicon.** UE 5.8.4 source build, Metal rendering. The 60 fps
  contract is measured, not hoped: standalone `-game` CSV captures on the M1
  Max average 16.81 ms/frame in the lair and 16.12 ms over the village
  (`Saved/Profiling/*.csv`).
- **Testable by design.** One gameplay system per C++ class with dependencies
  injected at seams, so the **20 automation tests** (8 files under
  `Source/UnderTheMountain/Tests/`) run headless and hermetic — no live
  level, no network, no real clock.
- **Real line coverage.** `Scripts/coverage.sh` measures llvm-cov on the game
  module (module-only rsp-relink instrumentation); `gate.sh` enforces the
  29% floor (AGENTS.md snapshot) before every commit. Latest measured: 43.5%
  (2026-10-28, T-047).
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
│   ├── UI/           code-built tint-picker widget
│   └── Tests/        20 hermetic automation tests (8 files)
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
| **Dragon** (`SK_Dragon_v4`) | Muse multi-images → Meshy multi-image-to-3D mesh/textures → Blender rig/skin/Walk/Fly → Unreal | Blender-authored 33-bone quadruped (spine, 4 leg chains, wing chains, tail) with bone-heat skinning | Quadruped walk cycle (`SK_Dragon_v4_Anim`); flight anim (`SK_Dragon_v4_FlyAnim`: spread + flap with downstream lag, body bob) driven by an anim state machine — Idle / Walk / Fly (flap; 1.6× in dive) / Glide (0.25×, wings held). Transitions are logged in the Output Log (T-047) | No attack/roar/eat/landing poses (combat plays a cone mesh, no pose change); live flight feel + wing motion in a human PIE session is owed (T-030) |
| **Burglar, Adventurer, Dwarf** (5 models; the dwarf *party* in-game uses the carrier model; guard/trapper models are in the repo unused) | Meshy image-to-3D + auto-rig | 24-joint humanoid, verified usable (`glb_stats` skinned-mesh check) | Walk + run (Meshy clips, single-node, switched by AI state — T-044) | No death/fall, hit reaction, grab/carry, or trap-placement clips — all owed in T-054 |
| **Goat** (`liv-goat_quad`) | Meshy → Blender 5.2 | 21-bone quadruped (region-gated distance weighting after Bone Heat failed on the mesh) | 24-frame in-place walk loop authored in Blender (verified in renders) | **Not in the game yet**: in-game goats are greybox sphere/cube (T-017); the quad clip is not imported and no run/death clips exist (T-054). The Meshy biped auto-rig was rejected and is flagged in `docs/WORLD-ASSETS.md` §5 — do not ship it |
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
| Game systems (not assets) | Notoriety stars + waves + win/lose (T-016), intro gnome choice (T-018), HUD + menus + gamepad parity (T-019), packaged macOS `.app` + perf gate (T-020) | M1 exit gate |

## Known limitations (honest list)

- **Editor-only play** until the T-020 packaged build lands.
- Fire is a **cone mesh**, not particles (T-051).
- Goats are **greybox spheres** in-game; the real rigged goat sits on disk
  unimported (T-054).
- Thief animations are **walk/run only** (T-054).
- **No HUD, menus, Notoriety, waves, intro, win/lose, or audio yet**
  (T-016–T-019, T-071) — the game is currently a sandbox of the M1 pillars:
  flight, scent, thieves, combat. Friend-composed music is incoming; it has
  not landed yet.
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
