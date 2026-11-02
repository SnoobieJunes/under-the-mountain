// Copyright Under the Mountain contributors.
// Purpose: headless terrain creation for the huge-world exterior
// (T-049). Unreal's Python API cannot create a Landscape: spawning
// ALandscape in a pythonscript commandlet asserts, and in the GUI it
// yields a LandscapePlaceholder with zero components (both proven
// 2026-11-01, T-049 probes). ALandscapeProxy::Import - the import
// wizard's own entry point - is C++ only, so the terrain generator
// lives here as an editor commandlet in the editor module (an
// editor-only UCLASS inside the runtime module breaks the game
// target's UHT pass, also proven in T-049):
//
//   UnrealEditor-Cmd <project> -run=Terrain
//   (or: python3 Scripts/create_world_terrain.py)
//
// Created 2026-11-01 (T-049).
// Invariants: deterministic (closed-form height function of world
// coordinates, no RNG), re-runnable (a prior Terrain_World actor is
// destroyed and rebuilt identically), and the Lair interior is never
// touched - the height function is masked below the chamber floors
// in the lair corridor (see TerrainHeightCm).

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "TerrainCommandlet.generated.h"

UCLASS()
class UNDERTHEMOUNTAINEDITOR_API UTerrainCommandlet : public UCommandlet
{
	GENERATED_BODY()

  public:
	UTerrainCommandlet();

	// [2026-11-01 T-049] Loads Lair, rebuilds the Terrain_World
	// landscape from TerrainHeightCm, saves the map, logs
	// TERRAIN_DONE with the actor bounds so a caller can verify the
	// world-space height range without opening the editor.
	virtual int32 Main(const FString& Params) override;
};
