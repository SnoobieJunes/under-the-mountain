# Under the Mountain

Open-world dragon simulator for macOS (Apple Silicon). You are the dragon:
guard your hoard, smell treasure you cannot locate precisely, and decide —
every time — whether a thief is worth the Notoriety. Meta AI Developer
Hackathon 2026, Indie track. Design: `idea.md`. Normative plan: `docs/PLAN.md`.
Process: `AGENTS.md`.

## Status

Playable in the editor: a textured dragon (walk/flap/dive, stamina, landing
damage) in a dressed lair (3 chambers + hoard room + peak shaft), with the
treasure/scent system live (value-scaled Niagara scent ribbons, Seeing Stone
true-sight) and a scale-colour picker. Thief AI is parked mid-implementation
(T-014, TASKS.md); combat/notoriety/waves and the packaged build are still to
come (T-015–T-020). [Updated 2026-10-28 at the T-004 sweep — the previous
status predated the walking skeleton.]

## Install / run / test

Engine: Unreal Engine 5.8.4 source build at
`/Users/auston/Development Projects/UnrealEngine` (Xcode 27.1, macOS, M1 Max
reference machine). ```bash
Scripts/setup.sh       # verify engine + toolchain (UE 5.8.4, Xcode 27.1)
Scripts/run-editor.sh  # open the project in the editor (pass /Game/Maps/Lair/Lair to open the lair)
Scripts/build.sh       # build the editor target (compiles the game module)
Scripts/test-fast.sh   # build + automation tests (inner loop)
Scripts/gate.sh        # full pre-commit gate: setup, both builds, format, tests, coverage floor
Scripts/coverage.sh    # real llvm-cov line coverage (floor: AGENTS.md snapshot, 29%)
```
In the editor, open `Content/Maps/Lair/Lair` and press Play. Controls:
WASD + mouse (move/look), Space flap, F toggle flight, Left Shift dive,
T true-sight, C scale-colour picker; gamepad equivalents on the left stick,
right stick, face buttons, and right trigger.

Packaged `.app` for judges arrives in T-020, not here.

## Assets

Models are generated with Meshy, imported per `docs/ASSET-PIPELINE.md`
(image approval → 3D → remesh → rig → FBX/GLB → `Content/Meshy/`).
