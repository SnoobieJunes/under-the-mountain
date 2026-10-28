// Copyright Under the Mountain contributors.
// Purpose: runtime record of every treasure in play (identity, gold
// value, location, carried state) behind one plain UObject, so scent,
// hoard totals and true-sight all read the same truth without needing
// a world. Created 2026-10-28 (T-013).
// Invariants: ids are unique keys of the entry map; registered gold
// values are clamped to >= 0 at Register time; an entry that is not
// carried always has CarrierId NAME_None.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TreasureRegistry.generated.h"

// [2026-10-28 T-013] One registered treasure. A plain struct (not a
// USTRUCT) on purpose: registry math is hermetic C++ exercised by
// automation tests with no reflection involved. Location is a snapshot
// the owning actor refreshes via UpdateLocation; scent rendering reads
// it but never feeds it back into ribbon width/opacity (the
// no-information rule, docs/GAMEPLAY.md section 3).
struct UNDERTHEMOUNTAIN_API FTreasureEntry
{
	FName Id = NAME_None;
	int32 GoldValue = 0;
	FVector Location = FVector::ZeroVector;
	bool bCarried = false;
	FName CarrierId = NAME_None;
	bool bIsSeeingStone = false;
};

// [2026-10-28 T-013] The registry itself. Pure UObject with no world
// dependency: tests NewObject it directly, while UTreasureSubsystem
// owns the in-game instance. All lookups of an unknown id fail
// explicitly (false/nullptr) rather than fabricating an entry.
UCLASS()
class UNDERTHEMOUNTAIN_API UTreasureRegistry : public UObject
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-013] Registers a copy of Entry. If Entry.Id is
	// NAME_None a unique "Treasure_<n>" id is generated. If the id
	// already exists the stored entry is replaced (re-registration is
	// the actor respawn path, not an error). GoldValue is clamped to
	// >= 0 and CarrierId is cleared while not carried, enforcing the
	// header invariants for every caller. Returns the stored id
	// (never NAME_None).
	FName Register(const FTreasureEntry& Entry);

	// [2026-10-28 T-013] Removes an entry. Returns false when the id is
	// unknown so double-unregister bugs surface instead of passing
	// silently.
	bool Unregister(FName Id);

	// [2026-10-28 T-013] Marks an entry carried (stolen) or resting.
	// Carrier is recorded only while carried; dropping clears it.
	// Returns false for an unknown id.
	bool SetCarried(FName Id, bool bCarried, FName Carrier);

	// [2026-10-28 T-013] Refreshes the location snapshot. Returns
	// false for an unknown id.
	bool UpdateLocation(FName Id, FVector Location);

	// [2026-10-28 T-013] Finds an entry by id, or nullptr when absent.
	// The pointer is invalidated by the next mutating call.
	const FTreasureEntry* Find(FName Id) const;

	int32 Count() const;

	// [2026-10-28 T-013] Sum of every registered GoldValue. int64 so a
	// large hoard of crown-class pieces cannot overflow the total the
	// way per-entry int32 values could in aggregate.
	int64 TotalGold() const;

	// [2026-10-28 T-013] Read-only view of the whole map for systems
	// (scent) that must iterate every treasure each tick.
	const TMap<FName, FTreasureEntry>& Entries() const;

  private:
	TMap<FName, FTreasureEntry> EntryMap;

	// [2026-10-28 T-013] Serial for generated ids; combined with a
	// contains-check loop so a generated id can never collide with a
	// caller-supplied one.
	int32 NextGeneratedSerial = 0;
};
