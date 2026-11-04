// Copyright Under the Mountain contributors.
// Purpose: dragon animation decisions in testable C++ (T-068). Created 2026-11-01.
// [2026-11-01 T-068] See DragonAnimInstance.h for the architecture. The static
// [2026-11-01 T-068] functions below are the whole decision surface; the
// [2026-11-01 T-068] instance half only samples the owning pawn and republishes
// [2026-11-01 T-068] their results, so a test that proves a static proves the
// [2026-11-01 T-068] in-game behaviour that consumes it.

#include "Dragon/DragonAnimInstance.h"

#include "Dragon/DragonCharacter.h"
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
		if (HorizontalSpeed > 600.0f && VerticalSpeed <= -50.0f)
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
	LocomotionState = SelectLocomotionState(bAirborne, bDiving, GroundSpeed, VerticalSpeed);
	FlapPlayRate = ComputeFlapPlayRate(LocomotionState, GroundSpeed, VerticalSpeed);
	FoldAlpha = ComputeFoldAlpha(LocomotionState);
	TuckAlpha = ComputeTuckAlpha(LocomotionState);
	MeshPitchDegrees = ComputeMeshPitchDegrees(GroundSpeed, VerticalSpeed);
	const float Yaw = Pawn->GetActorRotation().Yaw;
	if (!bHasLastYaw || DeltaSeconds <= 0.0f)
	{
		LastUpdateYaw = Yaw;
		bHasLastYaw = true;
		BankDegrees = 0.0f;
	}
	else
	{
		const float YawRate = FMath::FindDeltaAngleDegrees(LastUpdateYaw, Yaw) / DeltaSeconds;
		LastUpdateYaw = Yaw;
		BankDegrees = ComputeBankDegrees(YawRate, GroundSpeed);
	}
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
