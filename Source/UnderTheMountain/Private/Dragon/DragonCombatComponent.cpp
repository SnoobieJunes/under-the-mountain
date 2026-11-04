// Copyright Under the Mountain contributors.
// [2026-10-28 T-015] UDragonCombatComponent implementation: fire cone
// DPS, roar gating, and the eat channel. All world knowledge (which
// brains exist, where they stand, what time it is) arrives as call
// arguments from the pawn, and all resource math leaves through
// UDragonStatsComponent — this file owns only combat rules.

#include "Dragon/DragonCombatComponent.h"
#include "UnderTheMountain.h"
#include "World/BurnablePropActor.h"

#include "Dragon/DragonStatsComponent.h"
#include "Livestock/GoatActor.h"
#include "Thief/ThiefBrain.h"

UDragonCombatComponent::UDragonCombatComponent()
{
	// [2026-10-28 T-015] Ticking stays OFF, mirroring
	// UDragonStatsComponent: the pawn knows when combat verbs tick
	// (input held, channel running) and a component guessing at that
	// state was rejected. The pawn calls TickFire/TickEat instead.
	PrimaryComponentTick.bCanEverTick = false;
}

bool UDragonCombatComponent::IsInFireCone(const FVector& Origin, const FVector& Forward,
										  const FVector& Target, float Range, float HalfAngleDeg)
{
	const FVector Offset = Target - Origin;
	const float Distance = Offset.Size();

	// [2026-10-28 T-015] Range first: a target beyond the breath is
	// outside at any angle, and range-checking before normalising
	// also rejects a degenerate negative Range outright.
	if (Distance > Range)
	{
		return false;
	}

	// [2026-10-28 T-015] A zero-length offset has no angle to test
	// and counts as inside by contract: whatever stands exactly at
	// the breath's origin is in the breath.
	if (Distance <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	// [2026-10-28 T-015] A near-zero Forward aims nowhere; with a
	// real offset to judge, no aim means no hit (rather than
	// normalising noise into a random direction).
	const FVector ForwardNormal = Forward.GetSafeNormal();
	if (ForwardNormal.IsNearlyZero())
	{
		return false;
	}

	// [2026-10-28 T-015] The dot is clamped into Acos's domain
	// because two normalised vectors can dot to 1.0000001 in float.
	// The final comparison carries a hair of tolerance so a target
	// mathematically ON the cone edge counts as inside (the header
	// contract) instead of flickering on Acos rounding.
	const float CosAngle =
		FMath::Clamp(FVector::DotProduct(ForwardNormal, Offset / Distance), -1.0f, 1.0f);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(CosAngle));
	return AngleDeg <= HalfAngleDeg + 0.001f;
}

float UDragonCombatComponent::CrushDamageForImpact(float ImpactSpeed, float Damage,
												   float MinImpactSpeed)
{
	// [2026-11-01 T-077] Threshold is inclusive and speed is a
	// magnitude: exactly at the charge speed crushes, and direction
	// (falling vs running) never matters to a body being landed on.
	if (FMath::Abs(ImpactSpeed) < MinImpactSpeed)
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, Damage);
}

bool UDragonCombatComponent::TryStartFire(UDragonStatsComponent* Stats)
{
	// [2026-10-28 T-015] Both refusals log at Warning: "fire did
	// nothing" must always have a named reason in the log, whether
	// that is a wiring bug (null Stats) or honest exhaustion.
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryStartFire refused: stats component is null"));
		return false;
	}
	if (Stats->Stamina < FireMinStamina)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryStartFire refused: stamina %.1f below the %.1f needed to "
					"start fire"),
			   Stats->Stamina, FireMinStamina);
		return false;
	}

	bFiring = true;
	return true;
}

void UDragonCombatComponent::StopFire()
{
	bFiring = false;
}

bool UDragonCombatComponent::IsFiring() const
{
	return bFiring;
}

int32 UDragonCombatComponent::TickFire(float Dt, UDragonStatsComponent* Stats,
									   const FVector& Origin, const FVector& Forward,
									   const TArray<UThiefBrain*>& Brains,
									   const TArray<FVector>& BrainLocations,
									   const TArray<AGoatActor*>& Goats,
									   const TArray<FVector>& GoatLocations,
									   const TArray<ABurnablePropActor*>& Burnables,
									   const TArray<FVector>& BurnableLocations)
{
	if (!bFiring)
	{
		return 0;
	}

	// [2026-10-28 T-015] No stats, no breath: draining is the price
	// of fire, so a null Stats mid-breath stops the breath loudly
	// instead of letting it burn for free.
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TickFire: stats component is null - fire stopped"));
		bFiring = false;
		return 0;
	}

	// [2026-10-28 T-015] Negative time is clamped to a no-op frame,
	// never a stamina refund or negative damage.
	const float EffectiveDt = FMath::Max(0.0f, Dt);
	if (EffectiveDt <= 0.0f)
	{
		return 0;
	}

	// [2026-10-28 T-015] DrainForFlight's explicit-rate overload is
	// UDragonStatsComponent's generic per-second stamina drain (the
	// pawn already passes it glide/hover rates of its own choosing),
	// so fire reuses it deliberately rather than growing a second
	// drain verb for one caller. The drain runs BEFORE damage so
	// the tick that empties the pool has paid for its damage; the
	// empty-pool stop below then applies from the next tick.
	Stats->DrainForFlight(EffectiveDt, FireCostPerSecond);

	// [2026-10-28 T-015] Parallel arrays are the pawn's contract;
	// if they ever disagree, say so and work the shared prefix
	// rather than reading past the shorter array or failing mute.
	const int32 Count = FMath::Min(Brains.Num(), BrainLocations.Num());
	if (Brains.Num() != BrainLocations.Num())
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TickFire: %d brains but %d locations - checking only the "
					"first %d"),
			   Brains.Num(), BrainLocations.Num(), Count);
	}

	int32 NumDamaged = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UThiefBrain* Brain = Brains[Index];
		// [2026-10-28 T-015] Null holes and corpses are skipped:
		// burning a body costs the dragon nothing extra here, but
		// counting it as "damaged" would lie to the pawn and to
		// tests about what the breath accomplished.
		if (Brain == nullptr || Brain->State() == EThiefState::Dead)
		{
			continue;
		}
		if (!IsInFireCone(Origin, Forward, BrainLocations[Index], FireRange, FireConeHalfAngleDeg))
		{
			continue;
		}
		// [2026-10-28 T-015] The brain owns the consequences (loot
		// drops, death); combat only deals DPS scaled by Dt and
		// counts the brains it actually burned this tick.
		Brain->ApplyDamage(FireDps * EffectiveDt, BrainLocations[Index]);
		++NumDamaged;
	}

	// [2026-11-01 T-078] Goats in the same cone, same DPS: before
	// this, fire accepted only thief brains, so a goat could be
	// breathed on forever and only ever eaten alive. Corpses and
	// eaten goats are skipped (ApplyGoatDamage would refuse them
	// anyway) so the damaged count never claims a body twice.
	const int32 GoatCount = FMath::Min(Goats.Num(), GoatLocations.Num());
	if (Goats.Num() != GoatLocations.Num())
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TickFire: %d goats but %d locations - checking only the first %d"),
			   Goats.Num(), GoatLocations.Num(), GoatCount);
	}
	for (int32 Index = 0; Index < GoatCount; ++Index)
	{
		AGoatActor* Goat = Goats[Index];
		if (Goat == nullptr || Goat->IsDead() || Goat->IsEaten())
		{
			continue;
		}
		if (!IsInFireCone(Origin, Forward, GoatLocations[Index], FireRange, FireConeHalfAngleDeg))
		{
			continue;
		}
		Goat->ApplyGoatDamage(FireDps * EffectiveDt);
		++NumDamaged;
	}

	// [2026-11-01 T-055] Burnable props in the cone accumulate
	// exposure (seconds, not DPS): ignition is the prop's own
	// threshold call. Already-burning/burnt props ignore it inside
	// AddFireExposure, so no state check is needed here.
	const int32 PropCount = FMath::Min(Burnables.Num(), BurnableLocations.Num());
	for (int32 Index = 0; Index < PropCount; ++Index)
	{
		ABurnablePropActor* Prop = Burnables[Index];
		if (Prop == nullptr)
		{
			continue;
		}
		if (!IsInFireCone(Origin, Forward, BurnableLocations[Index], FireRange, FireConeHalfAngleDeg))
		{
			continue;
		}
		Prop->AddFireExposure(EffectiveDt);
	}

	// [2026-10-28 T-015] An emptied pool ends the breath. Checking
	// after the damage loop is what makes the final tick's damage
	// real (see the drain comment above); TryStartFire's minimum
	// then keeps the next press from sputtering.
	if (Stats->Stamina <= 0.0f)
	{
		bFiring = false;
	}

	return NumDamaged;
}

bool UDragonCombatComponent::TryRoar(float CurrentTimeSeconds, UDragonStatsComponent* Stats,
									 const FVector& Origin, const TArray<UThiefBrain*>& Brains,
									 const TArray<FVector>& BrainLocations)
{
	// [2026-10-28 T-015] Every block path below logs its reason at
	// Warning and leaves both stamina and LastRoarTime untouched:
	// a roar that never happened must cost nothing and cool down
	// nothing.
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryRoar refused: stats component is null"));
		return false;
	}

	// [2026-10-28 T-015] Cooldown is checked BEFORE the stamina
	// cost so mashing roar during the 6 s window is free to fail.
	const float SinceLastRoar = CurrentTimeSeconds - LastRoarTime;
	if (SinceLastRoar < RoarCooldownSeconds)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryRoar blocked: %.1fs of roar cooldown remain"),
			   RoarCooldownSeconds - SinceLastRoar);
		return false;
	}

	if (!Stats->TryConsumeStamina(RoarCost))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryRoar blocked: stamina %.1f below the %.1f roar cost"),
			   Stats->Stamina, RoarCost);
		return false;
	}

	// [2026-10-28 T-015] Point of no return: the roar happened, so
	// the cooldown runs from this instant even if no thief heard it.
	LastRoarTime = CurrentTimeSeconds;

	// [2026-10-28 T-015] Same parallel-array contract as TickFire:
	// warn on disagreement, work the shared prefix.
	const int32 Count = FMath::Min(Brains.Num(), BrainLocations.Num());
	if (Brains.Num() != BrainLocations.Num())
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryRoar: %d brains but %d locations - roaring at only the "
					"first %d"),
			   Brains.Num(), BrainLocations.Num(), Count);
	}

	int32 NumScared = 0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UThiefBrain* Brain = Brains[Index];
		// [2026-10-28 T-015] Corpses are skipped here (the brain
		// would ignore the roar anyway) so NumScared counts only
		// thieves the roar actually frightened.
		if (Brain == nullptr || Brain->State() == EThiefState::Dead)
		{
			continue;
		}
		// [2026-10-28 T-015] 3D distance (header contract): height
		// above a thief genuinely puts the dragon farther from
		// their ears than the ground distance suggests.
		if (FVector::Dist(Origin, BrainLocations[Index]) > RoarRadius)
		{
			continue;
		}
		// [2026-10-28 T-015] OnRoar takes the THIEF's location:
		// dropped loot lands where the thief stands, never at the
		// dragon.
		Brain->OnRoar(BrainLocations[Index]);
		++NumScared;
	}

	UE_LOG(LogUnderTheMountain, Log, TEXT("DragonCombat roar scared %d within %.0fcm"), NumScared,
		   RoarRadius);
	return true;
}

bool UDragonCombatComponent::IsInArc2D(const FVector& Origin, const FVector& Forward,
									   const FVector& Target, float Range, float HalfAngleDeg)
{
	// [2026-11-01 T-068] Ground-plane arc: height must not rescue a thief
	// [2026-11-01 T-068] from a wing gust at ankle level, nor doom one on a
	// [2026-11-01 T-068] ledge — these verbs sweep the floor the dragon owns.
	const FVector2D Offset(Target.X - Origin.X, Target.Y - Origin.Y);
	const float Distance = Offset.Size();
	if (Distance > Range)
	{
		return false;
	}
	if (Distance < 1.0f)
	{
		return true;
	}
	const FVector2D Fwd = FVector2D(Forward.X, Forward.Y).GetSafeNormal();
	if (Fwd.IsNearlyZero())
	{
		return false;
	}
	const float CosAngle = FMath::Clamp(FVector2D::DotProduct(Fwd, Offset / Distance), -1.0f, 1.0f);
	return FMath::RadiansToDegrees(FMath::Acos(CosAngle)) <= HalfAngleDeg;
}

bool UDragonCombatComponent::TryWingBuffet(float CurrentTimeSeconds, UDragonStatsComponent* Stats,
										   const FVector& Origin, const FVector& Forward,
										   const TArray<UThiefBrain*>& Brains,
										   const TArray<FVector>& BrainLocations, int32& OutNumDazed)
{
	OutNumDazed = 0;
	// [2026-11-01 T-068] TryRoar's gating contract, verbatim order:
	// [2026-11-01 T-068] null stats, cooldown, then stamina; blocked = free.
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryWingBuffet refused: stats component is null"));
		return false;
	}
	const float SinceLast = CurrentTimeSeconds - LastBuffetTime;
	if (SinceLast < WingBuffetCooldownSeconds)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryWingBuffet blocked: %.1fs of buffet cooldown remain"),
			   WingBuffetCooldownSeconds - SinceLast);
		return false;
	}
	if (!Stats->TryConsumeStamina(WingBuffetCost))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryWingBuffet blocked: stamina %.1f below the %.1f buffet cost"),
			   Stats->Stamina, WingBuffetCost);
		return false;
	}
	LastBuffetTime = CurrentTimeSeconds;
	const int32 Count = FMath::Min(Brains.Num(), BrainLocations.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UThiefBrain* Brain = Brains[Index];
		if (Brain == nullptr || Brain->State() == EThiefState::Dead)
		{
			continue;
		}
		if (!IsInArc2D(Origin, Forward, BrainLocations[Index], WingBuffetRadius, WingBuffetHalfAngleDeg))
		{
			continue;
		}
		Brain->OnWingBuffet(BrainLocations[Index]);
		++OutNumDazed;
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("DragonCombat wing buffet dazed %d within %.0fcm"), OutNumDazed,
		   WingBuffetRadius);
	return true;
}

bool UDragonCombatComponent::TryTailSlam(float CurrentTimeSeconds, UDragonStatsComponent* Stats,
										 const FVector& Origin, const FVector& Forward,
										 const TArray<UThiefBrain*>& Brains,
										 const TArray<FVector>& BrainLocations, int32& OutNumHit)
{
	OutNumHit = 0;
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryTailSlam refused: stats component is null"));
		return false;
	}
	const float SinceLast = CurrentTimeSeconds - LastTailSlamTime;
	if (SinceLast < TailSlamCooldownSeconds)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryTailSlam blocked: %.1fs of tail-slam cooldown remain"),
			   TailSlamCooldownSeconds - SinceLast);
		return false;
	}
	if (!Stats->TryConsumeStamina(TailSlamCost))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryTailSlam blocked: stamina %.1f below the %.1f slam cost"),
			   Stats->Stamina, TailSlamCost);
		return false;
	}
	LastTailSlamTime = CurrentTimeSeconds;
	// [2026-11-01 T-068] Rear arc: the tail sweeps behind the dragon, which
	// [2026-11-01 T-068] is exactly why GAMEPLAY section 5.4 says it bypasses
	// [2026-11-01 T-068] a frontal shield block.
	const FVector Backward = -Forward;
	const int32 Count = FMath::Min(Brains.Num(), BrainLocations.Num());
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UThiefBrain* Brain = Brains[Index];
		if (Brain == nullptr || Brain->State() == EThiefState::Dead)
		{
			continue;
		}
		if (!IsInArc2D(Origin, Backward, BrainLocations[Index], TailSlamRadius, TailSlamHalfAngleDeg))
		{
			continue;
		}
		Brain->ApplyDamage(TailSlamDamage, BrainLocations[Index]);
		++OutNumHit;
	}
	UE_LOG(LogUnderTheMountain, Log, TEXT("DragonCombat tail slam hit %d within %.0fcm rear arc"), OutNumHit,
		   TailSlamRadius);
	return true;
}

int32 UDragonCombatComponent::ClawTargetPriority(EThiefKind Kind, bool bCarryingLoot)
{
	// [2026-11-04 T-076] §5.4's "claw vs carrier is the efficient
	// kill", ranked: the dwarf carrier is the party's loot pipe,
	// any carrier is next, a non-carrying dwarf still outranks a
	// bystander of another kind.
	if (Kind == EThiefKind::Dwarf && bCarryingLoot)
	{
		return 30;
	}
	if (bCarryingLoot)
	{
		return 20;
	}
	if (Kind == EThiefKind::Dwarf)
	{
		return 10;
	}
	return 0;
}

bool UDragonCombatComponent::IsFrontalClawBlocked(EThiefKind Kind, int32 GearTier,
												  const FVector& ThiefForward,
												  const FVector& ThiefLocation,
												  const FVector& DragonLocation)
{
	if (Kind != EThiefKind::Adventurer || GearTier < 2)
	{
		return false;
	}
	// [2026-11-04 T-076] Frontal = dragon inside the shield's
	// 120°: reuse the arc test from the thief's seat (zero offset
	// counts as frontal — a dragon in the shield's face is blocked).
	return IsInArc2D(ThiefLocation, ThiefForward, DragonLocation, TNumericLimits<float>::Max(),
					 60.0f);
}

bool UDragonCombatComponent::TryClaw(float CurrentTimeSeconds, UDragonStatsComponent* Stats)
{
	// [2026-11-04 T-076] TryRoar's gating order, verbatim: null,
	// cooldown, stamina — a blocked swing is free and moves nothing.
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("DragonCombat TryClaw refused: stats is null"));
		return false;
	}
	const float SinceLast = CurrentTimeSeconds - LastClawTime;
	if (SinceLast < ClawCooldownSeconds)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryClaw blocked: %.1fs of claw cooldown remain"),
			   ClawCooldownSeconds - SinceLast);
		return false;
	}
	if (!Stats->TryConsumeStamina(ClawCost))
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TryClaw blocked: stamina %.1f below the %.1f claw cost"),
			   Stats->Stamina, ClawCost);
		return false;
	}
	LastClawTime = CurrentTimeSeconds;
	// [2026-11-04 T-076] No T-068 claw montage/notify exists, so
	// the verb timing IS the damage window (header note): the hit
	// is judged in TickClaw when this delay elapses.
	ClawTimeRemaining = ClawDamageDelaySeconds;
	return true;
}

int32 UDragonCombatComponent::TickClaw(float Dt, const FVector& Origin, const FVector& Forward,
									   const TArray<UThiefBrain*>& Brains,
									   const TArray<FVector>& BrainLocations,
									   const TArray<FVector>& BrainForwards, bool& bOutBlocked)
{
	bOutBlocked = false;
	if (ClawTimeRemaining <= 0.0f)
	{
		return 0;
	}
	ClawTimeRemaining -= FMath::Max(0.0f, Dt);
	if (ClawTimeRemaining > 0.0f)
	{
		return 0;
	}
	ClawTimeRemaining = 0.0f;

	const int32 Count = FMath::Min(Brains.Num(), FMath::Min(BrainLocations.Num(), BrainForwards.Num()));
	int32 Best = INDEX_NONE;
	int32 BestPriority = MIN_int32;
	float BestDistance = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UThiefBrain* Brain = Brains[Index];
		if (Brain == nullptr || Brain->State() == EThiefState::Dead)
		{
			continue;
		}
		if (!IsInArc2D(Origin, Forward, BrainLocations[Index], ClawRange, ClawHalfAngleDeg))
		{
			continue;
		}
		const int32 Priority = ClawTargetPriority(Brain->Kind(), Brain->IsCarryingLoot());
		const float Distance = FVector::Dist(Origin, BrainLocations[Index]);
		if (Priority > BestPriority || (Priority == BestPriority && Distance < BestDistance))
		{
			Best = Index;
			BestPriority = Priority;
			BestDistance = Distance;
		}
	}
	if (Best == INDEX_NONE)
	{
		UE_LOG(LogUnderTheMountain, Log, TEXT("DragonCombat claw whiffed (nobody in the arc)"));
		return 0;
	}
	UThiefBrain* Target = Brains[Best];
	if (IsFrontalClawBlocked(Target->Kind(), Target->GearTier(), BrainForwards[Best],
							 BrainLocations[Best], Origin))
	{
		// [2026-11-04 T-076] The shield eats the whole swing: no
		// damage, and no fall-through to the thief behind it.
		bOutBlocked = true;
		UE_LOG(LogUnderTheMountain, Log,
			   TEXT("DragonCombat claw BLOCKED frontally by adventurer shield (gear %d)"),
			   Target->GearTier());
		return 0;
	}
	Target->ApplyDamage(ClawDamage, BrainLocations[Best]);
	UE_LOG(LogUnderTheMountain, Log, TEXT("DragonCombat claw hit for %.0f (priority %d)"), ClawDamage,
		   BestPriority);
	return 1;
}

bool UDragonCombatComponent::StartEatThief(UThiefBrain* Brain)
{
	// [2026-10-28 T-015] Refusals are named, per the component's
	// never-silent rule: a null brain is a wiring bug, a living one
	// is the pawn offering a meal that is still using its legs.
	if (Brain == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat StartEatThief refused: brain is null"));
		return false;
	}
	if (Brain->State() != EThiefState::Dead)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat StartEatThief refused: thief is not dead"));
		return false;
	}

	// [2026-10-28 T-015] One-meal invariant: the new meal replaces
	// whatever was pending, and the channel restarts from full.
	PendingThief = Brain;
	PendingGoat = nullptr;
	EatTimeRemaining = EatChannelSeconds;
	return true;
}

bool UDragonCombatComponent::StartEatGoat(AGoatActor* Goat)
{
	if (Goat == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat StartEatGoat refused: goat is null"));
		return false;
	}
	if (Goat->IsEaten())
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat StartEatGoat refused: goat was already eaten"));
		return false;
	}

	// [2026-10-28 T-015] One-meal invariant, same as StartEatThief.
	PendingGoat = Goat;
	PendingThief = nullptr;
	EatTimeRemaining = EatChannelSeconds;
	return true;
}

EEatResult UDragonCombatComponent::TickEat(float Dt, UDragonStatsComponent* Stats)
{
	if (PendingThief == nullptr && PendingGoat == nullptr)
	{
		return EEatResult::None;
	}

	// [2026-10-28 T-015] A null Stats freezes (not cancels, not
	// completes) the channel: food applied to nothing would eat
	// the meal for no benefit, and a wiring hiccup should not
	// confiscate lunch outright either.
	if (Stats == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCombat TickEat: stats component is null - channel held"));
		return EEatResult::InProgress;
	}

	EatTimeRemaining -= FMath::Max(0.0f, Dt);
	if (EatTimeRemaining > 0.0f)
	{
		return EEatResult::InProgress;
	}
	EatTimeRemaining = 0.0f;

	// [2026-10-28 T-015] The pending pointer is cleared BEFORE the
	// food is applied in both branches, so completion is a one-way
	// door: no later TickEat (or confused double-call) can apply
	// the same meal twice.
	if (PendingThief != nullptr)
	{
		PendingThief = nullptr;
		Stats->ApplyStaminaDelta(ThiefFoodStamina);
		Stats->RestoreHP(ThiefFoodHP);
		// [2026-10-28 T-015] Nothing marks the brain consumed (by
		// design — see EEatResult): AteThief IS the mark, and the
		// pawn hides the corpse actor when it sees it.
		UE_LOG(LogUnderTheMountain, Log,
			   TEXT("DragonCombat ate thief corpse: +%.0f stamina, +%.0f HP"), ThiefFoodStamina,
			   ThiefFoodHP);
		return EEatResult::AteThief;
	}

	// [2026-10-28 T-015] Only the goat can be pending here (the
	// one-meal invariant plus the entry check), but the pointer is
	// still re-read and null-checked rather than trusted blindly.
	AGoatActor* Goat = PendingGoat.Get();
	PendingGoat = nullptr;
	if (Goat == nullptr)
	{
		return EEatResult::None;
	}

	// [2026-10-28 T-015] The goat's own food values, read before
	// Consume() so the log line reports what was actually fed.
	const float GainedStamina = Goat->FoodStamina;
	const float GainedHP = Goat->FoodHP;
	Stats->ApplyStaminaDelta(GainedStamina);
	Stats->RestoreHP(GainedHP);
	Goat->Consume();
	UE_LOG(LogUnderTheMountain, Log, TEXT("DragonCombat ate goat: +%.0f stamina, +%.0f HP"),
		   GainedStamina, GainedHP);
	return EEatResult::AteGoat;
}

void UDragonCombatComponent::CancelEat()
{
	// [2026-10-28 T-015] The damage-interrupt hook: abandoning the
	// channel applies no food and leaves the meal itself untouched
	// (a dead thief stays dead, a goat stays uneaten) so the dragon
	// can come back to it after the fight.
	PendingThief = nullptr;
	PendingGoat = nullptr;
	EatTimeRemaining = 0.0f;
}
