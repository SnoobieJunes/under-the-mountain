// Copyright Under the Mountain contributors.
// Purpose: Notoriety (T-016, docs/GAMEPLAY.md section 4) - the
// single writer of notoriety points. Stars are a display bucket
// over points; every gain in the spec table is a named verb here
// so no other system adds points by hand. The pawn observes world
// events (escapes, kills, roars, eats, ignitions) and calls the
// matching verb; this subsystem owns only the accounting.
// Created 2026-11-01 (T-016).
// Invariants: points never decrease while awake (decay is sleep,
// section 9, M2); inside-lair kills/eats gain exactly 0 by rule,
// enforced in PointsForKillAt, not by caller discipline.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NotorietySubsystem.generated.h"

UCLASS()
class UNDERTHEMOUNTAIN_API UNotorietySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-016] The lair volume for inside/outside calls:
	// the chamber chain plus hoard room (x -2500..21500, |y| <=
	// 2500, below the peak shaft top at 5200). Anything else -
	// village, terrain, sky - is outside, where witnesses are.
	static bool IsInsideLair(const FVector& Location);

	// [2026-11-01 T-016] Star bucket for a point total, per the
	// section 4 table (10/30/60/100/150 to enter 1-5 stars).
	static int32 StarsForPoints(int32 Points);

	// [2026-11-01 T-016] Kill/eat notoriety by location: outside
	// the lair a person is worth 10; inside, invisible, exactly 0.
	static int32 PointsForKillAt(const FVector& Location);

	int32 GetPoints() const;
	int32 GetStars() const;

	// [2026-11-01 T-016] Spec-table gains (section 4). Each logs
	// the new total so a session's notoriety story is readable
	// from the game log alone.
	void AddEscapePoints(bool bWithLoot);
	void AddPublicKillPoints(const FVector& KillLocation);
	void AddRoarOutsidePoints();
	void AddLivestockEatPoints();
	void AddStructureHitPoints();

	// [2026-11-01 T-016] Direct add for tests and future verbs;
	// negative adds clamp at 0 total (points never go negative,
	// and awake decay does not exist).
	void AddPoints(int32 Amount, const TCHAR* Reason);

  private:
	int32 Points = 0;
};
