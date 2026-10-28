// Copyright Under the Mountain contributors.
// Purpose: the playable dragon (walking skeleton, T-003). A placeholder mesh
// today; the Meshy dragon replaces BodyMesh in T-010 without touching movement
// code. Created 2026-10-27.
// Invariants: movement mode is the single source of "flying" truth; stamina
// is only mutated through UDragonStatsComponent.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DragonCharacter.generated.h"

class UDragonStatsComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API ADragonCharacter : public ACharacter
{
	GENERATED_BODY()

  public:
	ADragonCharacter();

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonStatsComponent> Stats;

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// [2026-10-27 T-003] Visible placeholder body (engine sphere, stretched).
	// The skeletal mesh stays hidden; T-010 swaps this for the Meshy dragon.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UStaticMeshComponent> PlaceholderBody;

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UCameraComponent> Camera;

	// [2026-10-27 T-003] Flight tuning for the skeleton; T-011 owns feel.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float FlapUpwardSpeed = 650.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float DiveAcceleration = 2400.0f;

	bool IsFlying() const;

  protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

  private:
	// [2026-10-27 T-003] Enhanced Input actions are instantiated in code for
	// the skeleton: one C++ source of truth, no hand-authored .uassets to
	// drift. Migration to data assets is a later decision, not an accident.
	TObjectPtr<UInputAction> MoveAction;
	TObjectPtr<UInputAction> LookAction;
	TObjectPtr<UInputAction> FlapAction;
	TObjectPtr<UInputAction> ToggleFlightAction;
	TObjectPtr<UInputAction> DiveAction;
	TObjectPtr<UInputMappingContext> MappingContext;

	void BuildInputActions();
	void HandleMove(const struct FInputActionValue& Value);
	void HandleLook(const struct FInputActionValue& Value);
	void HandleFlap();
	void HandleToggleFlight();
	void HandleDive(const struct FInputActionValue& Value);
	bool bDiving = false;
};
