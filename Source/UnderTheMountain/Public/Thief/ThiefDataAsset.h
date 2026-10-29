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

class UAnimSequence;
class USkeletalMesh;

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

	// [2026-10-28 T-044] Visual scale: applied to the skeletal mesh
	// when MeshAsset is set (below), else to the greybox Body sphere
	// as it was under T-014, so a squat dwarf (0.9 in its data) reads
	// differently from a tall adventurer either way.
	UPROPERTY(EditAnywhere, Category = "Thief")
	FVector BodyScale = FVector(1.0f, 1.0f, 1.0f);

	// [2026-10-28 T-044] Visuals are data per type so burglar/
	// adventurer/dwarf stay one class: the Meshy skeletal mesh this
	// type shows (Content/Meshy/Thieves/<slug>_rigged), assigned by
	// Scripts/create_thieves.py. Null keeps the greybox sphere.
	UPROPERTY(EditAnywhere, Category = "Thief|Visuals")
	TObjectPtr<USkeletalMesh> MeshAsset;

	// [2026-10-28 T-044] Looping single-node locomotion for MeshAsset:
	// WalkAnim at sneak/grab pace, RunAnim while fleeing or scared.
	// The Meshy walk/run sequences are bound to their own Skeleton
	// assets, so playback relies on UE skeleton compatibility with
	// the mesh's skeleton; either may stay null for a type, in which
	// case that pace holds bind pose.
	UPROPERTY(EditAnywhere, Category = "Thief|Visuals")
	TObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditAnywhere, Category = "Thief|Visuals")
	TObjectPtr<UAnimSequence> RunAnim;

	// [2026-10-28 T-014] Primary asset id under the shared "Thief"
	// type, keyed by asset name so the Asset Manager can enumerate
	// every thief definition without loading the whole Content tree.
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
