// Copyright Under the Mountain contributors.
// Purpose: pick up, carry, drop, and deposit treasure (T-070) —
// pillar 4 made literal. The registry already modelled carried
// state for thieves (T-013/T-014); this component is the
// dragon-as-carrier half, with every rule over a plain
// UTreasureRegistry so pickup->carry->drop is provable headless.
// Created 2026-11-04 (T-070).
// Invariants: at most one item is dragon-carried at a time
// (CarriedTreasureId NAME_None = empty claws); while carried, the
// registry entry is bCarried with CarrierId DragonCarrierId() and
// its location follows the dragon via SyncCarriedLocation (called
// by the pawn every tick — the T-038 freshness rule, same as a
// thief brain refreshing its loot); a drop/deposit always clears
// the carrier and lands the entry at the drop location, so scent's
// endpoint moves to where the item actually rests.
// [2026-11-04 T-070] Delegated deep-dive decisions: one item at a
// time (a second pickup is refused, never a swap — swapping would
// silently drop loot mid-flight); carry works flying or grounded
// at no stamina/speed cost for now (value/weight pricing is a
// tuning question with no weight field on FTreasureEntry — adding
// one is a data task, not guessed here); pickup range 500 cm
// (just beyond claw's 450, so anything clawable is grabbable);
// the item rides at a follow offset in the pawn, NOT socket-
// attached — the v6 rig has no authored claw socket, and a
// guessed socket would bury the item in the chest (the pawn sets
// the actor location each tick; ATreasureActor's own tick then
// keeps the registry fresh, this component mirrors it directly so
// the registry is truthful even before the actor ticks).
// [2026-11-04 T-070] Deposit vs the session ledger: dropping
// inside the lair (the pawn decides via UNotorietySubsystem::
// IsInsideLair) is a deposit — the item joins the hoard where
// HoardGoldAt counts it. UHoardSubsystem's session total stays
// registry-total-minus-stolen (T-016, untouched): a dragon-carried
// item was never unregistered, so the session total never lies
// either — what deposit changes is the at-hoard gold, which is
// the number HoardGoldAt and the mound already care about.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DragonCarryComponent.generated.h"

class UTreasureRegistry;

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonCarryComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UDragonCarryComponent();

	// [2026-11-04 T-070] The carrier id written into the registry
	// while the dragon holds an item. Thieves write their actor
	// name; the dragon is one actor with one stable name for this.
	static FName DragonCarrierId();

	// [2026-11-04 T-070] How close (cm, 3D) the dragon must be to
	// grab. Enforced here (registry location vs dragon location),
	// so a lock alone never teleports loot into the claws.
	UPROPERTY(EditAnywhere, Category = "Dragon|Carry")
	float PickupRange = 500.0f;

	bool IsCarrying() const
	{
		return CarriedTreasureId != NAME_None;
	}
	FName CarriedTreasureIdValue() const
	{
		return CarriedTreasureId;
	}

	// [2026-11-04 T-070] The grab: refuses (false, Warning logged)
	// when claws are full, the registry/id is unknown, the item is
	// already carried by anyone, or it is out of PickupRange. On
	// success the entry is carried-by-dragon at the dragon's
	// location. No actor knowledge — the pawn finds the actor.
	bool TryPickup(UTreasureRegistry* Registry, FName TreasureId, const FVector& DragonLocation);

	// [2026-11-04 T-070] Per-tick carry truth: the carried entry's
	// registry location becomes the dragon's (ride offset is the
	// pawn's presentation; the registry tracks the carrier, the
	// same way thief loot tracks the thief). False when empty or
	// the entry vanished (claws are then cleared — see .cpp).
	bool SyncCarriedLocation(UTreasureRegistry* Registry, const FVector& DragonLocation);

	// [2026-11-04 T-070] Put it down where the dragon is: entry
	// rests (uncarried, no carrier) at DropLocation and the gold
	// value is returned (0 when nothing was carried, with the
	// refusal logged). Deposit is this same landing inside the
	// hoard — DepositTreasure only adds the hoard reading.
	bool DropTreasure(UTreasureRegistry* Registry, const FVector& DropLocation, int32& OutGoldValue);
	bool DepositTreasure(UTreasureRegistry* Registry, const FVector& HoardLocation,
						 int32& OutGoldValue);

	// [2026-11-04 T-070] Gold physically at the hoard: resting
	// (uncarried) entries within Radius of Centre. Carried items —
	// the dragon's included — count nowhere until they land,
	// which is exactly what a deposit changes. Pure over the
	// registry; the pawn passes the drop point as Centre.
	static int64 HoardGoldAt(const UTreasureRegistry* Registry, const FVector& Centre, float Radius);

  private:
	// [2026-11-04 T-070] The one carried item (header invariant).
	// Plain FName, not a UPROPERTY: the registry entry is the
	// truth, this is only the pawn-side handle to it.
	FName CarriedTreasureId = NAME_None;
};
