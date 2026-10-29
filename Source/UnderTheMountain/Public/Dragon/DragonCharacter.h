// Copyright Under the Mountain contributors.
// Purpose: the playable dragon (walking skeleton, T-003). Created 2026-10-27.
// [2026-10-28 T-041] The visible body is SK_Dragon_v4 on the inherited
// skeletal mesh (visible since T-032, set up in the constructor);
// movement/collision stay on the capsule and movement component, so the
// mesh is display only. There is no placeholder body any more.
// [2026-10-28 T-011] Flight feel pass: per-mode stamina drain
// (dive/glide/hover), exhaustion HP burn, hard-landing damage on Landed,
// and an on-screen flight debug readout (all tuning in GAMEPLAY.md s2).
// Invariants: movement mode is the single source of "flying" truth.
// [2026-10-28 T-041] In production code paths the pawn mutates stamina
// and HP only through UDragonStatsComponent methods; the pools
// themselves are public UPROPERTYs (see that header) — not encapsulated.

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

// [2026-10-28 T-043] Flight animation state chosen purely from
// movement facts by ADragonCharacter::SelectAnimState. Idle/Walk are
// the grounded states from T-032; Fly is powered flight and Glide is
// wings-held flight (the fly clip at a low play rate in Tick).
enum class EDragonAnimState : uint8
{
	Idle,
	Walk,
	Fly,
	Glide
};

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

	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UCameraComponent> Camera;

	// [2026-10-27 T-003] Flight tuning for the skeleton; T-011 owns feel.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float FlapUpwardSpeed = 650.0f;

	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float DiveAcceleration = 2400.0f;

	bool IsFlying() const;

	// [2026-10-28 T-014] Dwarf trap effect (GAMEPLAY.md section 5.2):
	// multiplies walk/fly speed by Factor for Seconds, then restores.
	// Implemented as a Tick countdown (not a timer handle) so pausing
	// the world pauses the trap; re-triggering refreshes, never stacks.
	void ApplyTrapSlow(float Factor, float Seconds);
	bool IsTrapSlowed() const
	{
		return TrapSlowTimeRemaining > 0.0f;
	}

	// [2026-10-28 T-033] Input contract: true only when every
	// code-created action and the mapping context exist.
	// EnsureInputActions() guarantees this before SetupPlayerInput
	// Component binds anything (bindings store the action pointer,
	// so a null bound early can never fire).
	bool HasInputActions() const;
	// [2026-10-28 T-033] Build the actions if they don't exist yet.
	// Lazy because UObjects cannot be created inside the class
	// constructor (CDO creation asserts); called at the top of
	// SetupPlayerInputComponent and BeginPlay.
	void EnsureInputActions();

	// [2026-10-28 T-032] Walk cycle (T-029) played single-node while
	// the pawn actually moves; nullptr if the asset failed to load,
	// in which case the mesh holds its standing bind pose.
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> WalkAnim;

	// [2026-10-28 T-043] Flight cycle played single-node while flying;
	// loaded in the constructor from SK_Dragon_v4_FlyAnim. A separate
	// import step creates that asset; if it is missing FlyAnim stays
	// null, BeginPlay logs one warning, and flight falls back to the
	// bind pose in Tick (errors surfaced, never silent).
	UPROPERTY(VisibleAnywhere, Category = "Dragon")
	TObjectPtr<UAnimSequence> FlyAnim;

	// [2026-10-28 T-043] Pure animation-state selection, testable
	// without a world: flying and diving -> Fly (powered, fast flap
	// in Tick); flying, not diving, slow (HorizontalSpeed < 50) and
	// not climbing (VerticalSpeed <= 0) -> Glide (wings held; Tick
	// plays FlyAnim at a low rate so it reads as a glide); flying
	// otherwise -> Fly; not flying with HorizontalSpeed > 20 ->
	// Walk; otherwise Idle. Dive only matters airborne.
	static EDragonAnimState SelectAnimState(bool bFlying, bool bDiving, float HorizontalSpeed,
											float VerticalSpeed);

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
	// [2026-10-28 T-035] Every UObject member below is UPROPERTY-tracked:
	// without reflection the GC cannot see these TObjectPtrs from the
	// pawn, so the actions/widget survived only via external Enhanced
	// Input/widget-tree references (T-004 sweep finding, high).
	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY()
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY()
	TObjectPtr<UInputAction> FlapAction;
	UPROPERTY()
	TObjectPtr<UInputAction> ToggleFlightAction;
	UPROPERTY()
	TObjectPtr<UInputAction> DiveAction;
	UPROPERTY()
	TObjectPtr<UInputAction> TrueSightAction;
	UPROPERTY()
	TObjectPtr<UInputAction> TintAction;
	UPROPERTY()
	TObjectPtr<UDragonTintWidget> TintWidget;
	bool bTintPickerOpen = false;
	UPROPERTY()
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

	// [2026-10-28 T-043] Last state Tick applied to the single-node
	// mesh, so Tick only calls Play/Stop on SelectAnimState
	// transitions (replaces the T-032 bPlayingWalk bool).
	EDragonAnimState CurrentAnimState = EDragonAnimState::Idle;

	// [2026-10-28 T-011] Last Tick's vertical velocity (cm/s, signed).
	// Captured before Tick's flight logic runs so Landed() can recover
	// the impact speed the movement component has already discarded.
	float LastVelocityZ = 0.0f;

	// [2026-10-28 T-014] Trap-slow countdown + the un-slowed speeds
	// captured on first application (restored exactly at expiry).
	float TrapSlowTimeRemaining = 0.0f;
	float UnslowedWalkSpeed = 0.0f;
	float UnslowedFlySpeed = 0.0f;
};
