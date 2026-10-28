# Under the Mountain

Open-world dragon simulator for macOS (Apple Silicon). You are the dragon:
guard your hoard, smell treasure you cannot locate precisely, and decide —
every time — whether a thief is worth the Notoriety. Meta AI Developer
Hackathon 2026, Indie track. Design: `idea.md`. Normative plan: `docs/PLAN.md`.
Process: `AGENTS.md`.

## Status

Scaffolding + toolchain done (T-000–T-002). No playable build yet — the
walking skeleton (playable dragon in PIE) arrives in T-003.

## Install / run / test

Engine: Unreal Engine 5.8.4 source build at
`/Users/auston/Development Projects/UnrealEngine` (Xcode 27.1, macOS, M1 Max
reference machine). ```bash
Scripts/setup.sh       # verify engine + toolchain (UE 5.8.4, Xcode 27.1)
Scripts/run-editor.sh  # open the project in the editor
Scripts/build.sh       # build the editor target (compiles the game module)
Scripts/test-fast.sh   # build + automation tests (inner loop)
Scripts/gate.sh        # full pre-commit gate: setup, both builds, format, tests
Scripts/coverage.sh    # coverage report (floor 0% until T-022 instruments it)
```
Packaged `.app` for judges arrives in T-020, not here.

## Assets

Models are generated with Meshy, imported per `docs/ASSET-PIPELINE.md`
(image approval → 3D → remesh → rig → FBX/GLB → `Content/Meshy/`).
