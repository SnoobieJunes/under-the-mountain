// Copyright Under the Mountain contributors.
// Purpose: the villager in the world (T-053) - a skeletal-mesh
// civilian who wanders/idles around a home point and flees the
// dragon, with every decision delegated to UVillagerBrain (the
// testable half). Visual decision, recorded 2026-11-01: villagers
// reuse the Meshy burglar rig + walk/run clips (a Meshy humanoid
// already in the repo from T-044) rather than spending Meshy
// credits on a dedicated villager generation; if the silhouette
// reads too thief-like in review, a villager-specific generation
// is a follow-up, not a blocker.
// Created 2026-11-01 (T-053).
// Invariants: the actor owns position, home and timers only; it
// never carries, grabs or takes damage (T-016 owns witness rules).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NPC/VillagerBrain.h"
#include "VillagerActor.generated.h"

class USkeletalMeshComponent;
class UVillagerBrain;

UCLASS()
class UNDERTHEMOUNTAIN_API AVillagerActor : public AActor
{
	GENERATED_BODY()

  public:
	AVillagerActor();

	// [2026-11-01 T-053] Wander/idle/flee steering. Movement is a
	// flat steer toward the current target at the brain's speed;
	// Z never changes after spawn (settlements are grounded by
	// their placement script, slopes are walked as flat).
	virtual void Tick(float DeltaSeconds) override;

	// [2026-11-01 T-053] Pin the wander home (placement scripts use
	// this so BeginPlay's capture cannot move it). Also honoured
	// after BeginPlay - the pin is the contract, not the timing.
	// UFUNCTION because placement scripts (Python) can only call
	// reflected methods (proven by create_npcs.py's first run).
	UFUNCTION(BlueprintCallable, Category = "NPC")
	void SetHomeLocation(const FVector& NewHomeLocation);

	FVector GetHomeLocation() const;
	EVillagerState GetState() const;

  protected:
	virtual void BeginPlay() override;

  private:
	// [2026-11-01 T-053] Pick a wander target (or choose Idle)
	// around home and reset the decision timer.
	void PickNewBehaviour();

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "NPC")
	TObjectPtr<UVillagerBrain> Brain;

	// [2026-11-01 T-053] Locomotion clips, loaded in the
	// constructor (FObjectFinder is constructor-only). UPROPERTY
	// so the GC keeps them reachable.
	UPROPERTY()
	TObjectPtr<class UAnimSequence> WalkAnim;

	UPROPERTY()
	TObjectPtr<class UAnimSequence> RunAnim;

	// [2026-11-01 T-053] Wander home + current destination, both
	// flat-steered; Destination equals location while Idle/Flee
	// (flee recomputes its heading from the dragon each tick).
	FVector HomeLocation = FVector::ZeroVector;
	bool bHomeLocationOverridden = false;
	FVector Destination = FVector::ZeroVector;
	EVillagerState State = EVillagerState::Idle;
	float DecisionTimer = 0.0f;

	// [2026-11-01 T-053] Wander radius around home, cm.
	UPROPERTY(EditAnywhere, Category = "NPC|Villager")
	float WanderRadius = 700.0f;
};
