// Copyright Under the Mountain contributors.
// Purpose: T-053 villager rule tests - threat state, flee heading
// and per-state speed, exercised directly against UVillagerBrain's
// pure statics with no world spawned (the actor is a thin steering
// shell over these rules).

#include "Misc/AutomationTest.h"
#include "NPC/VillagerBrain.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-01 T-053] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillagerBrainTest, "UnderTheMountain.NPC.Villager",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillagerBrainTest::RunTest(const FString& Parameters)
{
	UVillagerBrain* Brain = NewObject<UVillagerBrain>();
	TestEqual(TEXT("default flee radius is 1500"), Brain->FleeRadius, 1500.0f);

	// [2026-11-01 T-053] Threat rule: inside (and exactly at) the
	// radius flees; outside returns to daily life; a negative
	// (broken) distance reads as point-blank, never as safe.
	TestTrue(TEXT("dragon at the well flees"),
			 UVillagerBrain::StateForThreat(100.0f, Brain->FleeRadius) == EVillagerState::Flee);
	TestTrue(TEXT("dragon exactly at radius flees"),
			 UVillagerBrain::StateForThreat(1500.0f, Brain->FleeRadius) == EVillagerState::Flee);
	TestTrue(TEXT("dragon beyond radius is daily life"),
			 UVillagerBrain::StateForThreat(1501.0f, Brain->FleeRadius) != EVillagerState::Flee);
	TestTrue(TEXT("negative distance flees"),
			 UVillagerBrain::StateForThreat(-5.0f, Brain->FleeRadius) == EVillagerState::Flee);

	// [2026-11-01 T-053] Flee heading: directly away, flat, and a
	// fixed answer when the dragon is on top of the villager.
	const FVector Away = UVillagerBrain::FleeDirection(FVector(100.0f, 0.0f, 0.0f), FVector::ZeroVector);
	TestTrue(TEXT("flee runs +X away from a dragon at the origin"),
			 Away.Equals(FVector(1.0f, 0.0f, 0.0f), 0.001f));
	const FVector AwayUp = UVillagerBrain::FleeDirection(FVector(0.0f, 100.0f, 50.0f), FVector(0.0f, 0.0f, 900.0f));
	TestTrue(TEXT("flee ignores dragon altitude"), AwayUp.Equals(FVector(0.0f, 1.0f, 0.0f), 0.001f));
	const FVector Coincident = UVillagerBrain::FleeDirection(FVector::ZeroVector, FVector::ZeroVector);
	TestTrue(TEXT("coincident flee has a fixed heading"),
			 Coincident.Equals(FVector(1.0f, 0.0f, 0.0f), 0.001f));

	// [2026-11-01 T-053] Speeds: idle stands, wander strolls, flee
	// runs; negative tuning clamps to standing, never backwards.
	TestEqual(TEXT("idle speed is zero"),
			  UVillagerBrain::SpeedForState(EVillagerState::Idle, 90.0f, 340.0f), 0.0f);
	TestEqual(TEXT("wander speed is walk"),
			  UVillagerBrain::SpeedForState(EVillagerState::Wander, 90.0f, 340.0f), 90.0f);
	TestEqual(TEXT("flee speed is run"),
			  UVillagerBrain::SpeedForState(EVillagerState::Flee, 90.0f, 340.0f), 340.0f);
	TestEqual(TEXT("negative walk tuning clamps to zero"),
			  UVillagerBrain::SpeedForState(EVillagerState::Wander, -90.0f, 340.0f), 0.0f);
	TestEqual(TEXT("negative flee tuning clamps to zero"),
			  UVillagerBrain::SpeedForState(EVillagerState::Flee, 90.0f, -340.0f), 0.0f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
