// Copyright Under the Mountain contributors.
// [2026-11-01 T-053] UVillagerBrain implementation: three pure
// rules (threat state, flee heading, per-state speed). No world,
// no timers - AVillagerActor supplies both.

#include "NPC/VillagerBrain.h"

EVillagerState UVillagerBrain::StateForThreat(float DragonDistance, float InFleeRadius)
{
	// [2026-11-01 T-053] Inclusive edge: exactly at the radius is
	// already too close (a dragon at the well is in the village,
	// whatever the arithmetic says). The actor maps the non-flee
	// answer onto its Idle/Wander timer.
	if (DragonDistance <= InFleeRadius)
	{
		return EVillagerState::Flee;
	}
	return EVillagerState::Wander;
}

FVector UVillagerBrain::FleeDirection(const FVector& VillagerLocation, const FVector& DragonLocation)
{
	// [2026-11-01 T-053] Flat away-vector: villagers run along the
	// street, not into the sky, whatever the dragon's altitude.
	FVector Away = VillagerLocation - DragonLocation;
	Away.Z = 0.0f;
	if (Away.IsNearlyZero())
	{
		return FVector(1.0f, 0.0f, 0.0f);
	}
	return Away.GetSafeNormal();
}

float UVillagerBrain::SpeedForState(EVillagerState State, float InWalkSpeed, float InFleeSpeed)
{
	// [2026-11-01 T-053] Negative tuning is a data error; clamping
	// to 0 turns it into a statue the playtest can see, never a
	// villager walking backwards at speed.
	switch (State)
	{
	case EVillagerState::Idle:
		return 0.0f;
	case EVillagerState::Flee:
		return FMath::Max(0.0f, InFleeSpeed);
	case EVillagerState::Wander:
	default:
		return FMath::Max(0.0f, InWalkSpeed);
	}
}
