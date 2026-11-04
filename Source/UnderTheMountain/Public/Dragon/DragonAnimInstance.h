// Copyright Under the Mountain contributors.
// Purpose: dragon animation decisions in testable C++ (T-068). Created 2026-11-01.
// [2026-11-01 T-068] UDragonAnimInstance is the C++ parent the lean ABP_Dragon
// [2026-11-01 T-068] graph will sit on (docs/ANIMATION-SPIKE.md): it owns every
// [2026-11-01 T-068] locomotion/blend decision as pure static functions so the
// [2026-11-01 T-068] rules are unit-tested without a world, and publishes the
// [2026-11-01 T-068] resulting variables (state, flap rate, fold/tuck alphas,
// [2026-11-01 T-068] bank, mesh pitch) for both the current single-node driver
// [2026-11-01 T-068] in ADragonCharacter and the future AnimBP graph. Invariant:
// [2026-11-01 T-068] this class never invents gameplay rules — it only turns
// [2026-11-01 T-068] movement facts (velocity, flying, diving) into pose facts.

#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "DragonAnimInstance.generated.h"

// [2026-11-01 T-068] Locomotion pose states. Idle/Walk/Fly/Glide keep the
// [2026-11-01 T-068] T-043 values/order (serialized logs + the old test named
// [2026-11-01 T-068] them); Hover and Dive are appended. Dive is its own state
// [2026-11-01 T-068] now, not "Fly but faster": per the spike, a dive is wings
// [2026-11-01 T-068] swept with little flap, the opposite of the old 1.6x flap.
// [2026-11-04 T-125] BlueprintType: ABP_Dragon's state machine transitions
// [2026-11-04 T-125] test the published LocomotionState below (the C++ static
// [2026-11-04 T-125] stays the single owner of the rule; the graph only reads).
UENUM(BlueprintType)
enum class EDragonAnimState : uint8
{
	Idle,
	Walk,
	Fly,
	Glide,
	Hover,
	Dive
};

UCLASS()
class UNDERTHEMOUNTAIN_API UDragonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

  public:
	// [2026-11-01 T-068] Pure state selection from movement facts. Airborne:
	// [2026-11-01 T-068] diving -> Dive; slow in both axes (precision band) ->
	// [2026-11-01 T-068] Hover; fast and descending -> Glide (wings held, its
	// [2026-11-01 T-068] own clip now, not slowed Fly); otherwise Fly. Grounded:
	// [2026-11-01 T-068] HorizontalSpeed > 20 -> Walk, else Idle. Dive input is
	// [2026-11-01 T-068] ignored on the ground, exactly as T-043 specified.
	static EDragonAnimState SelectLocomotionState(bool bFlying, bool bDiving, float HorizontalSpeed,
												  float VerticalSpeed);

	// [2026-11-01 T-068] Wingbeat play rate per state (spike wingbeat curves):
	// [2026-11-01 T-068] climb works harder, cruise moderates, hover beats deep
	// [2026-11-01 T-068] and fast, glide/dive barely move. Clamped 0.05..1.8 so
	// [2026-11-01 T-068] no tuning value can freeze or strobe the mesh.
	static float ComputeFlapPlayRate(EDragonAnimState State, float HorizontalSpeed, float VerticalSpeed);

	// [2026-11-01 T-068] Additive alphas for the future AnimBP layers (and the
	// [2026-11-01 T-068] contract the baked clips already satisfy): Fold 1 =
	// [2026-11-01 T-068] wings folded on the ground, 0 = fully spread; Tuck 1 =
	// [2026-11-01 T-068] legs streamlined in flight, 0.5 hover (legs ready).
	static float ComputeFoldAlpha(EDragonAnimState State);
	static float ComputeTuckAlpha(EDragonAnimState State);

	// [2026-11-01 T-068] Visual mesh tilt (display only; the capsule never
	// [2026-11-01 T-068] tilts): bank from yaw rate scaled by airspeed, pitch
	// [2026-11-01 T-068] from the flight-path angle. Both hard-clamped so the
	// [2026-11-01 T-068] body can never roll past readable at gameplay distance.
	static float ComputeBankDegrees(float YawRateDegPerSec, float HorizontalSpeed);
	static float ComputeMeshPitchDegrees(float HorizontalSpeed, float VerticalSpeed);

	// [2026-11-01 T-068] Published pose facts, refreshed in
	// [2026-11-01 T-068] NativeUpdateAnimation when an ABP uses this class and
	// [2026-11-01 T-068] read by ADragonCharacter's driver today via the statics.
	// [2026-11-04 T-125] LocomotionState + bDiving join the published set:
	// [2026-11-04 T-125] the ABP state machine transitions test LocomotionState
	// [2026-11-04 T-125] (computed by the tested SelectLocomotionState static,
	// [2026-11-04 T-125] never by graph logic).
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	EDragonAnimState LocomotionState = EDragonAnimState::Idle;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float GroundSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float VerticalSpeed = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	bool bFlying = false;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	bool bDiving = false;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float FlapPlayRate = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float FoldAlpha = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float TuckAlpha = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float BankDegrees = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float MeshPitchDegrees = 0.0f;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

  private:
	// [2026-11-04 T-125] Transition proof + bank feed, both instance-local:
	// LastLoggedStateId starts at -1 (nothing logged yet) so the first
	// tick always announces; LastUpdateYaw seeds on the first tick with a
	// valid pawn so the first bank rate is 0, not a 180-degree snap.
	int32 LastLoggedStateId = -1;
	float LastUpdateYaw = 0.0f;
	bool bHasLastYaw = false;
};
