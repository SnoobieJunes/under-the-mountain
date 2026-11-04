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

// [2026-11-04 T-126] Every v1 one-shot/action clip family (docs/ANIMATION-SPIKE.md
// [2026-11-04 T-126] adopted v1 + combat expansion). Locomotion families are the
// [2026-11-04 T-126] EDragonAnimState machine above; these families route through
// [2026-11-04 T-126] montage slots (or, for FirePose/SleepIdle, a looping layer).
// [2026-11-04 T-126] A family with no imported clip yet still has a routing entry:
// [2026-11-04 T-126] ADragonCharacter::ClipForFamily returns null and PlayClipFamily
// [2026-11-04 T-126] no-ops, so no family is silently dropped (T-126 Accept).
UENUM(BlueprintType)
enum class EDragonClipFamily : uint8
{
	HitHeavy,
	Defeat,
	SleepIdle,
	Wake,
	LieDown,
	FirePose,
	Claw,
	BiteEat,
	TailWhip,
	WingBuffet
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

	// [2026-11-04 feel/spam] Hysteresis/dwell overload (the raw overload above
	// [2026-11-04 feel/spam] stays the candidate rule): a Fly/Glide/Hover state
	// [2026-11-04 feel/spam] is held for MinLocomotionDwellSeconds and each band
	// [2026-11-04 feel/spam] has separate enter/exit edges (Glide enters at
	// [2026-11-04 feel/spam] VS<-250 in the raw rule, a held Glide exits only at
	// [2026-11-04 feel/spam] VS>-50; Hover enters inside 250/250, exits outside
	// [2026-11-04 feel/spam] 350/350), so borderline flight cannot churn states
	// [2026-11-04 feel/spam] frame-to-frame. Dive and ground/air switches are
	// [2026-11-04 feel/spam] input/fact changes, never dwell-blocked.
	static EDragonAnimState SelectLocomotionState(EDragonAnimState PreviousState, bool bFlying,
												  bool bDiving, float HorizontalSpeed,
												  float VerticalSpeed, float TimeInPreviousStateSeconds);
	static constexpr float MinLocomotionDwellSeconds = 0.3f;

	// [2026-11-01 T-068] Wingbeat play rate per state (spike wingbeat curves):
	// [2026-11-01 T-068] climb works harder, cruise moderates, hover beats deep
	// [2026-11-01 T-068] and fast, glide/dive barely move. Clamped 0.05..1.8 so
	// [2026-11-01 T-068] no tuning value can freeze or strobe the mesh.
	static float ComputeFlapPlayRate(EDragonAnimState State, float HorizontalSpeed, float VerticalSpeed);

	// [2026-11-04 feel/spam] Walk clip rate from ground speed alone: a
	// [2026-11-04 feel/spam] 300 cm/s stride plays at 1.0, clamped so a
	// [2026-11-04 feel/spam] shuffle never freezes and a charge never strobes.
	static float ComputeWalkPlayRate(float GroundSpeed);

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

	// [2026-11-04 T-126] Clip-family routing seam (pure, asset-free): slot name
	// [2026-11-04 T-126] per docs/ANIMATION-SPIKE.md montage slots — full-body:
	// [2026-11-04 T-126] heavy hit, defeat, sleep/wake transitions, tail whip,
	// [2026-11-04 T-126] wing buffet; upper-body (chest branch): fire pose, claw,
	// [2026-11-04 T-126] bite/eat. SleepIdle is a looping ground layer, not a
	// [2026-11-04 T-126] one-shot, so it shares FullBody with looping=true.
	static FName SlotForClipFamily(EDragonClipFamily Family);
	static bool IsLoopingClipFamily(EDragonClipFamily Family);
	// [2026-11-04 T-126] Notify seam: normalised clip time (0..1) at which the
	// [2026-11-04 T-126] gameplay event fires (claw damage window, bite/eat
	// [2026-11-04 T-126] consume point, tail-whip/buffet damage, roar point is
	// [2026-11-04 T-126] locomotion-adjacent and not a family here). Families
	// [2026-11-04 T-126] with no gameplay notify return -1. Rules stay in
	// [2026-11-04 T-126] combat components; this only synchronises presentation.
	static float NotifyFractionForClipFamily(EDragonClipFamily Family);

	// [2026-11-04 T-126] Slither / tail-lag procedural layer (spike: runtime
	// [2026-11-04 T-126] procedural layers, not baked clips, so every locomotion
	// [2026-11-04 T-126] state inherits them). Amplitude is 0 at rest, rises with
	// [2026-11-04 T-126] ground/air speed, and is capped so the spine never
	// [2026-11-04 T-126] reads as a snake at gameplay distance.
	static float ComputeSlitherAmplitudeDeg(EDragonAnimState State, float HorizontalSpeed);
	// [2026-11-04 T-126] Per-segment slither offset: a travelling sine whose
	// [2026-11-04 T-126] phase lags by SegmentIndex (head leads, tail follows).
	static float ComputeSlitherOffsetDeg(int32 SegmentIndex, float PhaseRadians, float AmplitudeDeg);
	// [2026-11-04 T-126] Tail lag under yaw: each successive tail segment lags
	// [2026-11-04 T-126] further behind a turn, total capped at 30 degrees.
	static float ComputeTailLagDegrees(int32 SegmentIndex, float YawRateDegPerSec, float HorizontalSpeed);
	// [2026-11-04 T-126] Fire head-aim: clamp a desired head pitch (deg) to the
	// [2026-11-04 T-126] readable aim band the fire pose additive may use.
	static float ComputeFireAimPitchDegrees(float DesiredPitchDeg);
	// [2026-11-04 feel/spam] One additive bone rotation (component-space
	// [2026-11-04 feel/spam] yaw only) for a spine/tail segment: slither
	// [2026-11-04 feel/spam] offset + tail lag combined, so an ABP
	// [2026-11-04 feel/spam] ModifyBone node needs no graph math — it binds
	// [2026-11-04 feel/spam] one published FRotator below per bone.
	static FRotator ComputeSlitherBoneRotation(int32 SegmentIndex, float PhaseRadians,
											   float AmplitudeDeg, float YawRateDegPerSec,
											   float HorizontalSpeed);

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
	float WalkPlayRate = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float FoldAlpha = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float TuckAlpha = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float BankDegrees = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float MeshPitchDegrees = 0.0f;
	// [2026-11-04 T-126] Procedural-layer outputs for the ABP graph (T-139
	// [2026-11-04 T-126] wires them to spine/tail additives; the single-node
	// [2026-11-04 T-126] driver does not consume them, so publishing them
	// [2026-11-04 T-126] cannot regress the shipped course).
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float SlitherAmplitudeDeg = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float SlitherPhaseRadians = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float TailLagDegrees = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	float FireAimPitchDegrees = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	bool bFiring = false;
	// [2026-11-04 feel/spam] Additive ModifyBone rotations (component-space
	// [2026-11-04 feel/spam] yaw), one per wired bone: pelvis (segment 0),
	// [2026-11-04 feel/spam] tail.01 (1), tail.03 (3), tail.05 (5). The ABP
	// [2026-11-04 feel/spam] commandlet binds each to that bone's node.
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	FRotator SpineSlitherRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	FRotator TailBaseSlitherRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	FRotator TailMidSlitherRotation = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadOnly, Category = "Dragon|Anim")
	FRotator TailTipSlitherRotation = FRotator::ZeroRotator;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

  private:
	// [2026-11-04 T-125] Transition proof + bank feed, both instance-local:
	// LastLoggedStateId starts at -1 (nothing logged yet) so the first
	// tick always announces; LastUpdateYaw seeds on the first tick with a
	// valid pawn so the first bank rate is 0, not a 180-degree snap.
	int32 LastLoggedStateId = -1;
	float LastUpdateYaw = 0.0f;
	bool bHasLastYaw = false;
	// [2026-11-04 feel/spam] Seconds spent in the current LocomotionState;
	// [2026-11-04 feel/spam] feeds the hysteresis overload's dwell gate.
	float TimeInLocomotionStateSeconds = 0.0f;
};
