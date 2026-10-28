// Copyright Under the Mountain contributors.
// Purpose: the playable dragon (walking skeleton, T-003). A placeholder mesh
// today; the Meshy dragon replaces BodyMesh in T-010 without touching movement
// code. Created 2026-10-27.
// [2026-10-28 T-011] Flight feel pass: per-mode stamina drain
// (dive/glide/hover), exhaustion HP burn, hard-landing damage on Landed,
// and an on-screen flight debug readout (all tuning in GAMEPLAY.md s2).
// Invariants: movement mode is the single source of "flying" truth; stamina
// and HP are only mutated through UDragonStatsComponent.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DragonCharacter.generated.h"

class UDragonStatsComponent;
class UScentComponent;
class UDragonAppearanceComponent;
class UDragonTintWidget;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UCameraComponent;
class UAnimSequence;
class UStaticMeshComponent;

UCLASS()
class UNDERTHEMOUNTAIN_API ADragonCharacter : public ACharacter
{
	GENERATED_BODY()

  public:
	ADragonCharacter();

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonStatsComponent> Stats;

	// [2026-10-28 T-013] The nose: owns scent ribbons + true-sight
	// markers driven by the treasure registry. The pawn only forwards
	// the toggle input; all scent rules live in the component.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UScentComponent> Scent;

	// [2026-10-28 T-027] Scale tint owner: applies the player's RGB
	// choice to the mesh material and persists it across sessions.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UDragonAppearanceComponent> Appearance;

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

	// [2026-10-28 T-032] Walk cycle (T-029) played single-node while
	// the pawn actually moves; nullptr if the asset failed to load,
	// in which case the mesh holds its standing bind pose.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> WalkAnim;

  protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	// [2026-10-28 T-011] Hard-landing damage: ACharacter::Landed fires
	// after the movement component has already zeroed vertical velocity,
	// so the impact speed comes from LastVelocityZ (stored every Tick),
	// never from a live read in here.
	virtual void Landed(const FHitResult& Hit) override;

  private:
	// [2026-10-27 T-003] Enhanced Input actions are instantiated in code for
	// the skeleton: one C++ source of truth, no hand-authored .uassets to
	// drift. Migration to data assets is a later decision, not an accident.
	TObjectPtr<UInputAction> MoveAction;
	TObjectPtr<UInputAction> LookAction;
	TObjectPtr<UInputAction> FlapAction;
	TObjectPtr<UInputAction> ToggleFlightAction;
	TObjectPtr<UInputAction> DiveAction;
	TObjectPtr<UInputAction> TrueSightAction;
	TObjectPtr<UInputAction> TintAction;
	TObjectPtr<UDragonTintWidget> TintWidget;
	bool bTintPickerOpen = false;
	TObjectPtr<UInputMappingContext> MappingContext;

	void BuildInputActions();
	void HandleMove(const struct FInputActionValue& Value);
	void HandleLook(const struct FInputActionValue& Value);
	void HandleFlap();
	void HandleToggleFlight();
	void HandleDive(const struct FInputActionValue& Value);
	// [2026-10-28 T-030] Dive release: Triggered alone never fires on key-up,
	// so bDiving latched forever; Completed/Canceled clear it.
	void HandleDiveEnd();
	// [2026-10-28 T-013] Seeing Stone toggle (key T / gamepad left face).
	void HandleTrueSight();
	// [2026-10-28 T-027] Scale-colour picker toggle (key C / gamepad right face).
	void HandleToggleTintPicker();
	bool bDiving = false;

	// [2026-10-28 T-032] True while the single-node walk cycle is
	// playing, so Tick only calls Play/Stop on transitions.
	bool bPlayingWalk = false;

	// [2026-10-28 T-011] Last Tick's vertical velocity (cm/s, signed).
	// Captured before Tick's flight logic runs so Landed() can recover
	// the impact speed the movement component has already discarded.
	float LastVelocityZ = 0.0f;
};
