// Copyright Under the Mountain contributors.
// [2026-10-28 T-013] UTreasureRegistry implementation: a thin,
// invariant-enforcing wrapper over one TMap. No world, no actors, no
// clock — every function is deterministic in its arguments so the
// automation tests stay hermetic (see Tests/TreasureScentTest.cpp).

#include "Treasure/TreasureRegistry.h"

FName UTreasureRegistry::Register(const FTreasureEntry& Entry)
{
	FTreasureEntry Stored = Entry;

	// [2026-10-28 T-013] Negative gold has no design meaning (see
	// UTreasureDataAsset); clamp here so direct C++ registrations get
	// the same guarantee editor-authored assets do.
	Stored.GoldValue = FMath::Max(0, Stored.GoldValue);
	if (!Stored.bCarried)
	{
		Stored.CarrierId = NAME_None;
	}

	if (Stored.Id == NAME_None)
	{
		// [2026-10-28 T-013] Generate until unique: the serial alone is
		// not enough because a caller may have registered a
		// "Treasure_<n>" id by hand first.
		do
		{
			++NextGeneratedSerial;
			Stored.Id = FName(*FString::Printf(TEXT("Treasure_%d"), NextGeneratedSerial));
		} while (EntryMap.Contains(Stored.Id));
	}

	EntryMap.Add(Stored.Id, Stored);
	return Stored.Id;
}

bool UTreasureRegistry::Unregister(FName Id)
{
	return EntryMap.Remove(Id) > 0;
}

bool UTreasureRegistry::SetCarried(FName Id, bool bCarried, FName Carrier)
{
	FTreasureEntry* Found = EntryMap.Find(Id);
	if (Found == nullptr)
	{
		return false;
	}
	Found->bCarried = bCarried;
	// [2026-10-28 T-013] A resting treasure has no carrier; clearing on
	// drop keeps stale thief ids out of true-sight/steal queries.
	Found->CarrierId = bCarried ? Carrier : NAME_None;
	return true;
}

bool UTreasureRegistry::UpdateLocation(FName Id, FVector Location)
{
	FTreasureEntry* Found = EntryMap.Find(Id);
	if (Found == nullptr)
	{
		return false;
	}
	Found->Location = Location;
	return true;
}

const FTreasureEntry* UTreasureRegistry::Find(FName Id) const
{
	return EntryMap.Find(Id);
}

int32 UTreasureRegistry::Count() const
{
	return EntryMap.Num();
}

int64 UTreasureRegistry::TotalGold() const
{
	int64 Total = 0;
	for (const TPair<FName, FTreasureEntry>& Pair : EntryMap)
	{
		Total += Pair.Value.GoldValue;
	}
	return Total;
}

const TMap<FName, FTreasureEntry>& UTreasureRegistry::Entries() const
{
	return EntryMap;
}
