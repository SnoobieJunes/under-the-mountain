// Copyright Under the Mountain contributors.
// Purpose: T-068 animation-decision + new-verb contract tests — flap-rate
// curves, fold/tuck alphas, bank/pitch clamps, wing-buffet daze and tail-slam
// rear-arc damage, all headless against the pure seams (UDragonAnimInstance
// statics, UDragonCombatComponent, UThiefBrain). Created 2026-11-01 (T-068).

#include "Misc/AutomationTest.h"
#include "Dragon/DragonAnimInstance.h"
#include "Dragon/DragonCombatComponent.h"
#include "Dragon/DragonStatsComponent.h"
#include "Thief/ThiefBrain.h"
#include "Thief/ThiefDataAsset.h"
#include "Treasure/TreasureRegistry.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonAnimCurvesTest, "UnderTheMountain.Dragon.AnimCurves",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonAnimCurvesTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-068] Wingbeat curves: climb > cruise > sink, hover deep,
	// [2026-11-01 T-068] dive near-held, everything inside the hard clamp.
	const float Climb = UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 900.0f, 600.0f);
	const float Cruise = UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 900.0f, 0.0f);
	const float Sink = UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 900.0f, -600.0f);
	TestTrue(TEXT("climbing flaps faster than cruise"), Climb > Cruise);
	TestTrue(TEXT("cruise flaps faster than sinking"), Cruise > Sink);
	TestTrue(TEXT("hover beats faster than cruise"),
			 UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Hover, 0.0f, 0.0f) > Cruise);
	TestTrue(TEXT("dive is near-held, not fast-flapped"),
			 UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Dive, 3000.0f, -2000.0f) < 0.3f);
	TestTrue(TEXT("extreme climb stays clamped"),
			 UDragonAnimInstance::ComputeFlapPlayRate(EDragonAnimState::Fly, 0.0f, 100000.0f) <= 1.8f);

	// [2026-11-01 T-068] Fold/tuck contract the baked clips satisfy.
	TestEqual(TEXT("ground wings folded"), UDragonAnimInstance::ComputeFoldAlpha(EDragonAnimState::Idle), 1.0f);
	TestEqual(TEXT("air wings spread"), UDragonAnimInstance::ComputeFoldAlpha(EDragonAnimState::Fly), 0.0f);
	TestEqual(TEXT("flight legs tucked"), UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState::Fly), 1.0f);
	TestEqual(TEXT("hover legs half-ready"), UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState::Hover), 0.5f);
	TestEqual(TEXT("ground legs deployed"), UDragonAnimInstance::ComputeTuckAlpha(EDragonAnimState::Walk), 0.0f);

	// [2026-11-01 T-068] Bank/pitch: zero without airspeed/turn, capped always.
	TestEqual(TEXT("no bank at rest"), UDragonAnimInstance::ComputeBankDegrees(90.0f, 0.0f), 0.0f);
	TestTrue(TEXT("hard turn banks but caps at 35"),
			 FMath::Abs(UDragonAnimInstance::ComputeBankDegrees(720.0f, 3000.0f)) <= 35.0f);
	TestEqual(TEXT("level flight no pitch"), UDragonAnimInstance::ComputeMeshPitchDegrees(1000.0f, 0.0f), 0.0f);
	TestTrue(TEXT("steep dive pitch caps at 30"),
			 UDragonAnimInstance::ComputeMeshPitchDegrees(100.0f, -5000.0f) >= -30.0f);
	TestTrue(TEXT("descending pitches nose-down"),
			 UDragonAnimInstance::ComputeMeshPitchDegrees(1000.0f, -500.0f) < 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonBuffetSlamTest, "UnderTheMountain.Dragon.BuffetSlam",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonBuffetSlamTest::RunTest(const FString& Parameters)
{
	// [2026-11-01 T-068] Brain fixtures mirror DragonCombatTest's burglar
	// [2026-11-01 T-068] (20 HP) against a one-treasure registry.
	UTreasureRegistry* Registry = NewObject<UTreasureRegistry>();
	FTreasureEntry Crown;
	Crown.Id = FName(TEXT("Crown"));
	Crown.GoldValue = 1200;
	Crown.Location = FVector(100.0f, 0.0f, 0.0f);
	Registry->Register(Crown);

	auto MakeBrain = [Registry](const TCHAR* Name)
	{
		UThiefDataAsset* Config = NewObject<UThiefDataAsset>();
		Config->MaxHP = 20.0f;
		Config->GrabTimeSeconds = 1.5f;
		Config->CarriedCap = 1;
		Config->bDropsAllLootOnRoar = true;
		Config->bCanPlaceTraps = false;
		Config->MaxTraps = 3;
		UThiefBrain* Brain = NewObject<UThiefBrain>();
		Brain->Init(Config, Registry, FName(Name));
		Brain->SetExitLocation(FVector(-9000.0f, 0.0f, 0.0f));
		return Brain;
	};

	UDragonStatsComponent* Stats = NewObject<UDragonStatsComponent>();
	Stats->Stamina = 100.0f;
	UDragonCombatComponent* Combat = NewObject<UDragonCombatComponent>();
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward(1.0f, 0.0f, 0.0f);

	// [2026-11-01 T-068] Buffet: frontal thief dazed, rear thief untouched,
	// [2026-11-01 T-068] cooldown blocks an instant repeat for free.
	UThiefBrain* FrontBrain = MakeBrain(TEXT("FrontBurglar"));
	UThiefBrain* RearBrain = MakeBrain(TEXT("RearBurglar"));
	const TArray<UThiefBrain*> Brains = {FrontBrain, RearBrain};
	const TArray<FVector> Locations = {FVector(300.0f, 0.0f, 0.0f), FVector(-300.0f, 0.0f, 0.0f)};
	int32 NumDazed = -1;
	TestTrue(TEXT("buffet succeeds"),
			 Combat->TryWingBuffet(100.0f, Stats, Origin, Forward, Brains, Locations, NumDazed));
	TestEqual(TEXT("exactly the frontal thief is dazed"), NumDazed, 1);
	TestTrue(TEXT("frontal thief is Dazed"), FrontBrain->State() == EThiefState::Dazed);
	TestFalse(TEXT("rear thief is not dazed"), RearBrain->State() == EThiefState::Dazed);
	TestEqual(TEXT("buffet costs 8 stamina"), Stats->Stamina, 92.0f);
	int32 NumAgain = 0;
	TestFalse(TEXT("buffet on cooldown is blocked"),
			  Combat->TryWingBuffet(101.0f, Stats, Origin, Forward, Brains, Locations, NumAgain));
	TestEqual(TEXT("blocked buffet spends nothing"), Stats->Stamina, 92.0f);

	// [2026-11-01 T-068] Daze expiry: stands still, then flees.
	FrontBrain->Tick(1.0f, Locations[0]);
	TestTrue(TEXT("still dazed after 1s"), FrontBrain->State() == EThiefState::Dazed);
	FrontBrain->Tick(2.0f, Locations[0]);
	TestTrue(TEXT("daze fades into Flee"), FrontBrain->State() == EThiefState::Flee);

	// [2026-11-01 T-068] Tail slam: rear thief takes the GAMEPLAY 20 (lethal
	// [2026-11-01 T-068] to a 20 HP burglar), frontal thief is safe.
	int32 NumHit = -1;
	TestTrue(TEXT("tail slam succeeds"),
			 Combat->TryTailSlam(110.0f, Stats, Origin, Forward, Brains, Locations, NumHit));
	TestEqual(TEXT("exactly the rear thief is hit"), NumHit, 1);
	TestTrue(TEXT("rear thief died to 20 damage"), RearBrain->State() == EThiefState::Dead);
	TestEqual(TEXT("slam costs 4 stamina"), Stats->Stamina, 88.0f);

	// [2026-11-01 T-068] Arc edges: out of range never counts, point-blank does.
	TestFalse(TEXT("beyond buffet radius is outside"),
			  UDragonCombatComponent::IsInArc2D(Origin, Forward, FVector(701.0f, 0.0f, 0.0f), 700.0f, 70.0f));
	TestTrue(TEXT("point-blank is inside"),
			 UDragonCombatComponent::IsInArc2D(Origin, Forward, Origin, 700.0f, 70.0f));
	TestFalse(TEXT("directly behind is outside the frontal arc"),
			  UDragonCombatComponent::IsInArc2D(Origin, Forward, FVector(-300.0f, 0.0f, 0.0f), 700.0f, 70.0f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
