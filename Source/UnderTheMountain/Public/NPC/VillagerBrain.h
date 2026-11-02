// Copyright Under the Mountain contributors.
// Purpose: villager behaviour rules (T-053) - the pure, testable
// half of the settlement NPCs, mirroring UThiefBrain's pattern of
// keeping every decision in a world-free object the tests can drive
// directly. Villagers wander/idle around home and flee the dragon;
// they carry nothing, grab nothing, and have no HP - they are
// witnesses and life, not combatants (T-016 owns what their seeing
// means for Notoriety).
// Created 2026-11-01 (T-053).
// Invariants: state is a pure function of (dragon distance, tuning)
// - the actor owns timers and steering, the brain owns decisions,
// so Dragon.Villager tests prove the shipped rules verbatim.

#pragma once

#include "CoreMinimal.h"
#include "VillagerBrain.generated.h"

// [2026-11-01 T-053] Plain enum class (no UENUM), mirroring
// EThiefState's choice: pure logic types stay reflection-free.
enum class EVillagerState : uint8
{
	Idle,
	Wander,
	Flee
};

UCLASS()
class UNDERTHEMOUNTAIN_API UVillagerBrain : public UObject
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-053] Tuning. FleeRadius 1500 cm: a dragon
	// inside that bubble (landed among the houses, or heard - the
	// roar radius is 1200, so this also covers "roared nearby"
	// with margin) empties the street. WalkSpeed is a stroll;
	// FleeSpeed outruns the dragon's ground walk (600) never, but
	// villagers flee to scatter, not to win a race.
	UPROPERTY(EditAnywhere, Category = "NPC|Villager")
	float FleeRadius = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Villager")
	float WalkSpeed = 90.0f;

	UPROPERTY(EditAnywhere, Category = "NPC|Villager")
	float FleeSpeed = 340.0f;

	// [2026-11-01 T-053] The threat rule: at or inside FleeRadius
	// the villager flees; outside it, daily life (the actor picks
	// Idle vs Wander by its own timer). Negative distance (a
	// caller error) reads as point-blank: flee, never idle.
	static EVillagerState StateForThreat(float DragonDistance, float InFleeRadius);

	// [2026-11-01 T-053] Flat direction away from the dragon
	// (Z never flees). Coincident positions return +X: any fixed
	// answer beats a NaN heading at the one moment it matters.
	static FVector FleeDirection(const FVector& VillagerLocation, const FVector& DragonLocation);

	// [2026-11-01 T-053] Ground speed per state: Idle stands,
	// Wander strolls, Flee runs. Negative tuning clamps to 0.
	static float SpeedForState(EVillagerState State, float InWalkSpeed, float InFleeSpeed);
};
