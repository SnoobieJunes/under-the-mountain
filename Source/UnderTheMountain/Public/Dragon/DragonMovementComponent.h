// Copyright Under the Mountain contributors.
// Purpose: T-075 flight-spike movement chassis — Scheme A (momentum) / C
// (hybrid) / B (forced precision) prototyped on one custom movement
// component behind utm.flight.scheme. Created 2026-11-03 (T-075).
// Invariants: all flight math lives in pure statics over
// FDragonFlightParams (no world, no clock, no stamina pools) so automation
// tests prove the model headlessly; the pawn integrates, Stats spends
// stamina — this component never writes either pool. Slice 2a adds the
// PhysCustom integrator, impulses and CSV capture plus the pawn attach —
// still inert on scheme 0, so shipped flight is unchanged by default.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DragonMovementComponent.generated.h"

// [2026-11-03 T-075] Prototype flight scheme selector, mirrored by the
// utm.flight.scheme console var (0 = shipped flight, prototype inert).
UENUM(BlueprintType)
enum class EDragonFlightScheme : uint8
{
	Shipped UMETA(DisplayName = "Shipped (prototype inert)"),
	SchemeA UMETA(DisplayName = "Scheme A: pure pitch/momentum"),
	SchemeC UMETA(DisplayName = "Scheme C: speed-blended hybrid"),
	SchemeB UMETA(DisplayName = "Scheme B: forced precision"),
};

// [2026-11-03 T-075] Starting tuning for the prototype course, from
// docs/FLIGHT-SPIKE.md "Speed and band starting values". Every value is
// [tuning: PT-STAM]: course evidence moves them, preference does not.
USTRUCT(BlueprintType)
struct FDragonFlightParams
{
	GENERATED_BODY()

	// [2026-11-03 T-075] Full precision authority at or below this airspeed.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float PrecisionAuthorityMaxSpeed = 650.0f;

	// [2026-11-03 T-075] Full momentum authority at or above this airspeed;
	// the 650..1600 band blends continuously between them.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float MomentumAuthorityMinSpeed = 1600.0f;

	// [2026-11-03 T-075] Level-flight terminal speed; below-terminal glide
	// retains speed (no drag), above-terminal drag converges to terminal.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float LevelCruiseTerminal = 2400.0f;

	// [2026-11-03 T-075] Temporary ceiling while Surge is active.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float SurgeCeiling = 3200.0f;

	// [2026-11-03 T-075] Vertical-dive terminal; 3200/4800 are the course
	// falsifiers (spike doc: turn-radius derivation from this number).
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float SteepDiveTerminal = 4000.0f;

	// [2026-11-03 T-075] Yaw authority fades 90 deg/s (precision) to
	// 55 deg/s (dive) across the blend band.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float PrecisionYawRateDeg = 90.0f;

	// [2026-11-03 T-075] See PrecisionYawRateDeg.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float DiveYawRateDeg = 55.0f;

	// [2026-11-03 T-075] Gravity projected onto the flight path (cm/s^2).
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float GravityCM = 980.0f;

	// [2026-11-03 T-075] Exponential convergence rate toward terminal when
	// above it (1/s). Below terminal there is no drag by design.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float DragConvergenceRate = 1.5f;

	// [2026-11-03 T-075] Discrete impulse delta-v values (cm/s). Stamina
	// gating (one full 20-point reserve each) is the caller's job via Stats.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float SurgeDeltaV = 800.0f;

	// [2026-11-03 T-075] See SurgeDeltaV.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float AscentDeltaV = 900.0f;

	// [2026-11-03 T-075] Held-brake decel (cm/s^2); holding brake also forces
	// precision authority before airspeed has decayed.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float BrakeDecel = 2400.0f;

	// [2026-11-03 T-075] Landing-severity curve family, shared with the
	// shipped landing curve (Stats LandingSpeedThreshold/MaxDamage) so wall
	// impacts (T-079) price on the same scale. T-079 owns pawn wiring.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float LandingSpeedThreshold = 800.0f;

	// [2026-11-03 T-075] See LandingSpeedThreshold.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float LandingSpeedMaxDamage = 2200.0f;

	// [2026-11-03 T-075] Nose-follow responsiveness (1/s): how fast velocity
	// steers onto the nose-directed desired vector. B doubles it.
	// TODO(T-075): trap slow scales these params (spike: never only
	// MaxFlySpeed) — lands with the input-remap slice.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float SteerResponsiveness = 3.0f;

	// [2026-11-03 T-075] Below this airspeed the momentum model stalls and
	// sinks toward StallSinkRate descent (hover is C's low band, next slice).
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float StallSpeed = 400.0f;

	// [2026-11-03 T-075] Impulse authority window (s): after Surge/Ascent the
	// blend holds momentum authority this long so launches fly instead of
	// snapping back to hover. Provisional for the course.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float ImpulseAuthorityWindow = 1.5f;

	// [2026-11-03 T-075] See StallSpeed: maximum sink rate (cm/s, positive
	// number, applied downward).
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float StallSinkRate = 600.0f;

	// [2026-11-03 T-075] Precision-band thrust speed (cm/s): full-stick
	// deflection in the hover band commands this velocity. Matches the
	// precision cap so the blend hands off with no step.
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	float PrecisionThrustSpeed = 650.0f;
};

UCLASS(ClassGroup = (Dragon), meta = (BlueprintSpawnableComponent))
class UNDERTHEMOUNTAIN_API UDragonMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

  public:
	UDragonMovementComponent();

	// [2026-11-03 T-075] Custom-mode id for the PhysCustom flight
	// integration. The pawn enters MOVE_Custom with this mode; scheme 0
	// never enters it, so shipped flight is untouched.
	static constexpr uint8 DragonFlightCustomMode = 0;

	// [2026-11-03 T-075] Reads utm.flight.scheme (0..3, default 0 = shipped).
	// The prototype is inert until this returns non-zero.
	static int32 GetFlightScheme();

	// [2026-11-03 T-075] Precision authority weight in [0,1]: 1 at or below
	// PrecisionAuthorityMaxSpeed, 0 at or above MomentumAuthorityMinSpeed,
	// smoothstepped across the band so C's blend has no perceptible switch
	// (the spike's primary C-falsifier).
	static float PrecisionAuthorityWeight(float Airspeed, const FDragonFlightParams& Params);

	// [2026-11-03 T-075] One energy-integration step. PitchRadians > 0 is
	// nose-up: gravity bleeds speed; nose-down accelerates toward the
	// pitch-dependent terminal (level cruise -> steep dive). Below terminal
	// there is no drag (glide retention); above terminal speed converges
	// exponentially. Never returns negative. Pure: no world, no stamina.
	static float IntegrateEnergyStep(float Speed, float PitchRadians, float DeltaSeconds,
									 const FDragonFlightParams& Params);

	// [2026-11-03 T-075] Yaw authority for the current airspeed: 90 deg/s in
	// full precision fading to 55 deg/s at dive speed.
	static float YawRateForAirspeed(float Airspeed, const FDragonFlightParams& Params);

	// [2026-11-03 T-075] Nose-follow responsiveness for the current airspeed:
	// the base rate (precision-boosted like the integrator uses) floored by
	// 4x the yaw rate in rad/s, so the lag angle stays near 14 deg and a
	// sustained max-rate turn bleeds speed instead of spiralling to zero
	// (course finding 2026-11-03: 1268 to 80 cm/s in one 360). Pure.
	static float ComputeSteerResponsiveness(float Airspeed, float AuthorityW,
											const FDragonFlightParams& Params);

	// [2026-11-03 T-075] Effective precision weight: a fresh impulse latches
	// momentum authority for its window (takeoffs must FLY, not get yanked
	// back to hover — course finding 2026-11-03: C impulses evaporated on a
	// weight of 0.83). Zero/negative timer falls through to the airspeed
	// blend. Pure.
	static float BlendAuthorityWeight(float Airspeed, float ImpulseAuthorityRemaining,
									  const FDragonFlightParams& Params);

	// [2026-11-03 T-075] Hover-capture weight: 1 at/below rest, fading to 0
	// by StallSpeed; brake forces full capture. Settles near-rest flight
	// into a hold without auto-braking faster coasts (course finding
	// 2026-11-03: the multiplicative blend dragged sub-1600 flight to a
	// stop). Pure.
	static float HoverCaptureWeight(float Airspeed, bool bBraking, const FDragonFlightParams& Params);

	// [2026-11-03 T-075] One brake step: linear decel, clamped at zero.
	// Holding brake forces precision authority (caller applies) even before
	// airspeed has decayed — the adopted Brake rule.
	static float BrakeStep(float Speed, float DeltaSeconds, const FDragonFlightParams& Params);

	// [2026-11-03 T-075] Impact severity in [0,1] from speed AND impact
	// angle: only the surface-normal component (Speed * sin(angle)) prices
	// on the landing curve, so a fast shallow flare is cheap and a head-on
	// lawn-dart pays full price. AngleDeg is clamped to [0,90].
	static float LandingSeverity(float Speed, float ImpactAngleDeg, const FDragonFlightParams& Params);

	// [2026-11-03 T-075] Custom-mode integrator (Scheme A core / C high band /
	// B forced-precision): nose-directed energy flight per the pure statics
	// above, with steer lag, stall sink and brake. Collision slides; no floor
	// detection in v0 (Landing Assist lands next slice).
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	// [2026-11-03 T-075] Discrete impulses (stamina gating is the caller's
	// job via Stats' one-reserve rule): Surge along the nose, Ascent
	// up-forward. Speeds clamp to the same ceilings as the integrator.
	void ApplySurge();
	void ApplyAscent();

	// [2026-11-03 T-075] Brake feed from the pawn (Shift/LT held). While held
	// the integrator decels and (C slice) forces precision authority.
	bool bProtoBraking = false;

	// [2026-11-03 T-075] Impulse authority latch: seconds of forced momentum
	// authority remaining (set by Surge/Ascent, burned down in PhysCustom).
	float ImpulseAuthorityTime = 0.0f;

	// [2026-11-03 T-075] Slice 2c feeds (pawn-owned, read by PhysCustom):
	// Space-hold rise / Ctrl sink (blend band only), latched auto-thrust
	// (steer-to-look in precision), Landing Assist (forced precision +
	// brake + descent cap; F toggles, walk/fall clears).
	bool bProtoRise = false;
	bool bProtoSink = false;
	bool bProtoAutoThrust = false;
	bool bLandingAssist = false;

	// [2026-11-03 T-075] Course instrumentation: one row per pawn tick while
	// prototype-flying (columns: time,scheme,airspeed,authority,speed,vel_z,
	// stamina,braking,nose_pitch,energy_target,phys_dt,phys_iters,pos).
	// Dumped via FlightCsv; T-075 judges the course on these.
	void CaptureFlightCsvRow(float SimTime, float Stamina);
	void DumpFlightCsvToLog() const;
	void DumpFlightCsvToFile(const FString& FilePath) const;

	// [2026-11-03 T-075] Last integrator observation, for the CSV row above:
	// what energy target, dt and iteration count PhysCustom SAW.
	float LastEnergySpeed = 0.0f;
	float LastPhysDt = 0.0f;
	int32 LastPhysIters = 0;

	TArray<FString> FlightCsvRows;

  private:
	// [2026-11-03 T-075] Shared ceiling rule: absolute cap at the dive
	// terminal, Surge ceiling when not descending (pitch > -10 deg).
	float ClampSpeedToCeilings(float Speed, float PitchDeg) const;

  public:
	// [2026-11-03 T-075] Prototype tuning lives on the component so the
	// course moves data, not pawn code (same contract as Stats).
	UPROPERTY(EditAnywhere, Category = "Dragon|Flight")
	FDragonFlightParams FlightParams;
};
