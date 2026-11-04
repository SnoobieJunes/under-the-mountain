// Copyright Under the Mountain contributors.
// Purpose: dragon animation decisions in testable C++ (T-068). Created 2026-11-01.
// [2026-11-01 T-068] See DragonAnimInstance.h for the architecture. The static
// [2026-11-01 T-068] functions below are the whole decision surface; the
// [2026-11-01 T-068] instance half only samples the owning pawn and republishes
// [2026-11-01 T-068] their results, so a test that proves a static proves the
// [2026-11-01 T-068] in-game behaviour that consumes it.

#include "Dragon/DragonAnimInstance.h"

#include "Dragon/DragonCharacter.h"
#include "Dragon/DragonCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "UnderTheMountain.h"

EDragonAnimState UDragonAnimInstance::SelectLocomotionState(bool bFlying, bool bDiving, float HorizontalSpeed,
															float VerticalSpeed)
{
	if (bFlying)
	{
		if (bDiving)
		{
			return EDragonAnimState::Dive;
		}
		// [2026-11-01 T-068] Precision band: slow horizontally AND vertically.
		// [2026-11-01 T-068] Thresholds sit well under cruise (fly speed is in
		// [2026-11-01 T-068] the thousands of cm/s) so ordinary flight never
		// [2026-11-01 T-068] flickers into Hover mid-glide.
		if (HorizontalSpeed < 250.0f && FMath::Abs(VerticalSpeed) < 250.0f)
		{
			return EDragonAnimState::Hover;
		}
		// [2026-11-04 feel/spam] Glide ENTRY is VS<=-250 (was -50): the
		// [2026-11-04 feel/spam] -250..-50 band is the hysteresis zone the
		// [2026-11-04 feel/spam] dwell overload below holds a Glide through.
		if (HorizontalSpeed > 600.0f && VerticalSpeed <= -250.0f)
		{
			return EDragonAnimState::Glide;
		}
		return EDragonAnimState::Fly;
	}
	if (HorizontalSpeed > 20.0f)
	{
		return EDragonAnimState::Walk;
	}
	return EDragonAnimState::Idle;
}

EDragonAnimState UDragonAnimInstance::SelectLocomotionState(EDragonAnimState PreviousState, bool bFlying,
															bool bDiving, float HorizontalSpeed,
															float VerticalSpeed,
															float TimeInPreviousStateSeconds)
{
	const EDragonAnimState Candidate =
		SelectLocomotionState(bFlying, bDiving, HorizontalSpeed, VerticalSpeed);
	if (Candidate == PreviousState || Candidate == EDragonAnimState::Dive ||
		PreviousState == EDragonAnimState::Dive)
	{
		return Candidate;
	}
	const bool bPreviousAirborne = PreviousState == EDragonAnimState::Fly ||
								   PreviousState == EDragonAnimState::Glide ||
								   PreviousState == EDragonAnimState::Hover;
	const bool bCandidateAirborne = Candidate == EDragonAnimState::Fly ||
									Candidate == EDragonAnimState::Glide ||
									Candidate == EDragonAnimState::Hover;
	// [2026-11-04 feel/spam] Ground<->air and ground-state changes are fact
	// [2026-11-04 feel/spam] changes, not band churn: never dwell-blocked.
	if (!bFlying || !bPreviousAirborne || !bCandidateAirborne)
	{
		return Candidate;
	}
	// [2026-11-04 feel/spam] Exit bands are looser than entry bands: a held
	// [2026-11-04 feel/spam] Glide survives until VS rises past -50, a held
	// [2026-11-04 feel/spam] Hover until either axis leaves 350.
	if (PreviousState == EDragonAnimState::Glide && HorizontalSpeed > 600.0f && VerticalSpeed <= -50.0f)
	{
		return EDragonAnimState::Glide;
	}
	if (PreviousState == EDragonAnimState::Hover && HorizontalSpeed < 350.0f &&
		FMath::Abs(VerticalSpeed) < 350.0f)
	{
		return EDragonAnimState::Hover;
	}
	if (TimeInPreviousStateSeconds < MinLocomotionDwellSeconds)
	{
		return PreviousState;
	}
	return Candidate;
}

float UDragonAnimInstance::ComputeWalkPlayRate(float GroundSpeed)
{
	return FMath::Clamp(GroundSpeed / 300.0f, 0.5f, 1.5f);
}

float UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState State, float HorizontalSpeed, float VerticalSpeed)
{
	float Rate = 1.0f;
	switch (State)
	{
	case EDragonAnimState::Fly:
		// [2026-11-01 T-068] Climbing beats faster, sinking eases off toward
		// [2026-11-01 T-068] the glide; ±600 cm/s vertical spans the range.
		Rate = 1.0f + FMath::Clamp(VerticalSpeed / 600.0f, -1.0f, 1.0f) * 0.35f;
		break;
	case EDragonAnimState::Hover:
		Rate = 1.15f;
		break;
	case EDragonAnimState::Glide:
		Rate = 1.0f; // the Glide clip itself is near-still; rate stays honest
		break;
	case EDragonAnimState::Dive:
		Rate = 0.15f; // swept wings, effectively held (Glide clip, near-frozen)
		break;
	default:
		Rate = 1.0f;
		break;
	}
	return FMath::Clamp(Rate, 0.05f, 1.8f);
}

float UDragonAnimInstance::ComputeFoldAlpha(EDragonAnimState State)
{
	switch (State)
	{
	case EDragonAnimState::Idle:
	case EDragonAnimState::Walk:
		return 1.0f;
	case EDragonAnimState::Dive:
		return 0.35f; // partly swept, not folded flat
	default:
		return 0.0f;
	}
}

float UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState State)
{
	switch (State)
	{
	case EDragonAnimState::Fly:
	case EDragonAnimState::Glide:
	case EDragonAnimState::Dive:
		return 1.0f;
	case EDragonAnimState::Hover:
		return 0.5f;
	default:
		return 0.0f;
	}
}

float UDragonAnimInstance::ComputeBankDegrees(float YawRateDegPerSec, float HorizontalSpeed)
{
	// [2026-11-01 T-068] No bank without airspeed (a hovering dragon turning
	// [2026-11-01 T-068] on the spot must not corkscrew); full response by
	// [2026-11-01 T-068] 1200 cm/s. 90 deg/s of turn maps to the 35 deg cap.
	const float SpeedScale = FMath::Clamp(HorizontalSpeed / 1200.0f, 0.0f, 1.0f);
	return FMath::Clamp(-YawRateDegPerSec * (35.0f / 90.0f) * SpeedScale, -35.0f, 35.0f);
}

float UDragonAnimInstance::ComputeMeshPitchDegrees(float HorizontalSpeed, float VerticalSpeed)
{
	if (HorizontalSpeed < 50.0f && FMath::Abs(VerticalSpeed) < 50.0f)
	{
		return 0.0f;
	}
	// [2026-11-01 T-068] Follow 60% of the flight-path angle: full alignment
	// [2026-11-01 T-068] reads as a missile, none reads as a sliding statue
	// [2026-11-01 T-068] (the T-058 complaint). Negative = nose down.
	const float PathDegrees = FMath::RadiansToDegrees(FMath::Atan2(VerticalSpeed, FMath::Max(HorizontalSpeed, 1.0f)));
	return FMath::Clamp(PathDegrees * 0.6f, -30.0f, 30.0f);
}

FName UDragonAnimInstance::SlotForClipFamily(EDragonClipFamily Family)
{
	// [2026-11-04 T-126] Names must match ADragonCharacter::AnimSlot* and the
	// [2026-11-04 T-126] ABP build sheet exactly; tests pin both sides.
	switch (Family)
	{
	case EDragonClipFamily::FirePose:
	case EDragonClipFamily::Claw:
	case EDragonClipFamily::BiteEat:
		return FName(TEXT("UpperBody"));
	default:
		return FName(TEXT("FullBody"));
	}
}

bool UDragonAnimInstance::IsLoopingClipFamily(EDragonClipFamily Family)
{
	// [2026-11-04 T-126] Only the looping action/ground layers loop; every
	// [2026-11-04 T-126] other family is a one-shot with a notify point.
	return Family == EDragonClipFamily::FirePose || Family == EDragonClipFamily::SleepIdle;
}

float UDragonAnimInstance::NotifyFractionForClipFamily(EDragonClipFamily Family)
{
	// [2026-11-04 T-126] Fractions match the key poses in
	// [2026-11-04 T-126] Scripts/create_dragon_anims.py (damage/consume at
	// [2026-11-04 T-126] the strike/bite apex, not at clip start).
	switch (Family)
	{
	case EDragonClipFamily::Claw:
		return 0.45f;
	case EDragonClipFamily::BiteEat:
		return 0.60f;
	case EDragonClipFamily::TailWhip:
		return 0.50f;
	case EDragonClipFamily::WingBuffet:
		return 0.40f;
	case EDragonClipFamily::HitHeavy:
		return 0.25f;
	default:
		return -1.0f;
	}
}

float UDragonAnimInstance::ComputeSlitherAmplitudeDeg(EDragonAnimState State, float HorizontalSpeed)
{
	// [2026-11-04 T-126] Ground walk slithers most; flight keeps a smaller
	// [2026-11-04 T-126] travelling wave; idle/hover/dive hold still.
	float Cap = 0.0f;
	switch (State)
	{
	case EDragonAnimState::Walk:
		Cap = 6.0f;
		break;
	case EDragonAnimState::Fly:
	case EDragonAnimState::Glide:
		Cap = 3.0f;
		break;
	default:
		return 0.0f;
	}
	return Cap * FMath::Clamp(HorizontalSpeed / 1200.0f, 0.0f, 1.0f);
}

float UDragonAnimInstance::ComputeSlitherOffsetDeg(int32 SegmentIndex, float PhaseRadians, float AmplitudeDeg)
{
	// [2026-11-04 T-126] 0.6 rad of phase lag per segment: the wave travels
	// [2026-11-04 T-126] down the spine instead of swaying in unison.
	const float Lag = 0.6f * static_cast<float>(FMath::Max(SegmentIndex, 0));
	return AmplitudeDeg * FMath::Sin(PhaseRadians - Lag);
}

float UDragonAnimInstance::ComputeTailLagDegrees(int32 SegmentIndex, float YawRateDegPerSec,
												 float HorizontalSpeed)
{
	// [2026-11-04 T-126] No lag without airspeed (mirrors ComputeBankDegrees);
	// [2026-11-04 T-126] each segment adds lag, capped at 30 deg total.
	if (SegmentIndex <= 0)
	{
		return 0.0f;
	}
	const float SpeedScale = FMath::Clamp(HorizontalSpeed / 1200.0f, 0.0f, 1.0f);
	const float PerSegment = -YawRateDegPerSec * (10.0f / 90.0f) * SpeedScale;
	return FMath::Clamp(PerSegment * static_cast<float>(SegmentIndex), -30.0f, 30.0f);
}

FRotator UDragonAnimInstance::ComputeSlitherBoneRotation(int32 SegmentIndex, float PhaseRadians,
														 float AmplitudeDeg, float YawRateDegPerSec,
														 float HorizontalSpeed)
{
	// [2026-11-04 feel/spam] Component-space yaw only: lateral wave + turn
	// [2026-11-04 feel/spam] lag read the same in any bone's local frame.
	const float YawDeg = ComputeSlitherOffsetDeg(SegmentIndex, PhaseRadians, AmplitudeDeg) +
						 ComputeTailLagDegrees(SegmentIndex, YawRateDegPerSec, HorizontalSpeed);
	return FRotator(0.0f, YawDeg, 0.0f);
}

float UDragonAnimInstance::ComputeFireAimPitchDegrees(float DesiredPitchDeg)
{
	// [2026-11-04 T-126] Head aim band: enough to sell the breath direction
	// [2026-11-04 T-126] without folding the neck through the chest.
	return FMath::Clamp(DesiredPitchDeg, -35.0f, 25.0f);
}

void UDragonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	// [2026-11-01 T-068] Sample-only: every published value comes from the
	// [2026-11-01 T-068] statics above so ABP behaviour and tests cannot drift.
	// [2026-11-04 T-125] Hybrid-aware sampling (the T-068 version only knew
	// [2026-11-04 T-125] MOVE_Flying, hard-coded dive off, and never wrote
	// [2026-11-04 T-125] BankDegrees): the airborne rule mirrors the pawn
	// [2026-11-04 T-125] driver's exactly (legacy + hybrid + falling), dive
	// [2026-11-04 T-125] reads the pawn's input latch, and bank reads the yaw
	// [2026-11-04 T-125] rate — so the ABP transition tests see the same facts
	// [2026-11-04 T-125] the single-node driver acted on.
	APawn* Pawn = TryGetPawnOwner();
	if (Pawn == nullptr)
	{
		return;
	}
	const FVector Velocity = Pawn->GetVelocity();
	GroundSpeed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;
	bool bAirborne = false;
	bool bDiveInput = false;
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(Pawn))
	{
		bAirborne = Dragon->IsFlying() || Dragon->IsPrototypeFlying() ||
					Dragon->GetCharacterMovement()->MovementMode == MOVE_Falling;
		bDiveInput = Dragon->IsDiving();
	}
	else if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		bAirborne = Character->GetCharacterMovement()->MovementMode == MOVE_Flying;
	}
	bFlying = bAirborne;
	bDiving = bAirborne && bDiveInput;
	// [2026-11-04 feel/spam] Hysteresis overload owns the published state:
	// [2026-11-04 feel/spam] the dwell timer resets on every real change.
	TimeInLocomotionStateSeconds += FMath::Max(0.0f, DeltaSeconds);
	const EDragonAnimState NextLocomotionState = SelectLocomotionState(
		LocomotionState, bAirborne, bDiving, GroundSpeed, VerticalSpeed, TimeInLocomotionStateSeconds);
	if (NextLocomotionState != LocomotionState)
	{
		TimeInLocomotionStateSeconds = 0.0f;
	}
	LocomotionState = NextLocomotionState;
	FlapPlayRate = ComputeFlapPlayRate(LocomotionState, GroundSpeed, VerticalSpeed);
	WalkPlayRate = ComputeWalkPlayRate(GroundSpeed);
	FoldAlpha = ComputeFoldAlpha(LocomotionState);
	TuckAlpha = ComputeTuckAlpha(LocomotionState);
	MeshPitchDegrees = ComputeMeshPitchDegrees(GroundSpeed, VerticalSpeed);
	const float Yaw = Pawn->GetActorRotation().Yaw;
	float YawRateForLag = 0.0f;
	if (!bHasLastYaw || DeltaSeconds <= 0.0f)
	{
		LastUpdateYaw = Yaw;
		bHasLastYaw = true;
		BankDegrees = 0.0f;
	}
	else
	{
		const float YawRate = FMath::FindDeltaAngleDegrees(LastUpdateYaw, Yaw) / DeltaSeconds;
		YawRateForLag = YawRate;
		LastUpdateYaw = Yaw;
		// [2026-11-04 FLIGHT-VIGOR] Commanded bank wins while
		// prototype-flying: the pawn banks from turn input first and
		// derives yaw from it, so the graph reads the cause, not the
		// yaw-derivative effect. Other paths keep the derivative.
		const ADragonCharacter* Dragon = Cast<ADragonCharacter>(Pawn);
		BankDegrees = (Dragon != nullptr && Dragon->IsPrototypeFlying())
						  ? Dragon->GetProtoBankDegrees()
						  : ComputeBankDegrees(YawRate, GroundSpeed);
	}
	// [2026-11-04 T-126] Procedural layer: phase advances with speed (a
	// [2026-11-04 T-126] stopped dragon does not keep writhing), tail lag
	// [2026-11-04 T-126] reads the same yaw rate as bank, fire aim follows
	// [2026-11-04 T-126] mesh pitch only while the breath is actually lit.
	SlitherAmplitudeDeg = ComputeSlitherAmplitudeDeg(LocomotionState, GroundSpeed);
	if (DeltaSeconds > 0.0f)
	{
		SlitherPhaseRadians += DeltaSeconds * (2.0f + GroundSpeed / 300.0f);
	}
	// [2026-11-04 T-126] Segment 3 = mid-tail read for the graph's single
	// [2026-11-04 T-126] published value; per-segment values recompute in-graph.
	TailLagDegrees = ComputeTailLagDegrees(3, YawRateForLag, GroundSpeed);
	// [2026-11-04 feel/spam] Per-bone additive rotations for the ABP
	// [2026-11-04 feel/spam] ModifyBone chain (pelvis/tail.01/tail.03/tail.05).
	SpineSlitherRotation =
		ComputeSlitherBoneRotation(0, SlitherPhaseRadians, SlitherAmplitudeDeg, YawRateForLag, GroundSpeed);
	TailBaseSlitherRotation =
		ComputeSlitherBoneRotation(1, SlitherPhaseRadians, SlitherAmplitudeDeg, YawRateForLag, GroundSpeed);
	TailMidSlitherRotation =
		ComputeSlitherBoneRotation(3, SlitherPhaseRadians, SlitherAmplitudeDeg, YawRateForLag, GroundSpeed);
	TailTipSlitherRotation =
		ComputeSlitherBoneRotation(5, SlitherPhaseRadians, SlitherAmplitudeDeg, YawRateForLag, GroundSpeed);
	bFiring = false;
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(Pawn))
	{
		bFiring = Dragon->Combat != nullptr && Dragon->Combat->IsFiring();
	}
	FireAimPitchDegrees = bFiring ? ComputeFireAimPitchDegrees(MeshPitchDegrees) : 0.0f;
	// [2026-11-04 T-125] Transition proof on the T-113 ANIMSTATE channel
	// [2026-11-04 T-125] (headless -game runs keep no log file): on change
	// [2026-11-04 T-125] only, like the driver this replaces. clip=ABP — the
	// [2026-11-04 T-125] graph owns playback; per-state captures name clips.
	const int32 StateId = static_cast<int32>(LocomotionState);
	if (StateId != LastLoggedStateId)
	{
		LastLoggedStateId = StateId;
		UE_LOG(LogUnderTheMountain, Log, TEXT("Dragon anim state -> %d (flying=%d diving=%d)"),
			   StateId, bFlying ? 1 : 0, bDiving ? 1 : 0);
		printf("ANIMSTATE state=%d flying=%d diving=%d clip=ABP\n", StateId, bFlying ? 1 : 0,
			   bDiving ? 1 : 0);
		fflush(stdout);
	}
}
