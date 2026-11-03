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
