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
// [2026-11-04 T-020] Correct case (was PlatformFilemanager): the UAT
// [2026-11-04 T-020] packaging build failed on the miscased include while
// [2026-11-04 T-020] the gate's -NoUba builds pass — spell it exactly.
#include "HAL/PlatformFileManager.h"
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

float UDragonMovementComponent::LiftAccel(float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-04 FLIGHT-PHYSICS] Coefficient derived, not tuned: lift
	// must equal gravity at CruiseLiftSpeed, so k = 980 / 1600^2 and
	// Lift = k * v^2. Clamped so dive speeds cannot balloon.
	const float V = FMath::Max(0.0f, Airspeed);
	const float Ref = FMath::Max(1.0f, Params.CruiseLiftSpeed);
	const float Raw = Params.GravityCM * FMath::Square(V / Ref);
	return FMath::Min(Raw, FMath::Max(0.0f, Params.MaxLiftAccel));
}

float UDragonMovementComponent::LiftAccel(float Airspeed)
{
	return LiftAccel(Airspeed, FDragonFlightParams());
}

float UDragonMovementComponent::LiftAccel(float Airspeed, float BankDegrees,
										  const FDragonFlightParams& Params)
{
	// [2026-11-04 FLIGHT-PHYSICS] Only cos(bank) of lift opposes
	// gravity; the rest is what turns the flight path (coordinated
	// turn below). Bank past 90 deg earns no vertical lift.
	const float CosBank = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(BankDegrees, -90.0f, 90.0f)));
	return LiftAccel(Airspeed, Params) * CosBank;
}

float UDragonMovementComponent::LiftVerticalAccel(float Airspeed, float BankDegrees,
												  const FDragonFlightParams& Params)
{
	return LiftAccel(Airspeed, BankDegrees, Params) - Params.GravityCM;
}

float UDragonMovementComponent::TurnAuthority(float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-04 FLIGHT-PHYSICS] Smoothstep fade 200 -> 600 cm/s,
	// squared so the low end is genuinely near zero: zero at/below
	// 200, ~0.02 at 300, ~0.25 at 400, 1 at/above 600.
	const float Lo = Params.TurnAuthorityZeroSpeed;
	const float Hi = FMath::Max(Lo + 1.0f, Params.TurnAuthorityFullSpeed);
	const float T = FMath::Clamp((FMath::Max(0.0f, Airspeed) - Lo) / (Hi - Lo), 0.0f, 1.0f);
	const float S = T * T * (3.0f - 2.0f * T);
	return S * S;
}

float UDragonMovementComponent::TurnAuthority(float Airspeed)
{
	return TurnAuthority(Airspeed, FDragonFlightParams());
}

float UDragonMovementComponent::CoordinatedYawRateDeg(float BankDegrees, float Airspeed,
													  const FDragonFlightParams& Params)
{
	// [2026-11-04 FLIGHT-PHYSICS] Real coordinated-turn law:
	// yaw (rad/s) = g * tan(bank) / v. The denominator floor (600)
	// plus TurnAuthority mean slow flight cannot spin; the cap keeps
	// the shipped YawRateForAirspeed feel ceiling. Sign: the bank
	// convention banks negative for a positive (yaw-increasing) turn.
	const float Authority = TurnAuthority(Airspeed, Params);
	if (Authority <= 0.0f)
	{
		return 0.0f;
	}
	const float V = FMath::Max(FMath::Max(0.0f, Airspeed), Params.TurnAuthorityFullSpeed);
	const float RawDeg = -FMath::RadiansToDegrees(Params.GravityCM *
												  FMath::Tan(FMath::DegreesToRadians(BankDegrees)) / V);
	const float Cap = YawRateForAirspeed(Airspeed, Params);
	return FMath::Clamp(RawDeg * Authority, -Cap, Cap);
}

float UDragonMovementComponent::CoordinatedYawRateDeg(float BankDegrees, float Airspeed)
{
	return CoordinatedYawRateDeg(BankDegrees, Airspeed, FDragonFlightParams());
}

float UDragonMovementComponent::PlanePitchRateDeg(float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-04 PLANE-MODEL] Full-deflection pitch rate scaled by
	// turn authority: 75 deg/s at cruise, near zero below 300 cm/s.
	return Params.PitchCommandRateDegPerSec * TurnAuthority(Airspeed, Params);
}

float UDragonMovementComponent::PlaneRollRateDeg(float Airspeed, const FDragonFlightParams& Params)
{
	return Params.RollCommandRateDegPerSec * TurnAuthority(Airspeed, Params);
}

float UDragonMovementComponent::PlaneYawRateDeg(float Airspeed, const FDragonFlightParams& Params)
{
	// [2026-11-04 PLANE-MODEL] Right-stick direct yaw: small (30 deg/s
	// full) and authority-scaled like the other rates.
	return Params.RightStickYawRateDegPerSec * TurnAuthority(Airspeed, Params);
}

FQuat UDragonMovementComponent::StepFlightOrientation(const FQuat& Orientation, float PitchInput,
													  float RollInput, float YawInput, float Airspeed,
													  float DeltaSeconds,
													  const FDragonFlightParams& Params)
{
	// [2026-11-04 PLANE-MODEL] One tick of nose control in the local
	// flight frame. No pitch/roll clamp of any kind: holding pitch
	// input rotates through vertical to inverted and around (a loop),
	// holding roll input rolls past inverted. Positive pitch input
	// pitches the nose up; positive roll input rolls right (right wing
	// down, i.e. negative UE roll); positive yaw input yaws right.
	if (DeltaSeconds <= 0.0f)
	{
		return Orientation.GetNormalized();
	}
	const float PitchStep = FMath::Clamp(PitchInput, -1.0f, 1.0f) * PlanePitchRateDeg(Airspeed, Params) * DeltaSeconds;
	const float RollStep = -FMath::Clamp(RollInput, -1.0f, 1.0f) * PlaneRollRateDeg(Airspeed, Params) * DeltaSeconds;
	const float YawStep = FMath::Clamp(YawInput, -1.0f, 1.0f) * PlaneYawRateDeg(Airspeed, Params) * DeltaSeconds;
	const FQuat Delta(FRotator(PitchStep, YawStep, RollStep));
	return (Orientation.GetNormalized() * Delta).GetNormalized();
}

FVector UDragonMovementComponent::LiftVector(const FQuat& Orientation, float Airspeed,
											 const FDragonFlightParams& Params)
{
	// [2026-11-04 PLANE-MODEL] Lift is perpendicular to the wings:
	// along the dragon's own up, full magnitude at any bank. Banked,
	// its horizontal component steers the flight path; inverted, it
	// pulls toward the ground — which is what makes a loop close
	// instead of ballooning.
	return Orientation.GetNormalized().GetUpVector() * LiftAccel(Airspeed, Params);
}

float UDragonMovementComponent::BankedLiftHeadingRateDeg(float BankDegrees, float Airspeed,
														 const FDragonFlightParams& Params)
{
	// [2026-11-04 PLANE-MODEL] Emergent heading rate for a held bank:
	// the lift vector's horizontal component (Lift*sin(bank)) is the
	// centripetal acceleration, so heading rate = that / v. Sign:
	// negative bank (right wing down in our roll convention is
	// positive input, see StepFlightOrientation) — here the caller
	// passes the UE roll angle, whose sign yaws the heading oppositely.
	const float V = FMath::Max(0.0f, Airspeed);
	if (V <= 1.0f)
	{
		return 0.0f;
	}
	const float Horizontal = LiftAccel(V, Params) * FMath::Sin(FMath::DegreesToRadians(BankDegrees));
	return -FMath::RadiansToDegrees(Horizontal / V);
}

FVector UDragonMovementComponent::StepPlaneVelocity(const FVector& Velocity, const FQuat& Orientation,
													float DeltaSeconds,
													const FDragonFlightParams& Params)
{
	return StepPlaneVelocity(Velocity, Orientation, DeltaSeconds, Params, 0.0f);
}

FVector UDragonMovementComponent::StepPlaneVelocity(const FVector& Velocity, const FQuat& Orientation,
													float DeltaSeconds,
													const FDragonFlightParams& Params, float PitchInput)
{
	// [2026-11-04 PLANE-MODEL] Momentum step: velocity changes only
	// by lift (perpendicular to the wings) plus gravity. Gravity
	// along the flight path IS the energy exchange (climb bleeds,
	// dive gains); gravity across the path and banked lift bend the
	// path, so heading change emerges with no yaw formula. Drag
	// exists only above the pitch-dependent terminal (same terminal
	// law as IntegrateEnergyStep), so a sub-terminal glide retains.
	if (DeltaSeconds <= 0.0f)
	{
		return Velocity;
	}
	const FQuat Q = Orientation.GetNormalized();
	// [2026-11-04 PLANE-MODEL] Angle of attack: held pull scales lift
	// 1x->2x along the dragon's up (push unloads to 0x), so a loop
	// pull generates the centripetal force the turn demands.
	const float LiftScale = FMath::Clamp(1.0f + FMath::Clamp(PitchInput, -1.0f, 1.0f), 0.0f, 2.0f);
	// [2026-11-04 PLANE-MODEL fix] Flap thrust: a dragon is not a glider
	// — pulling up means flapping, which adds forward thrust along the
	// nose (600 cm/s^2 at full pull). Without it a loop bleeds to stall
	// at the top (proven: pitchTraversal 116deg, exit speed 99).
	const FVector FlapThrust =
		Q.GetForwardVector() * (600.0f * FMath::Max(0.0f, PitchInput));
	const FVector Accel = LiftVector(Q, Velocity.Size(), Params) * LiftScale + FlapThrust +
						  FVector(0.0f, 0.0f, -Params.GravityCM);
	FVector Out = Velocity + Accel * DeltaSeconds;
	const float SinPitch = FMath::Clamp(Q.GetForwardVector().Z, -1.0f, 1.0f);
	float Terminal;
	if (SinPitch <= 0.0f)
	{
		Terminal = Params.LevelCruiseTerminal + (Params.SteepDiveTerminal - Params.LevelCruiseTerminal) * (-SinPitch);
	}
	else
	{
		Terminal = Params.LevelCruiseTerminal * (1.0f - 0.7f * SinPitch);
	}
	const float Speed = Out.Size();
	if (Speed > Terminal && Speed > KINDA_SMALL_NUMBER)
	{
		const float Over = Speed - Terminal;
		Out = Out / Speed * (Terminal + Over * FMath::Exp(-DeltaSeconds * Params.DragConvergenceRate));
	}
	return Out;
}

FQuat UDragonMovementComponent::WeathervaneOrientation(const FQuat& Orientation, const FVector& Velocity,
													   float DeltaSeconds,
													   const FDragonFlightParams& Params)
{
	return WeathervaneOrientation(Orientation, Velocity, DeltaSeconds, Params, 1.0f);
}

FQuat UDragonMovementComponent::WeathervaneOrientation(const FQuat& Orientation, const FVector& Velocity,
													   float DeltaSeconds,
													   const FDragonFlightParams& Params, float RateScale)
{
	// [2026-11-04 PLANE-MODEL] Aerodynamic stability: the nose eases
	// onto the velocity direction while keeping the current roll
	// (up projected perpendicular to the new forward). This is what
	// carries the heading around in a banked turn — banked lift bends
	// the velocity, the weathervane follows it — at the steer
	// responsiveness rate, so loop pitch input (75 deg/s) always
	// outruns it and a loop cannot be weathervaned flat.
	const float Speed = Velocity.Size();
	if (DeltaSeconds <= 0.0f || Speed < Params.TurnAuthorityZeroSpeed)
	{
		return Orientation.GetNormalized();
	}
	const FQuat Q = Orientation.GetNormalized();
	const FVector Forward = Velocity / Speed;
	FVector Up = Q.GetUpVector() - Forward * FVector::DotProduct(Q.GetUpVector(), Forward);
	if (!Up.Normalize())
	{
		Up = Q.GetUpVector();
	}
	const FQuat Desired = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
	const float Alpha = FMath::Min(1.0f, DeltaSeconds * ComputeSteerResponsiveness(Speed, 0.0f, Params) *
											 FMath::Clamp(RateScale, 0.0f, 1.0f));
	return FQuat::Slerp(Q, Desired, Alpha).GetNormalized();
}

float UDragonMovementComponent::SteerStepDegrees(float PendingDegrees, float MaxRateDegPerSec,
												 float DeltaSeconds)
{
	// [2026-11-04 FLIGHT-VIGOR] RMB sailing consumes mouse demand at the
	// airspeed yaw-rate budget per tick; leftover demand stays queued
	// in the pawn, so a flick keeps turning at the cap instead of
	// snapping the nose (or being dropped).
	if (DeltaSeconds <= 0.0f || MaxRateDegPerSec <= 0.0f)
	{
		return 0.0f;
	}
	const float Budget = MaxRateDegPerSec * DeltaSeconds;
	return FMath::Clamp(PendingDegrees, -Budget, Budget);
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
	return BrakeStep(Speed, DeltaSeconds, Params, 1.0f);
}

float UDragonMovementComponent::BrakeStep(float Speed, float DeltaSeconds,
										  const FDragonFlightParams& Params, float Strength)
{
	if (DeltaSeconds <= 0.0f)
	{
		return FMath::Max(0.0f, Speed);
	}
	return FMath::Max(0.0f, Speed - Params.BrakeDecel * FMath::Clamp(Strength, 0.0f, 1.0f) *
										DeltaSeconds);
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

UDragonMovementComponent::FWallImpactResult UDragonMovementComponent::ResolveWallImpact(
	float IntoSurfaceSpeed, const UDragonStatsComponent& Stats)
{
	// [2026-11-03 T-079] A wall hit prices like a head-on landing at
	// the into-surface speed: the normal component IS the impact, so
	// the angle is 90 by construction (no asin reconstruction needed).
	// Damage is the shipped landing number at that speed; the keep
	// fraction is one-minus-severity (graze untouched, lawn-dart
	// crumple). Mapping, not tuning: the only numbers are Stats'
	// own curve `[tuning: PT-STAM]`, which T-105 already owns.
	FWallImpactResult Result;
	Result.DamageHP = Stats.LandingDamageForImpactSpeed(FMath::Max(0.0f, IntoSurfaceSpeed));
	const float Severity = LandingSeverity(FMath::Max(0.0f, IntoSurfaceSpeed), 90.0f, Stats);
	Result.KeepFraction = FMath::Clamp(1.0f - Severity, 0.0f, 1.0f);
	return Result;
}

UDragonMovementComponent::FWallImpact UDragonMovementComponent::ConsumeWallImpact()
{
	FWallImpact Hit = PendingWallImpact;
	PendingWallImpact = FWallImpact();
	return Hit;
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

	// [2026-11-04 PLANE-MODEL] The flight frame is a full quaternion
	// on the pawn (nose pitch + wing roll, unclamped); non-pawn owners
	// fall back to control rotation. Lift runs along the dragon's up
	// from that frame, so banked/inverted flight bends the velocity.
	FQuat FlightQuat = CharacterOwner->GetControlRotation().Quaternion();
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(CharacterOwner))
	{
		FlightQuat = Dragon->GetProtoFlightQuat();
	}
	const FRotator NoseRot = FlightQuat.Rotator();
	const FVector NoseDir = FlightQuat.GetForwardVector();

	float NewSpeed = IntegrateEnergyStep(Velocity.Size(), FMath::Asin(FMath::Clamp(NoseDir.Z, -1.0f, 1.0f)), deltaTime, FlightParams);
	LastEnergySpeed = NewSpeed;
	LastPhysDt = deltaTime;
	LastPhysIters = Iterations;
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
		AuthorityW = FMath::Max(AuthorityW, FMath::Clamp(BrakeStrength, 0.0f, 1.0f));
	}
	if (bLandingAssist)
	{
		AuthorityW = 1.0f;
	}

	FVector PendingInput = GetPendingInputVector();
	if (bProtoAutoThrust && AuthorityW > 0.0f)
	{
		PendingInput += NoseDir.GetSafeNormal2D() * 0.6f;
	}
	const FVector PrecisionVel = PendingInput.GetClampedToMaxSize(1.0f) * FlightParams.PrecisionThrustSpeed * TrapSlowFactor;
	const float Steer = ComputeSteerResponsiveness(Velocity.Size(), AuthorityW, FlightParams);
	// [2026-11-04 PLANE-MODEL] Momentum velocity is the lift+gravity
	// step (no nose-snap, no yaw formula); precision keeps its steered
	// thrust velocity; C blends the two velocities by AuthorityW and
	// hover capture still settles near-rest C flight. Brake/ceilings
	// scale the plane velocity's magnitude, never its direction.
	FVector PlaneVel = StepPlaneVelocity(Velocity, FlightQuat, deltaTime, FlightParams, ProtoPitchInput);
	{
		float PlaneSpeed = ClampSpeedToCeilings(PlaneVel.Size(), NoseRot.Pitch);
		if (bProtoBraking)
		{
			PlaneSpeed = BrakeStep(PlaneSpeed, deltaTime, FlightParams, BrakeStrength);
		}
		if (PlaneVel.Size() > KINDA_SMALL_NUMBER)
		{
			PlaneVel = PlaneVel.GetSafeNormal() * PlaneSpeed;
		}
	}
	const FVector PrecisionSteered = Velocity + (PrecisionVel - Velocity) * FMath::Min(1.0f, deltaTime * Steer);
	// [2026-11-04 PLANE-MODEL] Precision correction is gated on an
	// actual precision demand (input, brake, or Landing Assist). The
	// first course run proved why: lerping the plane velocity toward
	// a zero-input precision target inside the blend band is a huge
	// invisible drag — climbs/loop bled 2000->0 cm/s and froze
	// mid-air. With no precision demand, C flies the pure plane step
	// through the band; hover capture (below, near-rest only) still
	// settles slow flight, and stall recovery is gravity's job.
	const bool bPrecisionActive = PrecisionVel.SizeSquared() > 1.0f || bProtoBraking || bLandingAssist;
	if (Scheme == static_cast<int32>(EDragonFlightScheme::SchemeC))
	{
		Velocity = PlaneVel;
		if (bPrecisionActive)
		{
			Velocity += (PrecisionVel - Velocity) * FMath::Min(1.0f, deltaTime * Steer) * AuthorityW;
		}
		const float HoverH = HoverCaptureWeight(Velocity.Size(), bProtoBraking, FlightParams);
		Velocity -= Velocity * FMath::Min(1.0f, deltaTime * Steer) * HoverH;
	}
	else if (AuthorityW >= 1.0f)
	{
		Velocity = PrecisionSteered;
	}
	else
	{
		Velocity = PlaneVel;
	}
	NewSpeed = Velocity.Size();

	if (NewSpeed < FlightParams.StallSpeed && FlightParams.StallSpeed > 0.0f && AuthorityW < 1.0f && !bPrecisionActive)
	{
		// [2026-11-04 PLANE-MODEL] Stall sag cap (was FLIGHT-VIGOR 300):
		// slow momentum flight sinks, bounded, while the pilot dives
		// or spends an impulse to recover. Precision band exempt.
		Velocity.Z = FMath::Max(Velocity.Z, -FlightParams.StallSinkRate * (1.0f - AuthorityW));
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
			if (!bFloorHit && FloorIntoSpeed > 0.0f)
			{
				// [2026-11-03 T-079] Wall hit (anything that is not
				// floor): record the hardest substep for the pawn to
				// price, and kill momentum by severity now, in the
				// same step, so wall-slide farming pays every touch.
				// Landing Assist never shields this path: assist caps
				// descent speed, it does not reroute wall contacts.
				if (!PendingWallImpact.bPending ||
					FloorIntoSpeed > PendingWallImpact.IntoSpeed)
				{
					PendingWallImpact.IntoSpeed = FloorIntoSpeed;
					PendingWallImpact.bPending = true;
				}
				if (CurveSource != nullptr)
				{
					const FWallImpactResult Wall =
						ResolveWallImpact(FloorIntoSpeed, *CurveSource);
					Velocity *= Wall.KeepFraction;
				}
				else if (!bWarnedMissingCurve)
				{
					bWarnedMissingCurve = true;
					UE_LOG(LogUnderTheMountain, Warning,
						   TEXT("Wall impact unpriced: no Stats curve pushed (BeginPlay) — sliding only."));
				}
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
	// Stamina gating (one full impulse reserve) is the caller's job via Stats.
	// [2026-11-04 T-124] The reserve cost is Stats->ImpulseReserveCost (10,
	// was a 20.0f literal at the call sites).
	if (CharacterOwner == nullptr)
	{
		return;
	}
	// [2026-11-03 T-075] Slice 2c: the nose is pawn state
	// [2026-11-04 FLIGHT-VIGOR] (RMB steer-held sails it, steer-latch
	// turns it at the yaw limit); non-pawn owners fall back
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
	// [2026-11-03 T-075] Slice 2c: the nose is pawn state
	// [2026-11-04 FLIGHT-VIGOR] (RMB steer-held sails it, steer-latch
	// turns it at the yaw limit); non-pawn owners fall back
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
	float BankDeg = 0.0f;
	float UpZ = 1.0f;
	float FwdZ = 0.0f;
	float NoseYaw = 0.0f;
	FVector PawnPos = FVector::ZeroVector;
	if (const ADragonCharacter* Dragon = Cast<ADragonCharacter>(CharacterOwner))
	{
		const FQuat Q = Dragon->GetProtoFlightQuat();
		NosePitch = Q.Rotator().Pitch;
		BankDeg = Dragon->GetProtoBankDegrees();
		UpZ = Q.GetUpVector().Z;
		FwdZ = Q.GetForwardVector().Z;
		NoseYaw = Q.Rotator().Yaw;
		PawnPos = Dragon->GetActorLocation();
	}
	FlightCsvRows.Add(FString::Printf(TEXT("%.2f,%d,%.0f,%.3f,%.0f,%.0f,%.1f,%d,%.1f,%.0f,%.4f,%d,%.0f,%.0f,%.0f,%.1f,%.3f,%.3f,%.1f"),
									  SimTime, GetFlightScheme(), Airspeed, PrecisionAuthorityWeight(Airspeed, FlightParams), Airspeed,
									  Velocity.Z, Stamina, bProtoBraking ? 1 : 0, NosePitch, LastEnergySpeed, LastPhysDt,
									  LastPhysIters, PawnPos.X, PawnPos.Y, PawnPos.Z, BankDeg, UpZ, FwdZ, NoseYaw));
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
	FString Body = TEXT("time,scheme,airspeed,authority,speed,vel_z,stamina,braking,nose_pitch,energy_target,phys_dt,phys_iters,pos_x,pos_y,pos_z,bank_deg,up_z,fwd_z,nose_yaw\n");
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
