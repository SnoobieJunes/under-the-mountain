// Copyright Under the Mountain contributors.
// [2026-11-01 T-016] UNotorietySubsystem implementation: the
// section 4 point table as named verbs over one counter.

#include "Notoriety/NotorietySubsystem.h"
#include "UnderTheMountain.h"

bool UNotorietySubsystem::IsInsideLair(const FVector& Location)
{
	// [2026-11-03 T-066] Mountain bounds (was the T-012 chamber chain
	// x -2500..21500): the great chamber + slopes + portal runs live in
	// x [-500, 20000], |y| <= 10000, z <= 5400. Above the crown opening
	// (z > 5400) and past the apron (x > 20000) is outside — kills there
	// are witnessed, exactly like the old peak exit above z 5200 was.
	return Location.X >= -500.0f && Location.X <= 20000.0f && FMath::Abs(Location.Y) <= 10000.0f &&
		   Location.Z <= 5400.0f;
}

int32 UNotorietySubsystem::StarsForPoints(int32 InPoints)
{
	// [2026-11-01 T-016] Section 4 thresholds, checked high to
	// low; below 10 points the dragon is a myth (0 stars).
	if (InPoints >= 150)
	{
		return 5;
	}
	if (InPoints >= 100)
	{
		return 4;
	}
	if (InPoints >= 60)
	{
		return 3;
	}
	if (InPoints >= 30)
	{
		return 2;
	}
	if (InPoints >= 10)
	{
		return 1;
	}
	return 0;
}

int32 UNotorietySubsystem::PointsForKillAt(const FVector& Location)
{
	// [2026-11-01 T-016] The inside-lair zero-gain invariant
	// lives here so no caller can break it by forgetting.
	return IsInsideLair(Location) ? 0 : 10;
}

int32 UNotorietySubsystem::GetPoints() const
{
	return Points;
}

int32 UNotorietySubsystem::GetStars() const
{
	return StarsForPoints(Points);
}

void UNotorietySubsystem::AddPoints(int32 Amount, const TCHAR* Reason)
{
	Points = FMath::Max(0, Points + Amount);
	UE_LOG(LogUnderTheMountain, Log, TEXT("Notoriety %+d (%s): %d points, %d star(s)"), Amount,
		   Reason, Points, GetStars());
}

void UNotorietySubsystem::AddEscapePoints(bool bWithLoot)
{
	AddPoints(bWithLoot ? 8 : 5, bWithLoot ? TEXT("escape with loot") : TEXT("escape"));
}

void UNotorietySubsystem::AddPublicKillPoints(const FVector& KillLocation)
{
	const int32 Gain = PointsForKillAt(KillLocation);
	if (Gain > 0)
	{
		AddPoints(Gain, TEXT("public kill/eat"));
	}
}

void UNotorietySubsystem::AddRoarOutsidePoints()
{
	AddPoints(4, TEXT("roar-scare outside"));
}

void UNotorietySubsystem::AddLivestockEatPoints()
{
	AddPoints(3, TEXT("livestock eaten at settlement"));
}

void UNotorietySubsystem::AddStructureHitPoints()
{
	AddPoints(6, TEXT("settlement structure burnt"));
}

int32 UNotorietySubsystem::PointsFloorForStars(int32 Stars)
{
	// [2026-11-04 T-084] The S4 thresholds read backwards: entering
	// 1-5 stars takes 10/30/60/100/150 points, so those are the
	// band floors a decayed total resets to. Clamped at both ends
	// so a bogus star count lands on a real band, never between.
	static const int32 Floors[] = {0, 10, 30, 60, 100, 150};
	return Floors[FMath::Clamp(Stars, 0, 5)];
}

int32 UNotorietySubsystem::PointsAfterSleepDecay(int32 InPoints)
{
	// [2026-11-04 T-084] S9: -2 stars, then points snap DOWN to the
	// resulting band's floor (partial progress into the old band
	// is forgotten — that snap is what makes one sleep meaningful
	// from 4-5 stars without erasing a massacre in one nap).
	const int32 NewStars = FMath::Max(0, StarsForPoints(FMath::Max(0, InPoints)) - 2);
	return PointsFloorForStars(NewStars);
}

void UNotorietySubsystem::ApplySleepDecay()
{
	// [2026-11-04 T-084] The single-writer application of the pure
	// math above; logged like every other notoriety change so a
	// session's star story stays readable from the game log alone.
	const int32 OldPoints = Points;
	Points = PointsAfterSleepDecay(Points);
	UE_LOG(LogUnderTheMountain, Log, TEXT("Notoriety sleep decay: %d -> %d points, %d star(s)"),
		   OldPoints, Points, GetStars());
}

void UNotorietySubsystem::AddPublicKillPointsWitnessed(const FVector& KillLocation,
													   const TArray<FSettlementArea>& Settlements,
													   const TArray<FVillagerWitness>& Villagers)
{
	// [2026-11-04 T-134] Witness gate replaces the box-only charge for
	// pawn-reported kills; the legacy verb above stays for callers
	// that already established witness (and for its T-016 tests).
	const bool bInside = IsInsideLair(KillLocation);
	const bool bWitnessed = FWitnessRules::IsWitnessed(KillLocation, bInside, Settlements, Villagers,
													   FWitnessRules::DefaultWitnessRadius);
	const int32 Gain = FWitnessRules::PointsForAct(EWitnessedAct::PersonKill, bWitnessed, bInside);
	if (Gain > 0)
	{
		AddPoints(Gain, TEXT("witnessed public kill/eat"));
	}
}

void UNotorietySubsystem::AddLivestockEatPointsWitnessed(const FVector& EatLocation,
														 const TArray<FSettlementArea>& Settlements,
														 const TArray<FVillagerWitness>& Villagers)
{
	const bool bInside = IsInsideLair(EatLocation);
	const bool bWitnessed = FWitnessRules::IsWitnessed(EatLocation, bInside, Settlements, Villagers,
													   FWitnessRules::DefaultWitnessRadius);
	const int32 Gain = FWitnessRules::PointsForAct(EWitnessedAct::LivestockEat, bWitnessed, bInside);
	if (Gain > 0)
	{
		AddPoints(Gain, TEXT("witnessed livestock eaten"));
	}
}
