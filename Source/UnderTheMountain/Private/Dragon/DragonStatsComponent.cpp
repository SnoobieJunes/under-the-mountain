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

// [2026-11-04 T-124] God mode (testable slice of T-088's Testing toggle):
// while on, every stamina cost/drain and the exhaustion HP burn no-op
// (the gates below); HP damage is untouched. Default off; T-088's screen
// keeps the toggle UI, this cvar is the automation/PIE vehicle.
static TAutoConsoleVariable<int32> CVarInfiniteStamina(
	TEXT("utm.testing.infinitestamina"), 0,
	TEXT("Infinite stamina god mode (T-124): 1 disables every stamina cost/drain + " TEXT("the exhaustion HP burn (HP damage still applies); 0 is the shipped economy.")));

bool UDragonStatsComponent::IsInfiniteStaminaEnabled()
{
	return CVarInfiniteStamina.GetValueOnGameThread() != 0;
}

UDragonStatsComponent::UDragonStatsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Stamina = MaxStamina;
	// [2026-10-28 T-011] HP starts full, mirroring stamina.
	HP = MaxHP;
	// [2026-11-04 T-080] Magic starts full, mirroring the other pools.
	Magic = MaxMagic;
}

bool UDragonStatsComponent::TryConsumeStamina(float Cost)
{
	// [2026-11-04 T-124] God mode succeeds the purchase without spending:
	// callers gate their impulse/verb on this bool, so returning true
	// keeps Ascent/Surge/fire working while the pool never depletes.
	if (IsInfiniteStaminaEnabled())
	{
		return true;
	}
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

float UDragonStatsComponent::FlightStaminaRatePerSecond(float Airspeed, bool bHovering)
{
	// [2026-11-04 FLIGHT-VIGOR] Hover outranks speed: a near-stationary
	// dragon is working to stay up at any (near-zero) airspeed. The -3
	// mirrors HoverDrainPerSecond's default (pinned equal in tests);
	// a static cannot read that instance tuning, so the shipped band
	// values live here as the single flight-band definition.
	if (bHovering)
	{
		return -3.0f;
	}
	if (Airspeed < SlowFlightBandMaxSpeed)
	{
		return -1.0f;
	}
	if (Airspeed < VigorRechargeSpeed)
	{
		return 0.0f;
	}
	return 3.0f;
}

void UDragonStatsComponent::ApplyFlightStaminaRate(float DeltaSeconds, float SignedRatePerSecond)
{
	// [2026-11-04 FLIGHT-VIGOR] God mode gates signed flight stamina
	// (drain and refill alike) at this one seam, like DrainForFlight.
	if (IsInfiniteStaminaEnabled())
	{
		return;
	}
	ApplyStaminaDelta(SignedRatePerSecond * FMath::Max(0.0f, DeltaSeconds));
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
	// [2026-11-04 T-124] God mode: no per-second drain lands (covers the
	// legacy no-rate overload above, which delegates here, plus fire
	// breath, which reuses this verb for its per-second cost).
	if (IsInfiniteStaminaEnabled())
	{
		return;
	}
	// [2026-10-28 T-011] Negative time and non-positive rates (dive) are
	// free by construction: Max() on both, never a regen in disguise.
	ApplyStaminaDelta(-FMath::Max(0.0f, RatePerSecond) * FMath::Max(0.0f, DeltaSeconds));
}

void UDragonStatsComponent::ApplyHPDamage(float Damage)
{
	// [2026-10-28 T-011] Negative damage is not healing — clamp it away
	// so a bad caller cannot top the pool up through the damage verb.
	const float Before = HP;
	HP = FMath::Clamp(HP - FMath::Max(0.0f, Damage), 0.0f, MaxHP);
	// [2026-11-04 T-074] Hit/defeat events fire only on an effective
	// drop, so 0-damage probes and overkill past 0 stay silent past the
	// first defeat broadcast.
	if (HP < Before)
	{
		OnDragonHit.Broadcast(Before - HP, HP);
		if (HP <= 0.0f && !bDefeatBroadcast)
		{
			bDefeatBroadcast = true;
			OnDragonDefeated.Broadcast();
		}
	}
}

void UDragonStatsComponent::RestoreHP(float Amount)
{
	// [2026-10-28 T-015] Max() clamps a negative "heal" away — healing
	// must never become a second damage verb with a sign flip — and
	// the outer Clamp keeps the header invariant ([0, MaxHP]) for an
	// over-generous food value exactly as ApplyHPDamage does for
	// lethal overkill.
	HP = FMath::Clamp(HP + FMath::Max(0.0f, Amount), 0.0f, MaxHP);
	// [2026-11-04 T-074] Healing above 0 re-arms the defeat broadcast.
	if (HP > 0.0f)
	{
		bDefeatBroadcast = false;
	}
}

bool UDragonStatsComponent::TryConsumeMagic(float Cost)
{
	// [2026-11-04 T-080] Mirrors TryConsumeStamina: negative costs
	// and costs above the pool are refused with the pool untouched,
	// so no caller can spend magic it does not have. God mode does
	// NOT gate magic: the cvar is stamina-only by its own contract.
	if (Cost < 0.0f || Magic < Cost)
	{
		return false;
	}
	Magic -= Cost;
	return true;
}

void UDragonStatsComponent::ApplyMagicDelta(float Delta)
{
	// [2026-11-04 T-080] The one clamped write every magic verb
	// funnels through, exactly as ApplyStaminaDelta is for stamina.
	Magic = FMath::Clamp(Magic + Delta, 0.0f, MaxMagic);
}

bool UDragonStatsComponent::DrainMagic(float DeltaSeconds, float RatePerSecond)
{
	// [2026-11-04 T-080] Negative time/rates are free by construction
	// (never a regen in disguise), matching DrainForFlight. Returns
	// HasMagic() after the drain: false is the caller's signal to
	// shut the draining effect off this tick (true-sight, T-080).
	ApplyMagicDelta(-FMath::Max(0.0f, RatePerSecond) * FMath::Max(0.0f, DeltaSeconds));
	return HasMagic();
}

void UDragonStatsComponent::RegenMagic(float DeltaSeconds)
{
	// [2026-11-04 T-080] Time-based base regen (0.5/s, S6 basis in
	// the header). Callers pause it while a drain is active.
	ApplyMagicDelta(MagicRegenPerSecond * FMath::Max(0.0f, DeltaSeconds));
}

bool UDragonStatsComponent::HasMagic() const
{
	return Magic > 0.0f;
}

void UDragonStatsComponent::DrainHPForExhaustion(float DeltaSeconds)
{
	// [2026-11-04 T-124] God mode: the exhaustion HP burn is a stamina
	// consequence, so it is gated with the drains (plain HP damage via
	// ApplyHPDamage is never gated).
	if (IsInfiniteStaminaEnabled())
	{
		return;
	}
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
	// burn. [2026-11-03 T-108] Same for glide (0/s since T-062):
	// only hover still drains (3/s since T-124), so only continuing
	// to hover at zero stamina has both consequences. (Corrects the
	// T-036 note, which gave glide a positive rate.)
	// [2026-11-04 T-124] God mode never forces the fall (the pool never
	// depletes while on; without this, toggling on at empty stamina
	// would ground a dragon that can no longer spend). The rate rule
	// below is unchanged when the cvar is off.
	if (IsInfiniteStaminaEnabled())
	{
		return false;
	}
	return RatePerSecond > 0.0f;
}
