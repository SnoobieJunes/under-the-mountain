// Copyright Under the Mountain contributors.
// Purpose: witness-based notoriety (T-134) — the pure rule replacing
// "outside the lair box = charged" with "charged only if someone could
// have seen it", per GAMEPLAY.md S2/S4. Created 2026-11-04 (T-134).
// Invariants: an act inside the lair is never witnessed-charged (the
// T-016 zero invariant, kept); an outside act with no settlement and
// no villager witness is free — unwitnessed wilderness is not a crime
// scene. Line-of-sight arrives as an injected flag: this file never
// traces, so tests decide sight, the pawn decides geometry.

#pragma once

#include "CoreMinimal.h"

// [2026-11-04 T-134] One potential witness as the pawn sees them: where
// they stand and whether they had line-of-sight to the act.
struct UNDERTHEMOUNTAIN_API FVillagerWitness
{
	FVector Location = FVector::ZeroVector;
	bool bHasLineOfSight = false;
};

// [2026-11-04 T-134] A settlement footprint. Acts inside one are
// witnessed by definition (streets, windows, the well) — no per-villager
// LOS roll is needed to charge a dragon that eats in the village square.
struct UNDERTHEMOUNTAIN_API FSettlementArea
{
	FVector Center = FVector::ZeroVector;
	float Radius = 0.0f;
};

// [2026-11-04 T-134] Acts GAMEPLAY.md S4 prices. Anything not listed
// here (roars, structure hits) keeps its existing verb and rule.
enum class EWitnessedAct : uint8
{
	PersonKill,
	LivestockEat,
};

struct UNDERTHEMOUNTAIN_API FWitnessRules
{
	// [2026-11-04 T-134] How far a villager can witness an act, cm
	// [tuning: PT-WAVE]: past VillagerBrain's 1500 flee radius (they saw
	// it coming) and short of scent ranges, so fleeing witnesses still count.
	static constexpr float DefaultWitnessRadius = 2500.0f;

	static bool IsWitnessed(const FVector& ActLocation, bool bInsideLair,
							const TArray<FSettlementArea>& Settlements,
							const TArray<FVillagerWitness>& Villagers, float WitnessRadius);

	// [2026-11-04 T-134] S4 prices, witness-gated: person 10, livestock
	// 3, and exactly 0 inside the lair or unwitnessed outside.
	static int32 PointsForAct(EWitnessedAct Act, bool bWitnessed, bool bInsideLair);
};
