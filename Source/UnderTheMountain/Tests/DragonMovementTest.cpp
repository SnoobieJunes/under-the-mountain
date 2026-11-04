// Copyright Under the Mountain contributors.
// Purpose: T-075 prototype flight-math contract tests — scheme defaults,
// authority blend, energy integration, yaw/brake/severity curves, exercised
// directly against UDragonMovementComponent pure statics with no world
// spawned. Created 2026-11-03 (T-075). Pawn integration (PhysCustom,
// CSV course runs) is PIE/PT-STAM territory; the math it delegates to is
// proven here.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonMovementComponent.h"
#include "Dragon/DragonStatsComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-03 T-075] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonMovementTest, "UnderTheMountain.Dragon.Movement",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonMovementTest::RunTest(const FString& Parameters)
{
	FDragonFlightParams Params;

	// [2026-11-03 T-075] Spike starting values are locked so the course
	// moves them deliberately, never by silent drift.
	TestEqual(TEXT("precision band caps at 650 cm/s"), Params.PrecisionAuthorityMaxSpeed, 650.0f);
	TestEqual(TEXT("momentum band starts at 1600 cm/s"), Params.MomentumAuthorityMinSpeed, 1600.0f);
	TestEqual(TEXT("level cruise terminal is 2400 cm/s"), Params.LevelCruiseTerminal, 2400.0f);
	TestEqual(TEXT("surge ceiling is 3200 cm/s"), Params.SurgeCeiling, 3200.0f);
	TestEqual(TEXT("steep dive terminal is 4000 cm/s"), Params.SteepDiveTerminal, 4000.0f);
	TestEqual(TEXT("precision yaw rate is 90 deg/s"), Params.PrecisionYawRateDeg, 90.0f);
	TestEqual(TEXT("dive yaw rate is 55 deg/s"), Params.DiveYawRateDeg, 55.0f);
	TestEqual(TEXT("steer responsiveness default is 3/s"), Params.SteerResponsiveness, 3.0f);
	TestEqual(TEXT("stall speed default is 400 cm/s"), Params.StallSpeed, 400.0f);
	TestEqual(TEXT("stall sink default is 300 cm/s"), Params.StallSinkRate, 300.0f);
	TestEqual(TEXT("precision thrust default is 650 cm/s"), Params.PrecisionThrustSpeed, 650.0f);
	TestEqual(TEXT("impulse window default is 1.5 s"), Params.ImpulseAuthorityWindow, 1.5f);
	// [2026-11-04 FLIGHT-PHYSICS] Lift/turn command defaults.
	TestEqual(TEXT("cruise lift reference is 1600 cm/s"), Params.CruiseLiftSpeed, 1600.0f);
	TestEqual(TEXT("lift clamp is 2450 cm/s^2"), Params.MaxLiftAccel, 2450.0f);
	TestEqual(TEXT("turn authority zero speed is 200 cm/s"), Params.TurnAuthorityZeroSpeed, 200.0f);
	TestEqual(TEXT("turn authority full speed is 600 cm/s"), Params.TurnAuthorityFullSpeed, 600.0f);
	TestEqual(TEXT("bank roll rate is 90 deg/s"), Params.BankRollRateDegPerSec, 90.0f);
	TestEqual(TEXT("pitch command rate is 60 deg/s"), Params.PitchCommandRateDegPerSec, 60.0f);

	// [2026-11-03 T-075] Impulse latch: fresh impulses read momentum (0) at
	// any airspeed; expired latches read the airspeed smoothstep.
	TestEqual(TEXT("slow with no impulse is full precision"),
			  UDragonMovementComponent::BlendAuthorityWeight(0.0f, 0.0f, Params), 1.0f);
	TestEqual(TEXT("slow with fresh impulse is momentum"),
			  UDragonMovementComponent::BlendAuthorityWeight(0.0f, 1.5f, Params), 0.0f);
	TestEqual(TEXT("fast with expired latch is momentum"),
			  UDragonMovementComponent::BlendAuthorityWeight(2000.0f, 0.0f, Params), 0.0f);
	TestEqual(TEXT("fast with fresh impulse is momentum"),
			  UDragonMovementComponent::BlendAuthorityWeight(2000.0f, 1.5f, Params), 0.0f);
	TestEqual(TEXT("mid-band expired latch blends"),
			  UDragonMovementComponent::BlendAuthorityWeight(1125.0f, -1.0f, Params),
			  UDragonMovementComponent::PrecisionAuthorityWeight(1125.0f, Params));

	// [2026-11-03 T-075] Hover capture: full hold at/below StallSpeed-300,
	// none at/above StallSpeed, brake forces capture anywhere.
	TestEqual(TEXT("rest captures without brake"),
			  UDragonMovementComponent::HoverCaptureWeight(0.0f, false, Params), 1.0f);
	TestEqual(TEXT("cruise never captures without brake"),
			  UDragonMovementComponent::HoverCaptureWeight(2000.0f, false, Params), 0.0f);
	TestEqual(TEXT("cruise captures under brake"),
			  UDragonMovementComponent::HoverCaptureWeight(2000.0f, true, Params), 1.0f);
	TestTrue(TEXT("capture fades across the band"),
			 UDragonMovementComponent::HoverCaptureWeight(250.0f, false, Params) > 0.25f && UDragonMovementComponent::HoverCaptureWeight(250.0f, false, Params) < 0.75f);

	// [2026-11-03 T-075] Steer floor: lag angle (yaw/responsiveness) stays
	// near 0.25 rad at every band — hard turns cost speed, never spiral it.
	const float SteerHover = UDragonMovementComponent::ComputeSteerResponsiveness(0.0f, 1.0f, Params);
	TestTrue(TEXT("hover steer clears the lag floor"), SteerHover >= 6.0f);
	const float SteerCruise = UDragonMovementComponent::ComputeSteerResponsiveness(2400.0f, 0.0f, Params);
	TestTrue(TEXT("cruise steer clears the lag floor"), SteerCruise >= 3.5f);
	TestTrue(TEXT("lag angle bounded at hover"),
			 FMath::DegreesToRadians(90.0f) / SteerHover <= 0.30f);
	TestTrue(TEXT("lag angle bounded at cruise"),
			 FMath::DegreesToRadians(55.0f) / SteerCruise <= 0.30f);

	// [2026-11-03 T-062] Scheme C ships by default (T-075 course pick); 0 is
	// now the legacy baseline the course/A-B legs opt into explicitly.
	TestEqual(TEXT("flight scheme defaults to C hybrid (2)"), UDragonMovementComponent::GetFlightScheme(), 2);

	// [2026-11-03 T-075] Authority blend endpoints: full precision at rest
	// and at the 650 boundary, full momentum at 1600 and beyond.
	TestEqual(TEXT("zero airspeed is full precision"),
			  UDragonMovementComponent::PrecisionAuthorityWeight(0.0f, Params), 1.0f);
	TestEqual(TEXT("650 cm/s is full precision"),
			  UDragonMovementComponent::PrecisionAuthorityWeight(650.0f, Params), 1.0f);
	TestEqual(TEXT("1600 cm/s is full momentum"),
			  UDragonMovementComponent::PrecisionAuthorityWeight(1600.0f, Params), 0.0f);
	TestEqual(TEXT("dive speed is full momentum"),
			  UDragonMovementComponent::PrecisionAuthorityWeight(4000.0f, Params), 0.0f);

	// [2026-11-03 T-075] The blend is continuous and monotone: sample the
	// band and require each step down. A perceptible mode switch fails here.
	float PrevW = UDragonMovementComponent::PrecisionAuthorityWeight(650.0f, Params);
	for (float V = 745.0f; V <= 1600.0f; V += 95.0f)
	{
		const float W = UDragonMovementComponent::PrecisionAuthorityWeight(V, Params);
		TestTrue(TEXT("authority weight never rises across the band"), W <= PrevW + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("authority weight stays in [0,1]"), W >= 0.0f && W <= 1.0f);
		PrevW = W;
	}
	const float MidW = UDragonMovementComponent::PrecisionAuthorityWeight(1125.0f, Params);
	TestTrue(TEXT("mid-band is genuinely blended"), MidW > 0.25f && MidW < 0.75f);

	// [2026-11-03 T-075] Energy: nose-down gains speed, nose-up bleeds it,
	// level glide retains (no below-terminal drag).
	const float DiveEnd = UDragonMovementComponent::IntegrateEnergyStep(1500.0f, -0.5f, 1.0f, Params);
	TestTrue(TEXT("nose-down accelerates"), DiveEnd > 1500.0f);
	const float ClimbEnd = UDragonMovementComponent::IntegrateEnergyStep(1500.0f, 0.5f, 1.0f, Params);
	TestTrue(TEXT("nose-up decelerates"), ClimbEnd < 1500.0f);
	const float GlideEnd = UDragonMovementComponent::IntegrateEnergyStep(1500.0f, 0.0f, 1.0f, Params);
	TestEqual(TEXT("level glide retains sub-terminal speed"), GlideEnd, 1500.0f);
	const float OverEnd = UDragonMovementComponent::IntegrateEnergyStep(4000.0f, 0.0f, 1.0f, Params);
	TestTrue(TEXT("above-terminal drag converges down"), OverEnd < 4000.0f && OverEnd > 2400.0f);
	TestEqual(TEXT("non-positive dt returns speed unchanged"),
			  UDragonMovementComponent::IntegrateEnergyStep(1500.0f, -0.5f, 0.0f, Params), 1500.0f);
	TestEqual(TEXT("speed never goes negative"),
			  UDragonMovementComponent::IntegrateEnergyStep(100.0f, 1.2f, 5.0f, Params), 0.0f);

	// [2026-11-03 T-075] Yaw authority follows the blend: 90 low, 55 high.
	TestEqual(TEXT("hover yaw rate is 90 deg/s"),
			  UDragonMovementComponent::YawRateForAirspeed(0.0f, Params), 90.0f);
	TestEqual(TEXT("dive yaw rate is 55 deg/s"),
			  UDragonMovementComponent::YawRateForAirspeed(4000.0f, Params), 55.0f);

	// [2026-11-03 T-075] Brake: one second at 2400 decel stops a 2400
	// cruise exactly; partial steps scale linearly; never negative.
	TestEqual(TEXT("full brake stops cruise in one second"),
			  UDragonMovementComponent::BrakeStep(2400.0f, 1.0f, Params), 0.0f);
	TestEqual(TEXT("half-second brake halves cruise"),
			  UDragonMovementComponent::BrakeStep(2400.0f, 0.5f, Params), 1200.0f);
	TestEqual(TEXT("brake never goes negative"),
			  UDragonMovementComponent::BrakeStep(100.0f, 1.0f, Params), 0.0f);

	// [2026-11-03 T-105] Severity reads the Stats curve (single source of
	// truth): below the 800 curve it is free, at 2200 head-on it is full,
	// and the same speed at a shallow angle is cheaper than steep
	// (flare != lawn-dart). T-079's pawn wiring passes Stats straight in.
	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	TestEqual(TEXT("sub-threshold impact is free"),
			  UDragonMovementComponent::LandingSeverity(700.0f, 90.0f, *Stats), 0.0f);
	TestEqual(TEXT("max-curve head-on impact is full"),
			  UDragonMovementComponent::LandingSeverity(2200.0f, 90.0f, *Stats), 1.0f);
	const float Steep = UDragonMovementComponent::LandingSeverity(2000.0f, 90.0f, *Stats);
	const float Shallow = UDragonMovementComponent::LandingSeverity(2000.0f, 20.0f, *Stats);
	TestTrue(TEXT("shallow flare is cheaper than head-on at the same speed"), Shallow < Steep);
	TestEqual(TEXT("grazing contact is free"),
			  UDragonMovementComponent::LandingSeverity(4000.0f, 5.0f, *Stats), 0.0f);

	// [2026-11-03 T-105] The pins: the params cache defaults to Stats'
	// threshold (what the pawn pushes at BeginPlay), and severity
	// follows a retuned Stats — proving no private copy survives. The
	// retune uses a separate instance so the T-064 cases below keep
	// proving default-curve behavior.
	TestEqual(TEXT("params cache defaults to the Stats threshold"),
			  Params.CachedLandingSpeedThreshold, Stats->LandingSpeedThreshold);
	UDragonStatsComponent* Retuned = NewObject<UDragonStatsComponent>();
	Retuned->LandingSpeedThreshold = 1000.0f;
	Retuned->LandingSpeedMaxDamage = 3000.0f;
	TestEqual(TEXT("severity follows a retuned Stats curve (no private copy)"),
			  UDragonMovementComponent::LandingSeverity(900.0f, 90.0f, *Retuned), 0.0f);
	TestEqual(TEXT("severity reaches full at the retuned max"),
			  UDragonMovementComponent::LandingSeverity(3000.0f, 90.0f, *Retuned), 1.0f);

	// [2026-11-03 T-079] Wall-impact rule: brushes are free and keep
	// all momentum, lawn-darts pay full and crumple. Damage is the
	// shipped landing number at the into-surface speed (pinned equal
	// to the Stats path, not re-derived), keep is one-minus-severity.
	{
		const UDragonMovementComponent::FWallImpactResult Brush =
			UDragonMovementComponent::ResolveWallImpact(700.0f, *Stats);
		TestEqual(TEXT("sub-threshold wall brush is free"), Brush.DamageHP, 0.0f);
		TestEqual(TEXT("sub-threshold wall brush keeps momentum"), Brush.KeepFraction, 1.0f);
	}
	{
		const UDragonMovementComponent::FWallImpactResult Mid =
			UDragonMovementComponent::ResolveWallImpact(1500.0f, *Stats);
		TestEqual(TEXT("wall damage equals the shipped landing number at that speed"),
				  Mid.DamageHP, Stats->LandingDamageForImpactSpeed(1500.0f));
		TestTrue(TEXT("mid wall impact keeps partial momentum"),
				 Mid.KeepFraction > 0.0f && Mid.KeepFraction < 1.0f);
		TestTrue(TEXT("mid wall impact hurts"), Mid.DamageHP > 5.0f);
	}
	{
		const UDragonMovementComponent::FWallImpactResult Slam =
			UDragonMovementComponent::ResolveWallImpact(2200.0f, *Stats);
		TestEqual(TEXT("max-curve wall impact pays full"), Slam.DamageHP, 25.0f);
		TestEqual(TEXT("max-curve wall impact crumples"), Slam.KeepFraction, 0.0f);
		const UDragonMovementComponent::FWallImpactResult Over =
			UDragonMovementComponent::ResolveWallImpact(5000.0f, *Stats);
		TestEqual(TEXT("past-max wall impact stays capped"), Over.DamageHP, 25.0f);
		TestEqual(TEXT("past-max wall impact stays crumpled"), Over.KeepFraction, 0.0f);
	}
	{
		// Retuned curve: the wall rule follows Stats (no private copy).
		UDragonStatsComponent* RetunedWall = NewObject<UDragonStatsComponent>();
		RetunedWall->LandingSpeedThreshold = 1000.0f;
		RetunedWall->LandingSpeedMaxDamage = 3000.0f;
		const UDragonMovementComponent::FWallImpactResult RetunedHit =
			UDragonMovementComponent::ResolveWallImpact(900.0f, *RetunedWall);
		TestEqual(TEXT("retuned wall curve frees 900 cm/s"), RetunedHit.DamageHP, 0.0f);
		TestEqual(TEXT("retuned wall curve keeps momentum at 900 cm/s"), RetunedHit.KeepFraction, 1.0f);
	}

	// [2026-11-03 T-064] Dive is a posture: committed nose-down past 30 deg
	// while above the precision band. Level, climbing, shallow, slow, and
	// boundary flight are never a dive — however steep the camera looks.
	TestTrue(TEXT("steep fast descent is a dive"),
			 UDragonMovementComponent::IsDivePosture(-45.0f, 2000.0f, Params));
	TestTrue(TEXT("vertical dive at terminal is a dive"),
			 UDragonMovementComponent::IsDivePosture(-90.0f, 4000.0f, Params));
	TestTrue(TEXT("just past the pitch line is a dive"),
			 UDragonMovementComponent::IsDivePosture(-31.0f, 2000.0f, Params));
	TestTrue(TEXT("just above the band is a dive"),
			 UDragonMovementComponent::IsDivePosture(-45.0f, 1601.0f, Params));
	TestFalse(TEXT("on the pitch line is not a dive"),
			  UDragonMovementComponent::IsDivePosture(-30.0f, 2000.0f, Params));
	TestFalse(TEXT("at the band edge is not a dive"),
			  UDragonMovementComponent::IsDivePosture(-45.0f, 1600.0f, Params));
	TestFalse(TEXT("shallow fast descent is not a dive"),
			  UDragonMovementComponent::IsDivePosture(-10.0f, 3000.0f, Params));
	TestFalse(TEXT("level cruise is not a dive"),
			  UDragonMovementComponent::IsDivePosture(0.0f, 2000.0f, Params));
	TestFalse(TEXT("climbing is not a dive"),
			  UDragonMovementComponent::IsDivePosture(30.0f, 3000.0f, Params));
	TestFalse(TEXT("slow steep looks are not a dive"),
			  UDragonMovementComponent::IsDivePosture(-45.0f, 400.0f, Params));

	// [2026-11-03 T-064] Speed+angle interplay at dive speeds: a terminal
	// head-on lawn-dart pays full, and a fast shallow dive flare stays
	// cheap — the flare is never priced like the lawn-dart.
	TestEqual(TEXT("dive-terminal head-on impact is full"),
			  UDragonMovementComponent::LandingSeverity(4000.0f, 90.0f, *Stats), 1.0f);
	const float DiveSteep = UDragonMovementComponent::LandingSeverity(4000.0f, 90.0f, *Stats);
	const float DiveShallow = UDragonMovementComponent::LandingSeverity(4000.0f, 20.0f, *Stats);
	TestTrue(TEXT("fast shallow dive flare is cheaper than head-on at dive speed"),
			 DiveShallow < DiveSteep);
	TestEqual(TEXT("grazing dive entry is free"),
			  UDragonMovementComponent::LandingSeverity(3200.0f, 10.0f, *Stats), 0.0f);

	// [2026-11-04 FLIGHT-PHYSICS] Causal bank: input commands bank to
	// the +-40 cap, yaw derives from it by the coordinated-turn law
	// (g*tan(bank)/v). Full bank at cruise earns ~29.5 deg/s, at dive
	// speed only ~11.8 deg/s (faster = wider), and at rest exactly
	// nothing — the old linear bank/35 x yaw-rate pins are replaced
	// by the physics, not weakened.
	TestEqual(TEXT("no turn input is wings-level"), UDragonMovementComponent::CommandedBankDegrees(0.0f), 0.0f);
	TestEqual(TEXT("full turn input banks to the cap"), UDragonMovementComponent::CommandedBankDegrees(1.0f), -40.0f);
	TestEqual(TEXT("full opposite input banks to the opposite cap"),
			  UDragonMovementComponent::CommandedBankDegrees(-1.0f), 40.0f);
	TestEqual(TEXT("half input banks half"), UDragonMovementComponent::CommandedBankDegrees(0.5f), -20.0f);
	TestEqual(TEXT("over-input clamps to the cap"), UDragonMovementComponent::CommandedBankDegrees(2.0f), -40.0f);
	TestEqual(TEXT("wings-level never yaws"),
			  UDragonMovementComponent::YawRateForBank(0.0f, 2400.0f, Params), 0.0f);
	TestTrue(TEXT("full bank at dive speed yaws at the coordinated rate (~11.8 deg/s)"),
			 FMath::Abs(UDragonMovementComponent::YawRateForBank(-40.0f, 4000.0f, Params) - 11.8f) < 0.2f);
	TestEqual(TEXT("full bank at rest yaws nothing (no spin in place)"),
			  UDragonMovementComponent::YawRateForBank(-40.0f, 0.0f, Params), 0.0f);
	TestTrue(TEXT("half bank at dive speed yaws at the half-bank coordinated rate (~5.1 deg/s)"),
			 FMath::Abs(UDragonMovementComponent::YawRateForBank(-20.0f, 4000.0f, Params) - 5.1f) < 0.2f);
	TestTrue(TEXT("opposite bank yaws opposite"),
			 FMath::Abs(UDragonMovementComponent::YawRateForBank(40.0f, 4000.0f, Params) + 11.8f) < 0.2f);
	// Bank -> yaw round-trips the input through the coordinated law.
	TestEqual(TEXT("bank-derived yaw round-trips full input through the coordinated law"),
			  UDragonMovementComponent::YawRateForBank(UDragonMovementComponent::CommandedBankDegrees(1.0f),
													   4000.0f, Params),
			  UDragonMovementComponent::CoordinatedYawRateDeg(-40.0f, 4000.0f, Params));

	// [2026-11-04 FLIGHT-VIGOR] RMB sailing step: demand consumes at
	// the rate budget per tick, never overshoots, never runs on
	// non-positive time or rate.
	TestEqual(TEXT("small steer demand applies in full"),
			  UDragonMovementComponent::SteerStepDegrees(0.5f, 90.0f, 1.0f), 0.5f);
	TestEqual(TEXT("large steer demand clamps to the tick budget"),
			  UDragonMovementComponent::SteerStepDegrees(90.0f, 90.0f, 0.5f), 45.0f);
	TestEqual(TEXT("negative steer demand clamps symmetrically"),
			  UDragonMovementComponent::SteerStepDegrees(-90.0f, 55.0f, 1.0f), -55.0f);
	TestEqual(TEXT("zero time steers nothing"), UDragonMovementComponent::SteerStepDegrees(10.0f, 90.0f, 0.0f),
			  0.0f);
	TestEqual(TEXT("zero rate steers nothing"), UDragonMovementComponent::SteerStepDegrees(10.0f, 0.0f, 1.0f),
			  0.0f);

	// [2026-11-04 FLIGHT-PHYSICS] Lift law: k = 980/1600^2, so lift
	// equals gravity exactly at cruise, is far under it at 600 (the
	// wing cannot hold slow flight), and clamps at dive speeds.
	TestEqual(TEXT("lift at cruise equals gravity"), UDragonMovementComponent::LiftAccel(1600.0f), 980.0f);
	TestEqual(TEXT("lift at half cruise is quarter gravity"), UDragonMovementComponent::LiftAccel(800.0f), 245.0f);
	TestTrue(TEXT("lift at 600 is far under gravity (stall)"),
			 UDragonMovementComponent::LiftAccel(600.0f) > 100.0f && UDragonMovementComponent::LiftAccel(600.0f) < 200.0f);
	TestEqual(TEXT("lift at rest is zero"), UDragonMovementComponent::LiftAccel(0.0f), 0.0f);
	TestEqual(TEXT("lift clamps at dive speed"), UDragonMovementComponent::LiftAccel(4000.0f), 2450.0f);
	TestTrue(TEXT("banked lift keeps only cos(bank) against gravity"),
			 FMath::Abs(UDragonMovementComponent::LiftAccel(1600.0f, -30.0f, Params) - 848.7f) < 0.5f);
	TestEqual(TEXT("level cruise lift nets zero vertical accel"),
			  UDragonMovementComponent::LiftVerticalAccel(1600.0f, 0.0f, Params), 0.0f);
	TestTrue(TEXT("slow flight lift nets a sink accel"),
			 UDragonMovementComponent::LiftVerticalAccel(500.0f, 0.0f, Params) < -800.0f);
	TestTrue(TEXT("fast flight lift nets a climb accel the pilot trims with pitch"),
			 UDragonMovementComponent::LiftVerticalAccel(2400.0f, 0.0f, Params) > 1000.0f);

	// [2026-11-04 FLIGHT-PHYSICS] Turn authority: zero at/below 200,
	// near zero at 300, full at/above 600, monotone between.
	TestEqual(TEXT("turn authority is zero at rest"), UDragonMovementComponent::TurnAuthority(0.0f), 0.0f);
	TestEqual(TEXT("turn authority is zero at 200 cm/s"), UDragonMovementComponent::TurnAuthority(200.0f), 0.0f);
	TestTrue(TEXT("turn authority at 300 cm/s is near zero (<0.05)"),
			 UDragonMovementComponent::TurnAuthority(300.0f) < 0.05f);
	TestEqual(TEXT("turn authority is full at 600 cm/s"), UDragonMovementComponent::TurnAuthority(600.0f), 1.0f);
	TestEqual(TEXT("turn authority stays full at cruise"), UDragonMovementComponent::TurnAuthority(1600.0f), 1.0f);
	{
		float PrevA = UDragonMovementComponent::TurnAuthority(200.0f);
		for (float V = 250.0f; V <= 600.0f; V += 50.0f)
		{
			const float A = UDragonMovementComponent::TurnAuthority(V);
			TestTrue(TEXT("turn authority never falls as speed rises"), A >= PrevA);
			PrevA = A;
		}
	}

	// [2026-11-04 FLIGHT-PHYSICS] Coordinated turn at cruise: a 30 deg
	// bank at 1600 cm/s earns g*tan(30)/1600 = 20.26 deg/s. Honest
	// geometry: radius = v/omega = 4525 cm, diameter ~9049 cm (the
	// 500-2500 guess does not survive the formula; this pins the
	// computed range). Faster flight turns wider, never tighter.
	{
		const float YawDeg = UDragonMovementComponent::CoordinatedYawRateDeg(-30.0f, 1600.0f, Params);
		TestTrue(TEXT("30 deg bank at 1600 yaws ~20.3 deg/s"), YawDeg > 20.0f && YawDeg < 20.6f);
		const float Diameter = 2.0f * 1600.0f / FMath::DegreesToRadians(YawDeg);
		TestTrue(FString::Printf(TEXT("30 deg bank at 1600 turn diameter is %.0f cm"), Diameter),
				 Diameter > 8000.0f && Diameter < 10000.0f);
		TestTrue(TEXT("full bank at 600 stays under the feel cap"),
				 UDragonMovementComponent::CoordinatedYawRateDeg(-40.0f, 600.0f, Params) <=
					 UDragonMovementComponent::YawRateForAirspeed(600.0f, Params));
		TestEqual(TEXT("coordinated yaw at 200 cm/s is exactly zero"),
				  UDragonMovementComponent::CoordinatedYawRateDeg(-40.0f, 200.0f, Params), 0.0f);
		TestTrue(TEXT("coordinated yaw at 300 cm/s is near zero (<10 deg/s)"),
				 UDragonMovementComponent::CoordinatedYawRateDeg(-40.0f, 300.0f, Params) < 10.0f);
		TestTrue(TEXT("dive-speed turn is wider than cruise turn at the same bank"),
				 UDragonMovementComponent::CoordinatedYawRateDeg(-30.0f, 4000.0f, Params) < YawDeg);
	}

	// [2026-11-04 FLIGHT-PHYSICS] Full-step sims: the exact momentum
	// core PhysCustom integrates (energy step -> steer lag onto the
	// nose -> lift-vs-gravity Z, momentum weight 1), at 60 Hz.
	{
		auto MomentumStep = [&Params](FVector Vel, const FVector& NoseDir, float BankDeg, float Dt) -> FVector
		{
			const float Speed = UDragonMovementComponent::IntegrateEnergyStep(
				Vel.Size(), FMath::Asin(FMath::Clamp(NoseDir.Z, -1.0f, 1.0f)), Dt, Params);
			const float Steer = UDragonMovementComponent::ComputeSteerResponsiveness(Vel.Size(), 0.0f, Params);
			FVector Out = Vel + (NoseDir * Speed - Vel) * FMath::Min(1.0f, Dt * Steer);
			Out.Z += UDragonMovementComponent::LiftVerticalAccel(Speed, BankDeg, Params) * Dt;
			return Out;
		};
		const float Dt = 1.0f / 60.0f;
		// Level cruise: 10 s, altitude and speed hold without thrust.
		{
			FVector Vel(1600.0f, 0.0f, 0.0f);
			FVector Pos = FVector::ZeroVector;
			for (int32 Step = 0; Step < 600; ++Step)
			{
				Vel = MomentumStep(Vel, FVector(1.0f, 0.0f, 0.0f), 0.0f, Dt);
				Pos += Vel * Dt;
			}
			TestTrue(FString::Printf(TEXT("level cruise holds altitude over 10 s (dz=%.1f cm)"), Pos.Z),
					 FMath::Abs(Pos.Z) < 50.0f);
			TestTrue(TEXT("level cruise holds speed over 10 s"), FMath::Abs(Vel.Size() - 1600.0f) < 50.0f);
		}
		// Slow momentum flight: the wing cannot hold it, it sinks.
		{
			FVector Vel(500.0f, 0.0f, 0.0f);
			FVector Pos = FVector::ZeroVector;
			for (int32 Step = 0; Step < 600; ++Step)
			{
				Vel = MomentumStep(Vel, FVector(1.0f, 0.0f, 0.0f), 0.0f, Dt);
				Pos += Vel * Dt;
			}
			TestTrue(FString::Printf(TEXT("slow flight sinks in momentum (dz=%.1f cm over 10 s)"), Pos.Z),
					 Pos.Z < -200.0f && Vel.Z < -50.0f);
		}
		// Dive still gains speed through the lift model (energy owns
		// speed along the nose; lift only bends the vertical).
		{
			const FVector DiveNose = FVector(0.866f, 0.0f, -0.5f).GetSafeNormal();
			FVector Vel = DiveNose * 1500.0f;
			FVector Pos = FVector::ZeroVector;
			for (int32 Step = 0; Step < 120; ++Step)
			{
				Vel = MomentumStep(Vel, DiveNose, 0.0f, Dt);
				Pos += Vel * Dt;
			}
			TestTrue(FString::Printf(TEXT("dive gains speed in the full-step sim (%.0f cm/s)"), Vel.Size()),
					 Vel.Size() > 1500.0f && Pos.Z < 0.0f);
		}
		// Banked turn, flown: heading follows the coordinated yaw for
		// one full turn at cruise; the flown diameter matches the
		// analytic ~9049 cm range above.
		{
			const float YawRateRad =
				FMath::DegreesToRadians(UDragonMovementComponent::CoordinatedYawRateDeg(-30.0f, 1600.0f, Params));
			FVector Pos = FVector::ZeroVector;
			FVector MinP = Pos;
			FVector MaxP = Pos;
			float Heading = 0.0f;
			const int32 Steps = FMath::CeilToInt(2.0f * PI / FMath::Max(YawRateRad, 0.001f) / Dt);
			for (int32 Step = 0; Step < Steps; ++Step)
			{
				Heading += YawRateRad * Dt;
				Pos += FVector(FMath::Cos(Heading), FMath::Sin(Heading), 0.0f) * 1600.0f * Dt;
				MinP = FVector(FMath::Min(MinP.X, Pos.X), FMath::Min(MinP.Y, Pos.Y), 0.0f);
				MaxP = FVector(FMath::Max(MaxP.X, Pos.X), FMath::Max(MaxP.Y, Pos.Y), 0.0f);
			}
			const float FlownDiameter = FMath::Max(MaxP.X - MinP.X, MaxP.Y - MinP.Y);
			TestTrue(FString::Printf(TEXT("flown 30 deg bank turn diameter is %.0f cm"), FlownDiameter),
					 FlownDiameter > 8000.0f && FlownDiameter < 10000.0f);
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
