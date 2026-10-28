// Copyright Under the Mountain contributors.
// Purpose: the visible scent ribbon — one stretched placeholder cube
// drawn from the dragon to a treasure. Geometry only: width/opacity
// decisions come from FScentMath via UScentComponent. Created
// 2026-10-28 (T-013).
// Invariants: the actor's +X axis always points from the "from"
// endpoint to the "to" endpoint and its origin sits at their midpoint;
// the mesh never collides (a smell is not an obstacle).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScentRibbonActor.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API AScentRibbonActor : public AActor
{
	GENERATED_BODY()

  public:
	AScentRibbonActor();

	// [2026-10-28 T-013] The engine-cube body (100 cm unit cube, set in
	// the constructor) that SetEndpoints stretches into a ribbon. A
	// ribbon mesh/Niagara system replaces it later without changing
	// the endpoint maths.
	UPROPERTY(VisibleAnywhere, Category = "Scent")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// [2026-10-28 T-013] Last opacity factor handed to
	// SetOpacityFactor, kept VisibleAnywhere so true-sight dimming can
	// be inspected on a live ribbon in the editor.
	UPROPERTY(VisibleAnywhere, Category = "Scent")
	float VisibleOpacity = 1.0f;

	// [2026-10-28 T-013] Places the ribbon between two world points:
	// origin at the midpoint, +X oriented along (To - From), scaled
	// (length/100, WidthCm/100, WidthCm/100) because the engine cube
	// is 100 cm. Coincident endpoints collapse the length scale to 0
	// at From rather than producing a NaN rotation.
	void SetEndpoints(const FVector& From, const FVector& To, float WidthCm);

	// [2026-10-28 T-013] Stores the opacity factor and pushes it to
	// the dynamic material instance's "Opacity" scalar when one
	// exists. Safe before BeginPlay and with no material assigned:
	// the stored value is applied when the instance is created.
	void SetOpacityFactor(float Opacity);

  protected:
	// [2026-10-28 T-013] Creates the dynamic material instance from
	// the mesh's material 0 (if any) so per-ribbon opacity never
	// writes into a shared material asset, then applies the stored
	// VisibleOpacity.
	virtual void BeginPlay() override;

  private:
	// [2026-10-28 T-013] Per-ribbon material instance; null when the
	// mesh has no material, in which case opacity is stored only.
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
