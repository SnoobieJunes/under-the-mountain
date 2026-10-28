// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Stamina implementation. Flight drain is driven from the
// pawn (it knows the movement mode); regen likewise, so TickComponent stays
// empty — a component that guesses state it doesn't own was rejected.

#include "Dragon/DragonStatsComponent.h"

UDragonStatsComponent::UDragonStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Stamina = MaxStamina;
}

bool UDragonStatsComponent::TryConsumeStamina(float Cost)
{
	if (Cost < 0.0f || Stamina < Cost)
	{
		return false;
	}
	Stamina -= Cost;
	return true;
}

void UDragonStatsComponent::ApplyStaminaDelta(float Delta)
{
	Stamina = FMath::Clamp(Stamina + Delta, 0.0f, MaxStamina);
}

void UDragonStatsComponent::DrainForFlight(float DeltaSeconds)
{
	ApplyStaminaDelta(-FlyingDrainPerSecond * FMath::Max(0.0f, DeltaSeconds));
}

void UDragonStatsComponent::RegenOnGround(float DeltaSeconds)
{
	ApplyStaminaDelta(GroundRegenPerSecond * FMath::Max(0.0f, DeltaSeconds));
}

bool UDragonStatsComponent::CanFlap() const
{
	return Stamina >= FlapStaminaCost;
}

void UDragonStatsComponent::TickComponent(float DeltaTime, ELevelTick TickType,
										  FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}
