// Copyright Under the Mountain contributors.
// [2026-10-27 T-003] Stamina implementation. Flight drain is driven from the
// pawn (it knows the movement mode); regen likewise — a component that
// guesses state it doesn't own was rejected.
// [2026-10-28 T-041] Tick stays disabled in the constructor and there
// is no TickComponent override: the pawn drives every drain/regen call.
// [2026-10-28 T-011] HP pool + landing-damage curve + exhaustion drain
// added under the same contract: all math here, all state knowledge
// (movement mode, impact speed) stays with the pawn.

#include "Dragon/DragonStatsComponent.h"

UDragonStatsComponent::UDragonStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Stamina = MaxStamina;
	// [2026-10-28 T-011] HP starts full, mirroring stamina.
	HP = MaxHP;
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
	// [2026-10-28 T-011] Legacy overload kept verbatim in behaviour
	// (FlyingDrainPerSecond rate) so T-003 tests/callers still hold; new
	// pawn code uses the explicit-rate overload below.
	DrainForFlight(DeltaSeconds, FlyingDrainPerSecond);
}

void UDragonStatsComponent::DrainForFlight(float DeltaSeconds, float RatePerSecond)
{
	// [2026-10-28 T-011] Negative time and non-positive rates (dive) are
	// free by construction: Max() on both, never a regen in disguise.
	ApplyStaminaDelta(-FMath::Max(0.0f, RatePerSecond) * FMath::Max(0.0f, DeltaSeconds));
}

void UDragonStatsComponent::ApplyHPDamage(float Damage)
{
	// [2026-10-28 T-011] Negative damage is not healing — clamp it away
	// so a bad caller cannot top the pool up through the damage verb.
	HP = FMath::Clamp(HP - FMath::Max(0.0f, Damage), 0.0f, MaxHP);
}

void UDragonStatsComponent::RestoreHP(float Amount)
{
	// [2026-10-28 T-015] Max() clamps a negative "heal" away — healing
	// must never become a second damage verb with a sign flip — and
	// the outer Clamp keeps the header invariant ([0, MaxHP]) for an
	// over-generous food value exactly as ApplyHPDamage does for
	// lethal overkill.
	HP = FMath::Clamp(HP + FMath::Max(0.0f, Amount), 0.0f, MaxHP);
}

void UDragonStatsComponent::DrainHPForExhaustion(float DeltaSeconds)
{
	// [2026-10-28 T-011] GAMEPLAY.md section 2: at 0 stamina, continuing a
	// draining action burns HP at ExhaustionHPDrainPerSecond (2/s). The
	// pawn only calls this while a draining action continues, so a free
	// dive never reaches here.
	ApplyHPDamage(ExhaustionHPDrainPerSecond * FMath::Max(0.0f, DeltaSeconds));
}

float UDragonStatsComponent::LandingDamageForImpactSpeed(float Speed) const
{
	// [2026-10-28 T-011] GAMEPLAY.md section 2 hard-landing rule: no
	// damage at or below the threshold; from just above it the minimum
	// applies immediately (any recognised hard landing hurts), then a
	// straight line to the cap. Linear (not a curve asset yet) because
	// PT-STAM has no data to shape a curve against — see the header for
	// why 800/2200 cm/s were picked [tuning: PT-STAM].
	if (Speed <= LandingSpeedThreshold)
	{
		return 0.0f;
	}
	if (LandingSpeedMaxDamage <= LandingSpeedThreshold)
	{
		// [2026-10-28 T-011] Degenerate tuning (max at/under threshold):
		// any above-threshold impact is a max-damage landing rather than
		// dividing by a non-positive span.
		return MaxLandingDamage;
	}
	const float Alpha = FMath::Clamp(
		(Speed - LandingSpeedThreshold) / (LandingSpeedMaxDamage - LandingSpeedThreshold), 0.0f, 1.0f);
	return FMath::Lerp(MinLandingDamage, MaxLandingDamage, Alpha);
}

void UDragonStatsComponent::RegenOnGround(float DeltaSeconds)
{
	ApplyStaminaDelta(GroundRegenPerSecond * FMath::Max(0.0f, DeltaSeconds));
}

bool UDragonStatsComponent::CanFlap() const
{
	return Stamina >= FlapStaminaCost;
}

bool UDragonStatsComponent::ShouldForceFallAtZeroStamina(float RatePerSecond) const
{
	// [2026-10-28 T-036] A dive is deliberately free in GAMEPLAY.md
	// section 2: altitude, not stamina, pays for it. Therefore zero
	// stamina must not convert a dive into a fall or start the HP
	// burn. Glide and hover have positive rates, so continuing either
	// at zero stamina has both consequences.
	return RatePerSecond > 0.0f;
}
