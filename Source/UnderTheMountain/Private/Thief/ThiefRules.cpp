// Copyright Under the Mountain contributors.
// [2026-11-04 T-074/T-089/T-132/T-133] FThiefRules implementation: the
// pure tables and decisions declared in ThiefRules.h. No world, actor,
// clock or RNG is read here — callers inject positions, stars and rolls.

#include "Thief/ThiefRules.h"

const FThiefAttackProfile& FThiefRules::AttackProfile()
{
	static const FThiefAttackProfile Profile;
	return Profile;
}

bool FThiefRules::CanAttackDragon(EThiefRole Role)
{
	// [2026-11-04 T-074] Only the armed types fight (S5.2 guard, S5.3
	// adventurer). Burglars/carriers/trappers flee or work; giving them
	// a sword would break the counter matrix's teach order.
	return Role == EThiefRole::Adventurer || Role == EThiefRole::Guard;
}

float FThiefRules::MeleeDamageForRole(EThiefRole Role)
{
	const FThiefAttackProfile& Profile = AttackProfile();
	if (Role == EThiefRole::Adventurer)
	{
		return Profile.MeleeDamage;
	}
	if (Role == EThiefRole::Guard)
	{
		return Profile.GuardMeleeDamage;
	}
	return 0.0f;
}

float FThiefRules::ResolveIncomingDamage(float Damage, EThiefRole Role, bool bFromFront,
										 bool bIsTailSwipe, int32 Stars)
{
	const float In = FMath::Max(0.0f, Damage);
	if (In <= 0.0f || bIsTailSwipe || !bFromFront)
	{
		// [2026-11-04 T-133] Tail bypasses every frontal block (S5.4);
		// flank/rear hits are never blocked either.
		return In;
	}
	if (Role == EThiefRole::Guard)
	{
		return In * 0.5f; // [tuning: PT-THIEF] S5.2 guard brace: claw -50% from front
	}
	if (Role == EThiefRole::Adventurer && Stars >= 2)
	{
		return 0.0f; // [tuning: PT-WAVE] S5.3/S5.4 shield (2+ stars) blocks frontal claw outright
	}
	return In;
}

float FThiefRules::HPForRole(EThiefRole Role, float AuthoredHP)
{
	// [2026-11-04 T-133] S5.2 fixes dwarf HP by role (guard 70, others
	// 45) regardless of the shared party asset's authored value.
	if (Role == EThiefRole::Guard)
	{
		return 70.0f;
	}
	if (Role == EThiefRole::Carrier || Role == EThiefRole::Trapper)
	{
		return 45.0f;
	}
	return FMath::Max(0.0f, AuthoredHP);
}

int32 FThiefRules::CarriedCapForRole(EThiefRole Role, int32 AuthoredCap)
{
	switch (Role)
	{
	case EThiefRole::Carrier:
		return 3; // GAMEPLAY.md S5.2
	case EThiefRole::Burglar:
		return 1; // GAMEPLAY.md S5.1
	case EThiefRole::Adventurer:
		return 2; // GAMEPLAY.md S5.3
	default:
		return FMath::Max(1, AuthoredCap);
	}
}

int32 FThiefRules::RepickCarrierIndex(const TArray<FPartyMemberState>& Members,
									  float SecondsSinceCarrierDeath)
{
	bool bCarrierAlive = false;
	for (const FPartyMemberState& Member : Members)
	{
		if (Member.Role == EThiefRole::Carrier && Member.bAlive)
		{
			bCarrierAlive = true;
		}
	}
	if (bCarrierAlive || SecondsSinceCarrierDeath < CarrierRepickDelaySeconds)
	{
		return -1;
	}
	// [2026-11-04 T-133] Promotion order: trapper first (hands free of
	// the dragon), then any other living member, guard only last.
	int32 GuardIndex = -1;
	for (int32 i = 0; i < Members.Num(); ++i)
	{
		if (!Members[i].bAlive || Members[i].Role == EThiefRole::Carrier)
		{
			continue;
		}
		if (Members[i].Role == EThiefRole::Trapper)
		{
			return i;
		}
		if (Members[i].Role == EThiefRole::Guard)
		{
			GuardIndex = i;
			continue;
		}
		return i;
	}
	return GuardIndex;
}

int32 FThiefRules::NextWaypointIndex(const TArray<FVector>& Waypoints, const FVector& Current,
									 float ArriveRadius, int32 FromIndex)
{
	int32 Index = FMath::Max(0, FromIndex);
	const float Radius = FMath::Max(0.0f, ArriveRadius);
	while (Index < Waypoints.Num() && FVector::Dist2D(Current, Waypoints[Index]) <= Radius)
	{
		++Index;
	}
	return Index;
}

FVector FThiefRules::SteerTarget(const TArray<FVector>& Waypoints, const FVector& Current,
								 const FVector& FinalDestination, float ArriveRadius,
								 int32& InOutIndex)
{
	InOutIndex = NextWaypointIndex(Waypoints, Current, ArriveRadius, InOutIndex);
	return InOutIndex < Waypoints.Num() ? Waypoints[InOutIndex] : FinalDestination;
}

const TArray<FThiefEntryPoint>& FThiefRules::EntryPoints()
{
	// [2026-11-04 T-133] The one authored entry table. Coordinates are
	// the T-066 mountain mouths previously scattered across
	// ThiefCharacter/WaveSubsystem/DragonCharacter: two burglar cracks
	// (north/south) and the grand east portal everyone else uses.
	static const TArray<FThiefEntryPoint> Points = {
		{FName(TEXT("CrackNorth")), EThiefEntryKind::Crack, FVector(7500.0f, 7000.0f, 150.0f)},
		{FName(TEXT("CrackSouth")), EThiefEntryKind::Crack, FVector(11500.0f, -7000.0f, 150.0f)},
		{FName(TEXT("GrandPortal")), EThiefEntryKind::GrandPortal, FVector(18700.0f, 0.0f, 160.0f)},
	};
	return Points;
}

FThiefEntryPoint FThiefRules::EntryForRole(EThiefRole Role, const FVector& HintLocation)
{
	const TArray<FThiefEntryPoint>& Points = EntryPoints();
	const FThiefEntryPoint* Portal = nullptr;
	const FThiefEntryPoint* BestCrack = nullptr;
	float BestCrackDist = TNumericLimits<float>::Max();
	for (const FThiefEntryPoint& Point : Points)
	{
		if (Point.Kind == EThiefEntryKind::GrandPortal && Portal == nullptr)
		{
			Portal = &Point;
		}
		if (Point.Kind == EThiefEntryKind::Crack)
		{
			const float Dist = FVector::Dist2D(HintLocation, Point.Location);
			if (Dist < BestCrackDist)
			{
				BestCrackDist = Dist;
				BestCrack = &Point;
			}
		}
	}
	if (Role == EThiefRole::Burglar && BestCrack != nullptr)
	{
		return *BestCrack;
	}
	if (Portal != nullptr)
	{
		return *Portal;
	}
	return Points.Num() > 0 ? Points[0] : FThiefEntryPoint();
}

FNotorietyGearTier FThiefRules::GearForStars(int32 Stars)
{
	// [2026-11-04 T-132] GAMEPLAY.md S4 DT_NotorietyGear, verbatim rows.
	// Stars clamp to the table; gear beyond 5 stars is the M3 army's job.
	FNotorietyGearTier Tier;
	switch (FMath::Clamp(Stars, 0, 5))
	{
	case 1:
		Tier.bSmokePuff = true;
		Tier.LootMinGold = 5;
		Tier.LootMaxGold = 25;
		break;
	case 2:
		Tier.HPMultiplier = 1.25f;
		Tier.SpeedMultiplier = 1.05f;
		Tier.bTrapsLive = true;
		Tier.bShield = true;
		Tier.LootMinGold = 25;
		Tier.LootMaxGold = 75;
		break;
	case 3:
		Tier.HPMultiplier = 1.5f;
		Tier.SpeedMultiplier = 1.1f;
		Tier.bTrapsLive = true;
		Tier.bShield = true;
		Tier.bFireCloak = true;
		Tier.bCrossbow = true;
		Tier.LootMinGold = 75;
		Tier.LootMaxGold = 200;
		Tier.PrizeChance = 0.25f; // [tuning: PT-WAVE] "chance of Prize+ item", delegated 1-in-4
		break;
	case 4:
		Tier.HPMultiplier = 2.0f;
		Tier.SpeedMultiplier = 1.15f;
		Tier.bTrapsLive = true;
		Tier.bShield = true;
		Tier.bFireCloak = true;
		Tier.bCrossbow = true;
		Tier.bPitAndNet = true;
		Tier.LootMinGold = 200;
		Tier.LootMaxGold = 500;
		Tier.PrizeChance = 0.25f;
		break;
	case 5:
		Tier.HPMultiplier = 2.5f;
		Tier.SpeedMultiplier = 1.2f;
		Tier.bTrapsLive = true;
		Tier.bShield = true;
		Tier.bFireCloak = true;
		Tier.bCrossbow = true;
		Tier.bPitAndNet = true;
		Tier.bHealerSupport = true;
		Tier.LootMinGold = 500;
		Tier.LootMaxGold = 750; // [tuning: PT-WAVE] S4 says "500+"; capped for the drop roll
		Tier.PrizeChance = 1.0f;
		Tier.bGuaranteedPrizePerParty = true;
		break;
	default: // 0 stars
		Tier.LootMinGold = 0;
		Tier.LootMaxGold = 10;
		break;
	}
	return Tier;
}

float FThiefRules::ScaledHP(float BaseHP, int32 Stars)
{
	return FMath::Max(0.0f, BaseHP) * GearForStars(Stars).HPMultiplier;
}

float FThiefRules::ScaledSpeed(float BaseSpeed, int32 Stars)
{
	return FMath::Max(0.0f, BaseSpeed) * GearForStars(Stars).SpeedMultiplier;
}

FTreasureEntry FThiefRules::MakeRaiderLootDrop(FName ThiefId, int32 Stars, const FVector& Location,
											   float Roll01)
{
	const FNotorietyGearTier Tier = GearForStars(Stars);
	const float Roll = FMath::Clamp(Roll01, 0.0f, 1.0f);
	FTreasureEntry Drop;
	Drop.Id = FName(*FString::Printf(TEXT("RaiderLoot_%s"), *ThiefId.ToString()));
	Drop.GoldValue =
		Tier.LootMinGold + FMath::RoundToInt(Roll * static_cast<float>(Tier.LootMaxGold - Tier.LootMinGold));
	Drop.Location = Location;
	Drop.bCarried = false;
	Drop.CarrierId = NAME_None;
	return Drop;
}

FName FThiefRules::RegisterRaiderLootDrop(UTreasureRegistry* Registry, const FTreasureEntry& Drop)
{
	if (Registry == nullptr || Drop.Id == NAME_None)
	{
		return NAME_None;
	}
	// [2026-11-04 T-132] Registered at the death location, uncarried, so
	// scent/HUD read it the same tick the raider falls (GAMEPLAY.md S4).
	return Registry->Register(Drop);
}
