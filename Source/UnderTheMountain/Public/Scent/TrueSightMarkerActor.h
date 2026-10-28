// Copyright Under the Mountain contributors.
// Purpose: the exact-position marker true-sight reveals at a treasure's
// location (docs/GAMEPLAY.md section 3). Visibility only — what it
// marks and when is UScentComponent's decision. Created
// 2026-10-28 (T-013).
// Invariants: the marker starts hidden and is only ever shown by an
// explicit SetMarkerVisible(true); it carries no treasure data, so it
// can never mark anything that did not come from the registry.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrueSightMarkerActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API ATrueSightMarkerActor : public AActor
{
	GENERATED_BODY()

  public:
	ATrueSightMarkerActor();

	// [2026-10-28 T-013] Placeholder sphere body (engine basic shape,
	// set in the constructor) until the true-sight marker gets its
	// real art.
	UPROPERTY(VisibleAnywhere, Category = "Scent")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// [2026-10-28 T-013] Shows or hides the whole actor in game
	// (SetActorHiddenInGame), so a hidden marker neither renders nor
	// leaks a silhouette while true-sight is off.
	void SetMarkerVisible(bool bVisible);
};
