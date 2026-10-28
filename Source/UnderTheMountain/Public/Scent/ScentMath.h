// Copyright Under the Mountain contributors.
// Purpose: the pure scent-ribbon math from docs/GAMEPLAY.md section 3 —
// ribbon width from gold value, true-sight opacity, the carried pulse —
// with no world or actor dependency, so automation tests exercise it
// directly. Created 2026-10-28 (T-013).
// Invariants: the no-information rule. Nothing in this file reads an
// entry's location or any distance, so ribbon appearance can never
// leak where a treasure is, only what it is worth and whether it is
// carried. Width is strictly increasing across value brackets.

#pragma once

#include "CoreMinimal.h"
#include "Treasure/TreasureRegistry.h"

// [2026-10-28 T-013] Everything a ribbon actor needs for one treasure
// for one tick. Computed by FScentMath::ParamsForEntry from value,
// carried state and true-sight state only — never from position.
struct UNDERTHEMOUNTAIN_API FScentRibbonParams
{
	float Width = 0.0f;
	float Opacity = 1.0f;
	bool bCarried = false;
};

// [2026-10-28 T-013] Namespace-as-struct of static pure functions
// (matching the owner-free contract UDragonStatsComponent uses for its
// math): same inputs, same outputs, no hidden state.
struct UNDERTHEMOUNTAIN_API FScentMath
{
	// [2026-10-28 T-013] Base ribbon width in cm for a gold value, per
	// docs/GAMEPLAY.md section 3 brackets: 1-24 -> 4, 25-99 -> 8,
	// 100-399 -> 14, 400-999 -> 22, 1000+ -> 32, each with a
	// within-bracket micro-variation of W*(0.9 + 0.2*(v-lo)/(hi-lo))
	// so individual pieces read as distinct without crossing into the
	// next bracket's range. Values at or below 0 use the trinket
	// bracket floor. The open-ended crown bracket uses hi = 5000 with
	// the fraction clamped to [0, 1], capping growth at 32*1.1 instead
	// of dividing by an unbounded span.
	static float RibbonWidthForValue(int32 GoldValue);

	// [2026-10-28 T-013] True-sight dims the ribbons to 0.3 so the
	// exact markers it reveals stay readable against them; normal
	// smelling is fully opaque at 1.0 (docs/GAMEPLAY.md section 3).
	static float RibbonOpacity(bool bTrueSightActive);

	// [2026-10-28 T-013] A carried (stolen) treasure's ribbon breathes
	// at 0.5 Hz with +/-10% width — the one motion cue the design
	// allows, and it encodes carried state only, never direction or
	// distance. A resting treasure's width is returned unchanged.
	static float PulsedWidth(float BaseWidth, float TimeSeconds, bool bCarried);

	// [2026-10-28 T-013] Full ribbon parameters for one registry entry.
	// Reads Entry.GoldValue and Entry.bCarried only; Entry.Location is
	// deliberately never touched (no-information rule).
	// [2026-10-28 T-041] No colour here: the tarnished-gold scent
	// colour is baked into NS_ScentRibbon at authoring time, so the
	// params carry only width, opacity and carried state.
	static FScentRibbonParams ParamsForEntry(const FTreasureEntry& Entry, float TimeSeconds,
											 bool bTrueSightActive);
};
