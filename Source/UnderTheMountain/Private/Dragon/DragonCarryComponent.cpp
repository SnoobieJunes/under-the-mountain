// Copyright Under the Mountain contributors.
// [2026-11-04 T-070] UDragonCarryComponent implementation: every
// rule is a UTreasureRegistry call the tests can replay verbatim;
// the actor-follow presentation lives in the pawn.

#include "Dragon/DragonCarryComponent.h"
#include "UnderTheMountain.h"

#include "Treasure/TreasureRegistry.h"

UDragonCarryComponent::UDragonCarryComponent()
{
	// [2026-11-04 T-070] Never ticks, mirroring combat/stats: the
	// pawn owns the tick and calls SyncCarriedLocation, so a test
	// that never ticks a pawn still exercises the full rule set.
	PrimaryComponentTick.bCanEverTick = false;
}

FName UDragonCarryComponent::DragonCarrierId()
{
	return FName(TEXT("Dragon"));
}

bool UDragonCarryComponent::TryPickup(UTreasureRegistry* Registry, FName TreasureId,
									  const FVector& DragonLocation)
{
	if (Registry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("DragonCarry TryPickup refused: registry is null"));
		return false;
	}
	if (IsCarrying())
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCarry TryPickup refused: already carrying %s (one item at a time)"),
			   *CarriedTreasureId.ToString());
		return false;
	}
	const FTreasureEntry* Entry = Registry->Find(TreasureId);
	if (Entry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("DragonCarry TryPickup refused: unknown id %s"),
			   *TreasureId.ToString());
		return false;
	}
	if (Entry->bCarried)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCarry TryPickup refused: %s is already carried by %s"),
			   *TreasureId.ToString(), *Entry->CarrierId.ToString());
		return false;
	}
	if (FVector::Dist(DragonLocation, Entry->Location) > PickupRange)
	{
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCarry TryPickup refused: %s is beyond the %.0fcm pickup range"),
			   *TreasureId.ToString(), PickupRange);
		return false;
	}
	if (!Registry->SetCarried(TreasureId, true, DragonCarrierId()))
	{
		return false;
	}
	CarriedTreasureId = TreasureId;
	SyncCarriedLocation(Registry, DragonLocation);
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon picked up %s (%d gold)"), *TreasureId.ToString(),
		   Entry->GoldValue);
	return true;
}

bool UDragonCarryComponent::SyncCarriedLocation(UTreasureRegistry* Registry,
												const FVector& DragonLocation)
{
	if (!IsCarrying())
	{
		return false;
	}
	if (Registry == nullptr || !Registry->UpdateLocation(CarriedTreasureId, DragonLocation))
	{
		// [2026-11-04 T-070] The entry vanished mid-carry (actor
		// destroyed/unregistered): claws are empty in truth, so
		// say so and stop claiming to hold it.
		UE_LOG(LogUnderTheMountain, Warning,
			   TEXT("DragonCarry: carried entry %s is gone - claws cleared"),
			   *CarriedTreasureId.ToString());
		CarriedTreasureId = NAME_None;
		return false;
	}
	return true;
}

bool UDragonCarryComponent::DropTreasure(UTreasureRegistry* Registry, const FVector& DropLocation,
										 int32& OutGoldValue)
{
	OutGoldValue = 0;
	if (!IsCarrying())
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("DragonCarry drop refused: claws are empty"));
		return false;
	}
	if (Registry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("DragonCarry drop refused: registry is null"));
		return false;
	}
	const FTreasureEntry* Entry = Registry->Find(CarriedTreasureId);
	if (Entry == nullptr)
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("DragonCarry drop: entry %s is gone"),
			   *CarriedTreasureId.ToString());
		CarriedTreasureId = NAME_None;
		return false;
	}
	OutGoldValue = Entry->GoldValue;
	const FName DroppedId = CarriedTreasureId;
	// [2026-11-04 T-070] Land first, then let go: the entry's
	// location is the drop point before the carrier clears, so no
	// tick ever reads a resting item at the dragon's old position.
	Registry->UpdateLocation(DroppedId, DropLocation);
	if (!Registry->SetCarried(DroppedId, false, NAME_None))
	{
		return false;
	}
	CarriedTreasureId = NAME_None;
	UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon dropped %s (%d gold) at %s"),
		   *DroppedId.ToString(), OutGoldValue, *DropLocation.ToString());
	return true;
}

bool UDragonCarryComponent::DepositTreasure(UTreasureRegistry* Registry,
											const FVector& HoardLocation, int32& OutGoldValue)
{
	// [2026-11-04 T-070] A deposit IS a drop, at the hoard: the
	// separate verb exists so the pawn's log/HUD can say "deposited"
	// and tests can read the hoard delta across it (HoardGoldAt
	// before counts the carried item nowhere; after, here).
	return DropTreasure(Registry, HoardLocation, OutGoldValue);
}

int64 UDragonCarryComponent::HoardGoldAt(const UTreasureRegistry* Registry, const FVector& Centre,
										 float Radius)
{
	if (Registry == nullptr)
	{
		return 0;
	}
	int64 Total = 0;
	for (const TPair<FName, FTreasureEntry>& Pair : Registry->Entries())
	{
		if (!Pair.Value.bCarried && FVector::Dist(Centre, Pair.Value.Location) <= Radius)
		{
			Total += Pair.Value.GoldValue;
		}
	}
	return Total;
}
