# Under the Mountain

Open-world dragon simulator for macOS (Apple Silicon). You are the dragon:
guard your hoard, smell treasure you cannot locate precisely, and decide —
every time — whether a thief is worth the Notoriety. Meta AI Developer
Hackathon 2026, Indie track. Design: `idea.md`. Normative plan: `docs/PLAN.md`.
Process: `AGENTS.md`.

## Status

Scaffolding (T-000/T-001 done). No playable build yet — the Unreal project and
toolchain gate arrive in T-002, the walking skeleton in T-003.

## Install / run / test

Engine: Unreal Engine 5.8.4 source build at
`/Users/auston/Development Projects/UnrealEngine` (Xcode 27.1, macOS, M1 Max
reference machine). Commands below arrive in T-002 and are not real until then:

```bash
Scripts/setup.sh       # arrives in T-002 — verify engine + toolchain
Scripts/run-editor.sh  # arrives in T-002 — open the project in the editor
Scripts/test-fast.sh   # arrives in T-002 — inner-loop tests
Scripts/gate.sh        # arrives in T-002 — full pre-commit gate
Scripts/build.sh       # arrives in T-002 — packaged macOS build
```

## Assets

Models are generated with Meshy, imported per `docs/ASSET-PIPELINE.md`
(image approval → 3D → remesh → rig → FBX/GLB → `Content/Meshy/`).
