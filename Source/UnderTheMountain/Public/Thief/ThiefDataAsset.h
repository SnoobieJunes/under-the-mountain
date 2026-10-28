// Copyright Under the Mountain contributors.
// Purpose: authored definition of one thief type (name, toughness,
// speeds, carrying capacity, roar-drop behaviour and trap toolkit) as
// a primary data asset, so burglar/adventurer/dwarf variants are tuned
// in data while UThiefBrain holds the runtime state. Created
// 2026-10-28 (T-014).
// Invariants: this asset holds no runtime state — HP, carried loot and
// the state machine live in UThiefBrain; defaults here are the unnamed
// baseline thief and every authored variant overrides only what makes
// it distinct.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ThiefDataAsset.generated.h"

// [2026-10-28 T-014] UPrimaryDataAsset (not plain UDataAsset) so thief
// definitions are addressable by the Asset Manager under the "Thief"
// primary asset type, mirroring UTreasureDataAsset's seam.
UCLASS(BlueprintType)
class UNDERTHEMOUNTAIN_API UThiefDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

  public:
	// [2026-10-28 T-014] Human-readable name for HUD/log use; runtime
	// identity is the thief actor's registry id, never this string.
	UPROPERTY(EditAnywhere, Category = "Thief")
	FString DisplayName;

	// [2026-10-28 T-014] Hit points the brain starts with; at 0 the
	// thief dies and drops everything (see UThiefBrain::ApplyDamage).
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "1"))
	float MaxHP = 20.0f;

	// [2026-10-28 T-014] Walk speed (cm/s) while sneaking toward and
	// grabbing treasure; the quiet pace the dragon may not notice.
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "0"))
	float SneakSpeed = 220.0f;

	// [2026-10-28 T-014] Walk speed (cm/s) while fleeing or scared —
	// carrying loot to the exit is a run, not a sneak.
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "0"))
	float FleeSpeed = 520.0f;

	// [2026-10-28 T-014] Seconds spent stationary in the Grab state
	// before the target treasure becomes carried loot.
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "0"))
	float GrabTimeSeconds = 1.5f;

	// [2026-10-28 T-014] How many treasures this thief carries before
	// it heads for the exit (burglar 1; adventurers carry more).
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "1"))
	int32 CarriedCap = 1;

	// [2026-10-28 T-014] Roar-drop rule: true (adventurer/burglar)
	// drops every carried item when the dragon roars; false (dwarf)
	// drops exactly one — dwarves do not let go of gold easily.
	UPROPERTY(EditAnywhere, Category = "Thief")
	bool bDropsAllLootOnRoar = false;

	// [2026-10-28 T-014] Whether this thief may place traps that slow
	// the dragon (dwarves do; burglars/adventurers do not).
	UPROPERTY(EditAnywhere, Category = "Thief")
	bool bCanPlaceTraps = false;

	// [2026-10-28 T-014] Trap budget for one thief's run at the hoard.
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "0"))
	int32 MaxTraps = 3;

	// [2026-10-28 T-014] Multiplier a triggered trap applies to the
	// dragon's speed and how long it lasts (seconds); ATrapActor
	// snapshots these when the trap is placed.
	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "0", ClampMax = "1"))
	float TrapSlowFactor = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Thief", meta = (ClampMin = "0"))
	float TrapSlowSeconds = 4.0f;

	// [2026-10-28 T-014] Placeholder visual scale applied to the
	// character's greybox body so a squat dwarf reads differently from
	// a tall adventurer before real meshes exist.
	UPROPERTY(EditAnywhere, Category = "Thief")
	FVector BodyScale = FVector(1.0f, 1.0f, 1.0f);

	// [2026-10-28 T-014] Primary asset id under the shared "Thief"
	// type, keyed by asset name so the Asset Manager can enumerate
	// every thief definition without loading the whole Content tree.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
