// Copyright Under the Mountain contributors.
// Purpose: a one-shot floor trap placed by trap-capable thieves
// (dwarves): when the dragon steps in, it is slowed for a few seconds
// and the trap is spent. Created 2026-10-28 (T-014).
// Invariants: a trap fires at most once (bHasTriggered latches before
// Destroy); only the dragon triggers it — thieves and scenery walk
// over their own traps unharmed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrapActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API ATrapActor : public AActor
{
	GENERATED_BODY()

  public:
	ATrapActor();

	// [2026-10-28 T-014] Greybox visual: a small flattened sphere so a
	// placed trap reads as a floor disc before real trap meshes exist.
	UPROPERTY(VisibleAnywhere, Category = "Thief|Trap")
	TObjectPtr<UStaticMeshComponent> Visual;

	// [2026-10-28 T-014] Overlap volume (150 cm) that arms the trap.
	UPROPERTY(VisibleAnywhere, Category = "Thief|Trap")
	TObjectPtr<USphereComponent> Trigger;

	// [2026-10-28 T-014] Slow applied to the dragon on trigger,
	// snapshotted from the placing thief's data (UThiefBrain getters).
	UPROPERTY(EditAnywhere, Category = "Thief|Trap", meta = (ClampMin = "0", ClampMax = "1"))
	float SlowFactor = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Thief|Trap", meta = (ClampMin = "0"))
	float SlowSeconds = 4.0f;

	// [2026-10-28 T-014] Latched the moment the dragon springs the
	// trap; the actor then destroys itself, so this is observable only
	// within that same overlap dispatch.
	UPROPERTY(VisibleAnywhere, Category = "Thief|Trap")
	bool bHasTriggered = false;

  protected:
	virtual void BeginPlay() override;

  private:
	// [2026-10-28 T-014] Trigger handler: dragons get slowed and spend
	// the trap; every other actor is ignored and the trap stays armed.
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
							   bool bFromSweep, const FHitResult& SweepResult);
};
