// Copyright Under the Mountain contributors.
// Purpose: a placeable treasure in the world. On BeginPlay it turns its
// authored UTreasureDataAsset into a UTreasureRegistry entry; on
// EndPlay it removes that entry, so the registry always mirrors the
// live level. Created 2026-10-28 (T-013).
// Invariants: at most one registry entry per actor, identified by
// RegisteredId (NAME_None until registered and after EndPlay); an
// actor with no data asset registers nothing and logs a warning —
// a misconfigured placement must be visible, never silent.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TreasureActor.generated.h"

class UStaticMeshComponent;
class UTreasureDataAsset;

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

  protected:
	// [2026-10-28 T-013] Builds an FTreasureEntry from TreasureData and
	// this actor's location, then registers it with the world's
	// UTreasureSubsystem. A missing data asset or missing subsystem is
	// logged as a warning and leaves RegisteredId NAME_None.
	virtual void BeginPlay() override;

	// [2026-10-28 T-013] Unregisters this actor's entry (if any) so a
	// destroyed treasure immediately stops emitting scent.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
