// Copyright Under the Mountain contributors.
// Purpose: a placeable treasure in the world. On BeginPlay it turns its
// authored UTreasureDataAsset into a UTreasureRegistry entry; on
// EndPlay it removes that entry, so the registry always mirrors the
// live level. Created 2026-10-28 (T-013).
// Invariants: at most one registry entry per actor, identified by
// RegisteredId (NAME_None until registered and after EndPlay); an
// actor with no data asset registers nothing and logs a warning —
// a misconfigured placement must be visible, never silent.
// [2026-10-28 T-038] Location freshness is the actor's own job: it
// ticks and pushes its current location into the registry whenever
// it moves, so a carried/repositioned treasure's scent endpoint
// follows it instead of going stale at the spawn point; a failed
// registration is logged as FAILED and leaves RegisteredId
// NAME_None, never reported as registered.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TreasureActor.generated.h"

class UStaticMeshComponent;
class UTreasureDataAsset;
class UTreasureRegistry;

UCLASS()
class UNDERTHEMOUNTAIN_API ATreasureActor : public AActor
{
	GENERATED_BODY()

  public:
	ATreasureActor();

	// [2026-10-28 T-013] Placeholder sphere body (engine basic shape,
	// set in the constructor) until per-treasure Meshy meshes arrive;
	// swapping the mesh never touches registration logic.
	UPROPERTY(VisibleAnywhere, Category = "Treasure")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// [2026-10-28 T-013] Authored definition this placement
	// instantiates. Required: BeginPlay warns and registers nothing
	// while it is unset.
	UPROPERTY(EditAnywhere, Category = "Treasure")
	TObjectPtr<UTreasureDataAsset> TreasureData;

	// [2026-10-28 T-013] Id returned by the registry at BeginPlay;
	// VisibleAnywhere so designers can cross-check a placement against
	// scent/debug output in the editor.
	UPROPERTY(VisibleAnywhere, Category = "Treasure")
	FName RegisteredId = NAME_None;

	// [2026-10-28 T-038] Registration seam, public so hermetic tests
	// (and any future non-world caller) can register against a bare
	// UTreasureRegistry without a world/subsystem. Builds the entry
	// from TreasureData + the CURRENT actor location, registers it,
	// stores that location as LastRegisteredLocation, and returns
	// the stored id. Returns NAME_None — with a warning log — when
	// Registry is null or TreasureData is unset, and leaves
	// RegisteredId NAME_None in that case. On success RegisteredId
	// and the return value are the registry-issued id.
	FName RegisterWithRegistry(UTreasureRegistry* Registry);

	// [2026-10-28 T-038] Location-freshness seam (Tick calls this).
	// Returns false when Registry is null or this actor has no
	// RegisteredId. Otherwise, if the actor's current location
	// differs from LastRegisteredLocation it calls
	// Registry->UpdateLocation and, on success, advances
	// LastRegisteredLocation. Returns whether the stored location
	// is now current (true also when nothing moved).
	bool RefreshRegisteredLocation(UTreasureRegistry* Registry);

  protected:
	// [2026-10-28 T-013] Builds an FTreasureEntry from TreasureData and
	// this actor's location, then registers it with the world's
	// UTreasureSubsystem. A missing data asset or missing subsystem is
	// logged as a warning and leaves RegisteredId NAME_None.
	// [2026-10-28 T-038] A re-BeginPlay first unregisters any earlier
	// entry from the CURRENT world's subsystem so it is never
	// orphaned; a NAME_None result is logged as a FAILED warning and
	// the "Treasure registered:" line is printed only on success.
	virtual void BeginPlay() override;

	// [2026-10-28 T-038] Keeps the registry's location snapshot
	// fresh: resolves the current world's subsystem registry and
	// calls RefreshRegisteredLocation, every hop null-guarded. A
	// moved/carried treasure's scent endpoint therefore follows it.
	virtual void Tick(float DeltaSeconds) override;

	// [2026-10-28 T-013] Unregisters this actor's entry (if any) so a
	// destroyed treasure immediately stops emitting scent.
	// [2026-10-28 T-038] Teardown-tolerant (a dying world/subsystem
	// just clears the id), but a reachable subsystem reporting the
	// entry already gone is warned about, not swallowed.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  private:
	// [2026-10-28 T-038] The location last known to be stored in the
	// registry for RegisteredId, and whether any location has been
	// stored yet. RefreshRegisteredLocation compares against this so
	// a resting treasure makes no redundant registry writes.
	FVector LastRegisteredLocation = FVector::ZeroVector;
	bool bHasRegisteredLocation = false;
};
