// Copyright Under the Mountain contributors.
// [2026-11-04 T-134] FWitnessRules implementation.

#include "Notoriety/WitnessRules.h"

bool FWitnessRules::IsWitnessed(const FVector& ActLocation, bool bInsideLair,
								const TArray<FSettlementArea>& Settlements,
								const TArray<FVillagerWitness>& Villagers, float WitnessRadius)
{
	if (bInsideLair)
	{
		return false;
	}
	for (const FSettlementArea& Settlement : Settlements)
	{
		if (Settlement.Radius > 0.0f &&
			FVector::Dist2D(ActLocation, Settlement.Center) <= Settlement.Radius)
		{
			return true;
		}
	}
	const float Radius = FMath::Max(0.0f, WitnessRadius);
	for (const FVillagerWitness& Villager : Villagers)
	{
		if (Villager.bHasLineOfSight && FVector::Dist2D(ActLocation, Villager.Location) <= Radius)
		{
			return true;
		}
	}
	return false;
}

int32 FWitnessRules::PointsForAct(EWitnessedAct Act, bool bWitnessed, bool bInsideLair)
{
	if (bInsideLair || !bWitnessed)
	{
		return 0;
	}
	// [2026-11-04 T-134] GAMEPLAY.md S4 prices, unchanged — only the gate moved.
	return Act == EWitnessedAct::PersonKill ? 10 : 3;
}
