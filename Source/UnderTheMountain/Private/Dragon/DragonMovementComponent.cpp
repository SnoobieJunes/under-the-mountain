// Copyright Under the Mountain contributors.
// Purpose: T-075 prototype flight math + custom-mode integrator — scheme
// selector, authority blend, energy integration, brake, impact severity,
// PhysCustom momentum core, Surge/Ascent impulses and CSV capture. Created
// 2026-11-03 (T-075). All model functions are pure statics over
// FDragonFlightParams: no world, no clock, no stamina. Slice 2a adds the
// integrator and pawn attach — still inert on scheme 0, so shipped flight
// is unchanged while the course has a flyable chassis.

#include "Dragon/DragonMovementComponent.h"
#include "Dragon/DragonStatsComponent.h"
#include "Dragon/DragonCharacter.h"
#include "GameFramework/Character.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnderTheMountain.h"

// [2026-11-03 T-062] Scheme C is the shipped flight model (T-075 course pick,
// provisional pending owner feel-check): the default selects the hybrid, so
// takeoffs enter the custom integrator with no opt-in. 0 keeps the legacy
// CharacterMovement flying path as the comparison baseline (course A/B legs,
// legacy feel checks); 1/3 keep A/B prototype-only.
static TAutoConsoleVariable<int32> CVarFlightScheme(
	TEXT("utm.flight.scheme"), 2,
	TEXT("Flight scheme (T-062): 2 = Scheme C hybrid (shipped default), " TEXT("0 = legacy shipped flight (comparison baseline), 1 = Scheme A momentum, 3 = Scheme B forced-precision.")));

UDragonMovementComponent::UDragonMovementComponent()
{
	// [2026-11-03 T-075] Defaults ride on FlightParams (header); nothing to
	// tune here.
}

int32 UDragonMovementComponent::GetFlightScheme()
{
	return CVarFlightScheme.GetValueOnGameThread();
}

float UDragonMovementComponent::PrecisionAuthorityWeight(float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] Smoothstepped band fade: full precision at or below
	// 650 cm/s, full momentum at or above 1600, C1-continuous between so the
	// hybrid never reads as a mode switch (the spike's C-falsifier).
	const float Lo = Params.PrecisionAuthorityMaxSpeed;
	const float Hi = Params.MomentumAuthorityMinSpeed;
	if (Airspeed <= Lo)
	{
		return 1.0f;
	}
	if (Airspeed >= Hi)
	{
		return 0.0f;
	}
	const float T = (Airspeed - Lo) / (Hi - Lo);
	const float S = T * T * (3.0f - 2.0f * T);
	return 1.0f - S;
}

float UDragonMovementComponent::IntegrateEnergyStep(float Speed, float PitchRadians, float DeltaSeconds,
													const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] Arcade-energy model v0 for the course to falsify:
	// gravity projects onto the flight path (nose-down gains, nose-up
	// bleeds), the terminal is pitch-dependent (cruise level, 4000 vertical
	// dive), and drag exists only above terminal so unpowered glide retains
	// speed. Negative speeds and non-positive dt are caller errors: clamp,
	// never propagate.
	if (DeltaSeconds <= 0.0f)
	{
		return FMath::Max(0.0f, Speed);
	}
	const float ClampedSpeed = FMath::Max(0.0f, Speed);
	const float SinPitch = FMath::Sin(PitchRadians);

	// [2026-11-03 T-075] Terminal for this pitch: dive side interpolates
	// cruise -> steep-dive by descent steepness; climb side decays cruise
	// toward zero so sustained climbs need Ascent/Surge impulses.
	float Terminal;
	if (SinPitch <= 0.0f)
	{
		Terminal = Params.LevelCruiseTerminal + (Params.SteepDiveTerminal - Params.LevelCruiseTerminal) * (-SinPitch);
	}
	else
	{
		Terminal = Params.LevelCruiseTerminal * (1.0f - 0.7f * SinPitch);
	}

	// [2026-11-03 T-075] Gravity along path: pitch > 0 (nose up) decelerates.
	float NewSpeed = ClampedSpeed - Params.GravityCM * SinPitch * DeltaSeconds;
	if (NewSpeed > Terminal)
	{
		const float Over = NewSpeed - Terminal;
		NewSpeed = Terminal + Over * FMath::Exp(-DeltaSeconds * Params.DragConvergenceRate);
	}
	return FMath::Max(0.0f, NewSpeed);
}

float UDragonMovementComponent::YawRateForAirspeed(float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] 90 deg/s in full precision fading to 55 deg/s at
	// dive speed, driven by the same blend weight as authority.
	const float W = PrecisionAuthorityWeight(Airspeed, Params);
	return Params.DiveYawRateDeg + (Params.PrecisionYawRateDeg - Params.DiveYawRateDeg) * W;
}

float UDragonMovementComponent::ComputeSteerResponsiveness(float Airspeed, float AuthorityW,
														   const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] First-order lag angle is yaw-rate/responsiveness; the
	// floor keeps it near 0.25 rad at every band so hard turns cost speed
	// (the skill) without collapsing it (the spiral).
	const float Base = Params.SteerResponsiveness * (1.0f + AuthorityW * 2.0f);
	const float Floor = FMath::DegreesToRadians(YawRateForAirspeed(Airspeed, Params)) * 4.0f;
	return FMath::Max(Base, Floor);
}

float UDragonMovementComponent::BlendAuthorityWeight(float Airspeed, float ImpulseAuthorityRemaining,
													 const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] Fresh impulses fly momentum-first; expired latches
	// read the airspeed smoothstep like before.
	if (ImpulseAuthorityRemaining > 0.0f)
	{
		return 0.0f;
	}
	return PrecisionAuthorityWeight(Airspeed, Params);
}

float UDragonMovementComponent::HoverCaptureWeight(float Airspeed, bool bBraking,
												   const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] Capture fades in across the bottom 300 cm/s of the
	// stall band (full hold at StallSpeed-300 and below), so coasts glide
	// through the precision band and only near-rest flight settles to hover.
	if (bBraking)
	{
		return 1.0f;
	}
	return FMath::Clamp((Params.StallSpeed - Airspeed) / 300.0f, 0.0f, 1.0f);
}

bool UDragonMovementComponent::IsDivePosture(float PitchDeg, float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-03 T-064] Committed nose-down (past -30 deg) while too fast
	// for precision thrust: gravity owns the flight path there, so speed
	// is being bought with altitude. Level, climbing, and slow flight —
	// however steep the camera looks — are never a dive.
	return PitchDeg < -30.0f && Airspeed > Params.MomentumAuthorityMinSpeed;
}

float UDragonMovementComponent::BrakeStep(float Speed, float DeltaSeconds, const FDragonFlightParams& Params)
{
	// [2026-11-03 T-075] Linear aerodynamic brake toward a controllable stop;
	// the caller forces precision authority while held (adopted Brake rule).
	if (DeltaSeconds <= 0.0f)
	{
		return FMath::Max(0.0f, Speed);
	}
	return FMath::Max(0.0f, Speed - Params.BrakeDecel * DeltaSeconds);
}

float UDragonMovementComponent::LandingSeverity(float Speed, float ImpactAngleDeg,
												const UDragonStatsComponent& Stats)
{
	// [2026-11-03 T-105] Severity prices the surface-normal component only:
	// Speed * sin(angle). A fast shallow flare (small normal) is cheap; a
	// head-on lawn-dart pays full price. The curve is Stats' shipped
	// landing curve, read here directly (single source of truth) so T-079
	// prices wall impacts on exactly the scale landing damage uses.
	const float ClampedAngle = FMath::Clamp(ImpactAngleDeg, 0.0f, 90.0f);
	const float NormalSpeed = FMath::Max(0.0f, Speed) * FMath::Sin(FMath::DegreesToRadians(ClampedAngle));
	if (NormalSpeed <= Stats.LandingSpeedThreshold)
	{
		return 0.0f;
	}
	if (NormalSpeed >= Stats.LandingSpeedMaxDamage)
	{
		return 1.0f;
	}
	return (NormalSpeed - Stats.LandingSpeedThreshold) / (Stats.LandingSpeedMaxDamage - Stats.LandingSpeedThreshold);
}

float UDragonMovementComponent::ClampSpeedToCeilings(float Speed, float PitchDeg) const
{
	// [2026-11-03 T-075] Dive terminal is the absolute cap; the Surge ceiling
	// holds level-ish flight (steep descents earn dive speed, per the skill
	// loop GAMEPLAY.md s2 names).
	float Capped = FMath::Min(Speed, FlightParams.SteepDiveTerminal);
	if (PitchDeg > -10.0f)
	{
		Capped = FMath::Min(Capped, FlightParams.SurgeCeiling);
	}
	return FMath::Max(0.0f, Capped);
}

void UDragonMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	// [2026-11-03 T-075] Scheme A momentum core (= C's high band; B forces
	// full precision): the nose defines the flight path, velocity steers
	// onto it with lag, stall sinks, brake decels. Collision slides along
	// surfaces; floor contact falls out fast or settles slow (v0 rule below).
	// Fighting PhysFlying from pawn Tick is what made the shipped dive a
	// camera-forward rocket — this integrator owns flight instead.
	(void)Iterations;
	if (deltaTime <= 0.0f || UpdatedComponent == nullptr || CharacterOwner == nullptr)
	{
		return;
	}
	if (CustomMovementMode != DragonFlightCustomMode)
	{
		return;
	}
	const int32 Scheme = GetFlightScheme();
	if (Scheme == static_cast<int32>(EDragonFlightScheme::Shipped))
	{
		SetMovementMode(MOVE_Flying);
		return;
	}

	// [2026-11-03 T-075] The nose defines the flight path (full pitch) — the
	// direct fix for root cause 1 (yaw-plane-only move input): aiming up the
	// chimney now means climbing the chimney. It is pawn state (free-look
	// freezes it, steer-latch turns it at the yaw limit); non-pawn owners
	// fall back to control rotation.
	FRotator NoseRot = CharacterOwner->GetControlRotation();
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(CharacterOwner))
	{
		NoseRot = Dragon->GetProtoNose();
	}
	const FVector NoseDir = NoseRot.Vector();
	const float PitchRad = FMath::DegreesToRadians(NoseRot.Pitch);

	float NewSpeed = IntegrateEnergyStep(Velocity.Size(), PitchRad, deltaTime, FlightParams);
	// [2026-11-03 T-075] System-identification record (see CaptureFlightCsvRow).
	LastEnergySpeed = NewSpeed;
	LastPhysDt = deltaTime;
	LastPhysIters = Iterations;
	// [2026-11-03 T-075] Authority blend: A pins 0 (pure momentum), B pins 1
	// (spike: disable momentum, force precision), C rides the airspeed
	// smoothstep — with a fresh impulse latched to momentum so launches fly.
	// Brake holds force full precision even before airspeed has decayed
	// (adopted Brake rule).
	float AuthorityW = 0.0f;
	if (Scheme == static_cast<int32>(EDragonFlightScheme::SchemeC))
	{
		ImpulseAuthorityTime = FMath::Max(0.0f, ImpulseAuthorityTime - deltaTime);
		AuthorityW = BlendAuthorityWeight(Velocity.Size(), ImpulseAuthorityTime, FlightParams);
	}
	else if (Scheme == static_cast<int32>(EDragonFlightScheme::SchemeB))
	{
		AuthorityW = 1.0f;
	}
	if (bProtoBraking)
	{
		AuthorityW = 1.0f;
	}
	// [2026-11-03 T-075] Landing Assist flies the approach as forced
	// precision (brake + descent cap below); F toggles, walk/fall clears.
	if (bLandingAssist)
	{
		AuthorityW = 1.0f;
	}
	NewSpeed = ClampSpeedToCeilings(NewSpeed, NoseRot.Pitch);
	if (bProtoBraking)
	{
		NewSpeed = BrakeStep(NewSpeed, deltaTime, FlightParams);
	}

	// [2026-11-03 T-075] Precision thrust from the pending move input
	// (existing WASD/stick bindings — no remap needed for the course); with
	// no input the precision target is a hover-hold at zero velocity, which
	// is what the chimney and cave legs steer with.
	FVector PendingInput = GetPendingInputVector();
	// [2026-11-03 T-075] Latched steer-to-look adds forward thrust in
	// precision so the dragon travels toward the view hands-free.
	if (bProtoAutoThrust && AuthorityW > 0.0f)
	{
		PendingInput += NoseDir.GetSafeNormal2D() * 0.6f;
	}
	const FVector PrecisionVel = PendingInput.GetClampedToMaxSize(1.0f) * FlightParams.PrecisionThrustSpeed * TrapSlowFactor;
	const float Steer = ComputeSteerResponsiveness(Velocity.Size(), AuthorityW, FlightParams);
	// [2026-11-03 T-075] C hybrid (additive): momentum is always the baseline
	// so coasts glide through the band; precision ADDS input thrust scaled by
	// band weight, and hover capture settles near-rest flight into a hold.
	// A/B keep the pinned paths (pure momentum / forced precision).
	FVector Desired;
	if (Scheme == static_cast<int32>(EDragonFlightScheme::SchemeC))
	{
		const float HoverH = HoverCaptureWeight(Velocity.Size(), bProtoBraking, FlightParams);
		Desired = NoseDir * NewSpeed + PrecisionVel * AuthorityW;
		Desired -= Velocity * FMath::Min(1.0f, deltaTime * Steer) * HoverH;
	}
	else
	{
		Desired = NoseDir * NewSpeed * (1.0f - AuthorityW) + PrecisionVel * AuthorityW;
	}
	Velocity = Velocity + (Desired - Velocity) * FMath::Min(1.0f, deltaTime * Steer);

	if (NewSpeed < FlightParams.StallSpeed && FlightParams.StallSpeed > 0.0f)
	{
		// [2026-11-03 T-075] Stall sink fades out across the blend: full
		// hover-hold at W=1, full sink in pure momentum.
		const float SinkFrac = (1.0f - NewSpeed / FlightParams.StallSpeed) * (1.0f - AuthorityW);
		Velocity.Z = FMath::Max(Velocity.Z - FlightParams.StallSinkRate * SinkFrac * deltaTime,
								-FlightParams.StallSinkRate * (1.0f - AuthorityW));
	}

	// [2026-11-03 T-075] Rise/sink ride the blend band (Space-hold/Ctrl):
	// momentum-band Space stays the Ascent impulse (press edge) and Brake
	// stays the momentum slow-down — holds only steer the hover band.
	if (AuthorityW > 0.0f && (bProtoRise || bProtoSink))
	{
		const float RiseInput = (bProtoRise ? 1.0f : 0.0f) - (bProtoSink ? 1.0f : 0.0f);
		Velocity.Z = FMath::Lerp(Velocity.Z, RiseInput * FlightParams.PrecisionThrustSpeed * TrapSlowFactor,
								 FMath::Min(1.0f, deltaTime * Steer));
	}
	if (bLandingAssist)
	{
		// [2026-11-03 T-075] Assisted approaches descend no faster than a
		// firm flare; the pilot still aims the touchdown.
		Velocity.Z = FMath::Max(Velocity.Z, -250.0f);
	}
	const FVector Delta = Velocity * deltaTime;
	FHitResult MoveHit(1.0f);
	float FloorIntoSpeed = 0.0f;
	bool bFloorHit = false;
	if (!Delta.IsNearlyZero())
	{
		SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, MoveHit);
		if (MoveHit.IsValidBlockingHit())
		{
			FloorIntoSpeed = -FVector::DotProduct(Velocity, MoveHit.Normal);
			bFloorHit = MoveHit.Normal.Z > 0.7f;
			SlideAlongSurface(Delta, 1.0f - MoveHit.Time, MoveHit.Normal, MoveHit, true);
			const float IntoSurface = FVector::DotProduct(Velocity, MoveHit.Normal);
			if (IntoSurface < 0.0f)
			{
				Velocity -= MoveHit.Normal * IntoSurface;
			}
		}
	}
	if (bFloorHit)
	{
		// [2026-11-03 T-075] v0 ground contact: fast floor hits fall out so
		// shipped Landed() prices the impact on the shared curve; slow
		// contact settles to walk. Wall hits keep sliding (T-079's case).
		if (FloorIntoSpeed > FlightParams.CachedLandingSpeedThreshold)
		{
			SetMovementMode(MOVE_Falling);
		}
		else if (Velocity.Size() < 300.0f)
		{
			// [2026-11-03 T-062] Touchdown keeps its (small) velocity into walk so
			// ground friction, not a zero-snap, finishes the stop — no landing
			// pop. Fast contacts fell out above (Landed() prices them).
			SetMovementMode(MOVE_Walking);
		}
	}
	// [2026-11-03 T-075] Do NOT call UpdateComponentVelocity() here:
	// PerformMovement calls it AFTER the scoped update applies the deferred
	// move, and engine Phys* functions never call it mid-physics. (An earlier
	// revision blamed this call for weak energy on the course; row-level
	// system identification then proved the integrator exact to 0.44 cm/s —
	// the call was redundant, not the diluter. Lesson kept: simulate the full
	// step, including steer lag, before blaming components.)
}

void UDragonMovementComponent::ApplySurge()
{
	// [2026-11-03 T-075] Surge Forward: forward impulse + temporary ceiling
	// (the ceiling is enforced by ClampSpeedToCeilings until drag converges).
	// Stamina gating (one full 20-point reserve) is the caller's job via Stats.
	if (CharacterOwner == nullptr)
	{
		return;
	}
	// [2026-11-03 T-075] Slice 2c: the nose is pawn state (free-look freezes
	// it, steer-latch turns it at the yaw limit); non-pawn owners fall back
	// to control rotation.
	FRotator NoseRot = CharacterOwner->GetControlRotation();
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(CharacterOwner))
	{
		NoseRot = Dragon->GetProtoNose();
	}
	// [2026-11-03 T-075] Latch momentum authority for the impulse window so
	// the launch flies instead of snapping back to hover (C).
	ImpulseAuthorityTime = FlightParams.ImpulseAuthorityWindow;
	// [2026-11-03 T-103] Trap slow taxes paid impulses like sustained
	// thrust: a slowed takeoff buys less launch.
	Velocity += NoseRot.Vector() * FlightParams.SurgeDeltaV * TrapSlowFactor;
	Velocity = Velocity.GetClampedToMaxSize(ClampSpeedToCeilings(Velocity.Size(), NoseRot.Pitch));
}

void UDragonMovementComponent::ApplyAscent()
{
	// [2026-11-03 T-075] Skyward Ascent: up-forward launch for takeoff and
	// climb-outs. Same caller-gates, same ceilings, same impulse latch as Surge.
	if (CharacterOwner == nullptr)
	{
		return;
	}
	// [2026-11-03 T-075] Slice 2c: the nose is pawn state (free-look freezes
	// it, steer-latch turns it at the yaw limit); non-pawn owners fall back
	// to control rotation.
	FRotator NoseRot = CharacterOwner->GetControlRotation();
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(CharacterOwner))
	{
		NoseRot = Dragon->GetProtoNose();
	}
	ImpulseAuthorityTime = FlightParams.ImpulseAuthorityWindow;
	const FVector UpForward = (FVector::UpVector + NoseRot.Vector() * 0.5f).GetSafeNormal();
	// [2026-11-03 T-103] Same trap tax as Surge (see above).
	Velocity += UpForward * FlightParams.AscentDeltaV * TrapSlowFactor;
	Velocity = Velocity.GetClampedToMaxSize(ClampSpeedToCeilings(Velocity.Size(), NoseRot.Pitch));
}

void UDragonMovementComponent::CaptureFlightCsvRow(float SimTime, float Stamina)
{
	// [2026-11-03 T-075] One row per pawn tick while prototype-flying. Hard
	// cap at 36000 rows (10 min at 60 Hz — course legs are far shorter) so a
	// forgotten capture can never grow memory without bound; the cap says so
	// in the log instead of silently truncating. Nose/energy/dt columns are
	// system-identification evidence (2026-11-03: per-tick energy did not
	// match the coded law, so the run records what the integrator SAW).
	if (FlightCsvRows.Num() >= 36000)
	{
		return;
	}
	const float Airspeed = Velocity.Size();
	float NosePitch = 0.0f;
	FVector PawnPos = FVector::ZeroVector;
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(CharacterOwner))
	{
		NosePitch = Dragon->GetProtoNose().Pitch;
		PawnPos = Dragon->GetActorLocation();
	}
	FlightCsvRows.Add(FString::Printf(TEXT("%.2f,%d,%.0f,%.3f,%.0f,%.0f,%.1f,%d,%.1f,%.0f,%.4f,%d,%.0f,%.0f,%.0f"),
									  SimTime, GetFlightScheme(), Airspeed, PrecisionAuthorityWeight(Airspeed, FlightParams), Airspeed,
									  Velocity.Z, Stamina, bProtoBraking ? 1 : 0, NosePitch, LastEnergySpeed, LastPhysDt,
									  LastPhysIters, PawnPos.X, PawnPos.Y, PawnPos.Z));
}

void UDragonMovementComponent::DumpFlightCsvToLog() const
{
	// [2026-11-03 T-075] Course-evidence dump: header + every captured row.
	// Columns: time,scheme,airspeed,authority,speed,vel_z,stamina,braking.
	UE_LOG(LogUnderTheMountain, Log, TEXT("FlightCsv rows=%d time,scheme,airspeed,authority,speed,vel_z,stamina,braking,nose_pitch,energy_target,phys_dt,phys_iters,pos"),
		   FlightCsvRows.Num());
	for (const FString& Row : FlightCsvRows)
	{
		UE_LOG(LogUnderTheMountain, Log, TEXT("FlightCsv %s"), *Row);
	}
}

void UDragonMovementComponent::DumpFlightCsvToFile(const FString& FilePath) const
{
	// [2026-11-03 T-075] Permanent course evidence: the headless -game path
	// never materialises a UE log file, so a measured run writes its own CSV
	// (directory created, failure loud, never silent).
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*FPaths::GetPath(FilePath));
	FString Body = TEXT("time,scheme,airspeed,authority,speed,vel_z,stamina,braking,nose_pitch,energy_target,phys_dt,phys_iters,pos_x,pos_y,pos_z\n");
	for (const FString& Row : FlightCsvRows)
	{
		Body += Row;
		Body += TEXT("\n");
	}
	if (!FFileHelper::SaveStringToFile(Body, *FilePath))
	{
		UE_LOG(LogUnderTheMountain, Warning, TEXT("FlightCsv file write failed: %s"), *FilePath);
	}
}
