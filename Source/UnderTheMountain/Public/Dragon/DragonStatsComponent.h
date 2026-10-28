// Copyright Under the Mountain contributors.
// Purpose: dragon resources (stamina first) behind one component so flight,
// combat and UI never reach into each other. Created 2026-10-27 (T-003).
// Invariants: stamina is always clamped to [0, MaxStamina]; only this
// component writes stamina — the pawn asks, never sets (PLAN.md seams).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DragonStatsComponent.generated.h"

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonStatsComponent : public UActorComponent
{
	GENERATED_BODY()

  public:
	UDragonStatsComponent();

	// [2026-10-27 T-003] Flight/flap tuning lives on the component so T-011
	// feel work tweaks data, not pawn code.
	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float FlyingDrainPerSecond = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float GroundRegenPerSecond = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Stats")
	float FlapStaminaCost = 10.0f;

	UPROPERTY(VisibleAnywhere, Category = "Dragon|Stats")
	float Stamina;

	// [2026-10-27 T-003] Pure stamina math, owner-free on purpose: automation
	// tests exercise these directly without spawning a world.
	bool TryConsumeStamina(float Cost);
	void ApplyStaminaDelta(float Delta);
	void DrainForFlight(float DeltaSeconds);
	void RegenOnGround(float DeltaSeconds);
	bool CanFlap() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;
};
