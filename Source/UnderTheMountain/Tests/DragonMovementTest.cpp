// Copyright Under the Mountain contributors.
// Purpose: T-075 prototype flight-math contract tests — scheme defaults,
// authority blend, energy integration, yaw/brake/severity curves, exercised
// directly against UDragonMovementComponent pure statics with no world
// spawned. Created 2026-11-03 (T-075). Pawn integration (PhysCustom,
// CSV course runs) is PIE/PT-STAM territory; the math it delegates to is
// proven here.

#include "Misc/AutomationTest.h"
#include "Dragon/DragonMovementComponent.h"

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
	TestEqual(TEXT("stall sink default is 600 cm/s"), Params.StallSinkRate, 600.0f);
	TestEqual(TEXT("precision thrust default is 650 cm/s"), Params.PrecisionThrustSpeed, 650.0f);
	TestEqual(TEXT("impulse window default is 1.5 s"), Params.ImpulseAuthorityWindow, 1.5f);

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

	// [2026-11-03 T-075] Severity prices the normal component: below the
	// 800 curve it is free, at 2200 head-on it is full, and the same speed
	// at a shallow angle is cheaper than steep (flare != lawn-dart).
	TestEqual(TEXT("sub-threshold impact is free"),
			  UDragonMovementComponent::LandingSeverity(700.0f, 90.0f, Params), 0.0f);
	TestEqual(TEXT("max-curve head-on impact is full"),
			  UDragonMovementComponent::LandingSeverity(2200.0f, 90.0f, Params), 1.0f);
	const float Steep = UDragonMovementComponent::LandingSeverity(2000.0f, 90.0f, Params);
	const float Shallow = UDragonMovementComponent::LandingSeverity(2000.0f, 20.0f, Params);
	TestTrue(TEXT("shallow flare is cheaper than head-on at the same speed"), Shallow < Steep);
	TestEqual(TEXT("grazing contact is free"),
			  UDragonMovementComponent::LandingSeverity(4000.0f, 5.0f, Params), 0.0f);

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
			  UDragonMovementComponent::LandingSeverity(4000.0f, 90.0f, Params), 1.0f);
	const float DiveSteep = UDragonMovementComponent::LandingSeverity(4000.0f, 90.0f, Params);
	const float DiveShallow = UDragonMovementComponent::LandingSeverity(4000.0f, 20.0f, Params);
	TestTrue(TEXT("fast shallow dive flare is cheaper than head-on at dive speed"),
			 DiveShallow < DiveSteep);
	TestEqual(TEXT("grazing dive entry is free"),
			  UDragonMovementComponent::LandingSeverity(3200.0f, 10.0f, Params), 0.0f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
