# SUBMISSION.md — Under the Mountain (draft, 2026-11-04, T-129/T-021)

Status: draft based on TASKS.md Done at 9305b83. Final package, capture and
checksum are T-129/T-130 and are **not** claimed here.

## The game
You are the dragon. Guard your hoard, smell treasure, manage notoriety.
An open-world dragon simulator for Mac (Apple Silicon): wake on your hoard,
follow scent ribbons to what is yours, drive off (or eat) the thieves who
come for it, and decide how infamous you can afford to become.

## Pillars
1. Scent is the compass — ribbons encode value and carried state, never location.
2. Notoriety is a ratchet with a price — public kills/roars raise the stars;
   the lair is private, the world remembers.
3. Three pools, one body — HP, stamina and (specced) magic priced off one scale.
4. The hoard is score **and** loadout — wearable artifacts are treasure a
   thief can carry out; an effect dies the tick its item leaves.

## Stack
Unreal Engine 5.8.4 (source build), C++ gameplay behind testable seams
(plain-struct pure rules + Unreal Automation tests), Blueprint-facing
components, Meshy-generated world/dragon assets with Blender fixup,
Niagara fire/rain, macOS/Metal on Apple Silicon (M1 Max development machine).

## Controls
Keyboard/mouse and gamepad are both first-class (see README.md "Controls"
for the full table): WASD/stick move, mouse/right-stick look, Space takeoff,
LMB/left-trigger fire (hold), R/left-shoulder roar, E/right-shoulder eat,
T true-sight, N scent toggle, V camera, C scale-colour picker.

## Credits
- Created & Designed by Auston Leroy.
- Music: original compositions by **Keith Vernon-Hutchinson, Hutchinson
  Music, PRS/MCPS/PPL-UK** — *Desert Winds, The Ancient One, Ryze of the
  Darkness, Drooon & Drooon Dark Wizard, Vikings Sounds & Samples* —
  for use only in Under the Mountain.
- 3D assets: Meshy (see docs/ASSET-PIPELINE.md / Content/Meshy provenance);
  engine: Unreal Engine 5.8.4 (Epic Games).

## Known limits (honest, 2026-11-04)
- Friend-music masters are not yet ingested; the audio director seam, cue
  names and slots are in code, the assigned sound assets are editor-side.
- Artifact effects: Seeing Stone, Hardened Scales, Enhanced Claws, Ring of
  Power and Flame Ring have code rules; Magic Pendant, Mind-Control Crown,
  Elemental Rings, Treasure-Sight Helm, Strength Armor and Thief-Glow are
  data-only named items (GAMEPLAY.md S6) with no effect code yet.
- GAMEPLAY DataAsset migration is code-side only: config structs pin the
  shipped literals; DA_*/DT_* assets still need editor authoring.
- Magic pool (T-080), settings screen (T-088), save system (T-087) and the
  final packaged freeze/upload (T-129/T-130) are open tasks, not shipped.
- Windows is a non-goal for this hackathon build (docs/PLAN.md).
