// Copyright Under the Mountain contributors.
// Purpose: the visible scent ribbon — one Niagara system
// (NS_ScentRibbon) whose emitter sweeps from the dragon's nose to a
// treasure, laying location-based ribbon particles along the line.
// Appearance decisions (width, opacity) come from FScentMath via
// UScentComponent. Created 2026-10-28 (T-013); cube placeholder
// replaced by the Niagara system in T-031 (2026-10-28).
// Invariants: ribbon width is always the FScentMath width handed to
// SetEndpoints (pushed to the User.RibbonWidth Niagara variable);
// the actor never collides (a smell is not an obstacle).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScentRibbonActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class UNDERTHEMOUNTAIN_API AScentRibbonActor : public AActor
{
	GENERATED_BODY()

  public:
	AScentRibbonActor();

	// [2026-10-28 T-031] The Niagara component that renders the
	// ribbon. Swept between the endpoints by Tick; the system's
	// ribbon particles are location-based, so the sweep path is the
	// ribbon.
	UPROPERTY(VisibleAnywhere, Category = "Scent")
	TObjectPtr<UNiagaraComponent> FX;

	// [2026-10-28 T-013] Last opacity factor handed to
	// SetOpacityFactor, kept VisibleAnywhere so true-sight dimming
	// can be inspected on a live ribbon in the editor.
	UPROPERTY(VisibleAnywhere, Category = "Scent")
	float VisibleOpacity = 1.0f;

	// [2026-10-28 T-031] Sweep speed (cm/s) of the emitter along the
	// nose->treasure line. Fast enough that the 0.6 s particle
	// lifetime always covers the full segment on lair scales, slow
	// enough to read as flowing scent rather than a laser.
	UPROPERTY(EditAnywhere, Category = "Scent")
	float SweepSpeed = 6000.0f;

	// [2026-10-28 T-031] Records the ribbon's segment and width:
	// stores From/To for the sweep and pushes WidthCm to the
	// system's User.RibbonWidth variable. Coincident endpoints park
	// the emitter at From (zero-length scent, dragon on the gold).
	void SetEndpoints(const FVector& From, const FVector& To, float WidthCm);

	// [2026-10-28 T-031] Stores the opacity factor and pushes it to
	// User.RibbonOpacity, which the system's ScaleColor module reads
	// as its alpha scale; before the system exists it is stored only.
	void SetOpacityFactor(float Opacity);

	// [2026-10-28 T-031] Current ribbon width in cm (the last value
	// pushed to User.RibbonWidth); tests and PIE logs read this.
	float CurrentWidth() const
	{
		return RibbonWidthCm;
	}

  protected:
	// [2026-10-28 T-031] Loads NS_ScentRibbon (authored via the
	// Niagara toolsets) and activates it. A missing system logs a
	// warning — errors surfaced, never swallowed — and the ribbon
	// simply renders nothing.
	virtual void BeginPlay() override;

	// [2026-10-28 T-031] Ping-pongs the Niagara component along the
	// segment; turning points are the nose and the treasure, so the
	// ribbon always spans the full scent line.
	virtual void Tick(float DeltaSeconds) override;

  private:
	UPROPERTY()
	TObjectPtr<UNiagaraSystem> RibbonSystem;

	FVector SegmentFrom = FVector::ZeroVector;
	FVector SegmentTo = FVector::ZeroVector;
	float RibbonWidthCm = 0.0f;
	// [2026-10-28 T-031] Sweep phase in [0,1] along the segment and
	// travel direction (+1 nose->treasure, -1 back).
	float SweepPhase = 0.0f;
	float SweepDirection = 1.0f;
};
