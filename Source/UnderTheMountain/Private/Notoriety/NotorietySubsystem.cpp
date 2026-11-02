// Copyright Under the Mountain contributors.
// [2026-11-01 T-016] UNotorietySubsystem implementation: the
// section 4 point table as named verbs over one counter.

#include "Notoriety/NotorietySubsystem.h"
#include "UnderTheMountain.h"

bool UNotorietySubsystem::IsInsideLair(const FVector& Location)
{
	// [2026-11-01 T-016] Chamber chain C1..hoard at x 0..19000
	// with 3000-wide chambers, plus margin; the shaft (z up to
	// 5200 at the hoard) is inside until the peak exit.
	return Location.X >= -2500.0f && Location.X <= 21500.0f && FMath::Abs(Location.Y) <= 2500.0f &&
		   Location.Z <= 5200.0f;
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
