// Copyright Under the Mountain contributors.
// Purpose: T-068 animation-decision + new-verb contract tests — flap-rate
// curves, fold/tuck alphas, bank/pitch clamps, wing-buffet daze and tail-slam
// rear-arc damage, all headless against the pure seams (UDragonAnimInstance
// statics, UDragonCombatComponent, UThiefBrain). Created 2026-11-01 (T-068).

#include "Misc/AutomationTest.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dragon/DragonAnimInstance.h"
#include "Dragon/DragonCharacter.h"
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

	// [2026-11-04 T-125] State order is serialized (logs name states by int;
	// ABP_Dragon transitions test the published LocomotionState): the T-043
	// values hold and Hover/Dive stay appended.
	TestEqual(TEXT("Idle is state 0"), static_cast<int32>(EDragonAnimState::Idle), 0);
	TestEqual(TEXT("Walk is state 1"), static_cast<int32>(EDragonAnimState::Walk), 1);
	TestEqual(TEXT("Fly is state 2"), static_cast<int32>(EDragonAnimState::Fly), 2);
	TestEqual(TEXT("Glide is state 3"), static_cast<int32>(EDragonAnimState::Glide), 3);
	TestEqual(TEXT("Hover is state 4"), static_cast<int32>(EDragonAnimState::Hover), 4);
	TestEqual(TEXT("Dive is state 5"), static_cast<int32>(EDragonAnimState::Dive), 5);

	// [2026-11-04 feel/spam] Hysteresis/dwell: held Glide exits only past
	// [2026-11-04 feel/spam] VS -50, held Hover outside 350/350, airborne
	// [2026-11-04 feel/spam] changes wait out the 0.3 s dwell, and Dive /
	// [2026-11-04 feel/spam] ground switches are never dwell-blocked.
	using Anim = UDragonAnimInstance;
	TestTrue(TEXT("held glide survives the -250..-50 band"),
			 Anim::SelectLocomotionState(EDragonAnimState::Glide, true, false, 900.0f, -100.0f, 1.0f) ==
				 EDragonAnimState::Glide);
	TestTrue(TEXT("held glide exits above -50"),
			 Anim::SelectLocomotionState(EDragonAnimState::Glide, true, false, 900.0f, 0.0f, 1.0f) ==
				 EDragonAnimState::Fly);
	TestTrue(TEXT("held hover survives to the 350 exit band"),
			 Anim::SelectLocomotionState(EDragonAnimState::Hover, true, false, 300.0f, 0.0f, 1.0f) ==
				 EDragonAnimState::Hover);
	TestTrue(TEXT("held hover exits past 350"),
			 Anim::SelectLocomotionState(EDragonAnimState::Hover, true, false, 400.0f, 0.0f, 1.0f) ==
				 EDragonAnimState::Fly);
	TestTrue(TEXT("dwell holds Fly against a fresh Hover candidate"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, true, false, 10.0f, 0.0f, 0.1f) ==
				 EDragonAnimState::Fly);
	TestTrue(TEXT("dwell elapsed lets Hover in"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, true, false, 10.0f, 0.0f, 0.3f) ==
				 EDragonAnimState::Hover);
	TestTrue(TEXT("dive is never dwell-blocked"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, true, true, 900.0f, -500.0f, 0.0f) ==
				 EDragonAnimState::Dive);
	TestTrue(TEXT("landing is never dwell-blocked"),
			 Anim::SelectLocomotionState(EDragonAnimState::Fly, false, false, 0.0f, 0.0f, 0.0f) ==
				 EDragonAnimState::Idle);

	// [2026-11-04 feel/spam] Walk rate + combined slither bone rotation.
	TestEqual(TEXT("walk rate floors at 0.5"), Anim::ComputeWalkPlayRate(0.0f), 0.5f);
	TestEqual(TEXT("walk rate is 1.0 at 300 cm/s"), Anim::ComputeWalkPlayRate(300.0f), 1.0f);
	TestEqual(TEXT("walk rate caps at 1.5"), Anim::ComputeWalkPlayRate(900.0f), 1.5f);
	TestTrue(TEXT("slither bone segment 0 is neutral at phase 0"),
			 FMath::Abs(Anim::ComputeSlitherBoneRotation(0, 0.0f, 6.0f, 0.0f, 0.0f).Yaw) < 0.01f);
	TestTrue(TEXT("slither bone yaw lags with segment index"),
			 Anim::ComputeSlitherBoneRotation(3, 0.0f, 6.0f, 0.0f, 0.0f).Yaw < 0.0f);
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

// [2026-11-04 T-125] Dual-mode contract: the pawn runs ABP_Dragon iff the
// graph asset is on disk (the constructor's class finder decides), else
// the shipped single-node driver. The expectation reads the same world
// the pawn saw (FindObject AFTER construction, so a landed graph that
// the constructor just loaded is visible) — future-proof across T-139,
// no hardcoded mode. Slot names pin the C++ side of the build-sheet
// contract (the graph's Slot nodes must carry these exact names).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDragonABPFallbackTest, "UnderTheMountain.Dragon.ABPFallback",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDragonABPFallbackTest::RunTest(const FString& Parameters)
{
	ADragonCharacter* Pawn = NewObject<ADragonCharacter>();
	TestNotNull(TEXT("pawn constructs for ABP check"), Pawn);
	if (Pawn == nullptr)
	{
		return false;
	}
	const UClass* AbpClass =
		FindObject<UClass>(nullptr, TEXT("/Game/Meshy/Dragon/ABP_Dragon.ABP_Dragon_C"));
	const bool bExpectABP = (AbpClass != nullptr);
	TestTrue(TEXT("ABP flag follows graph presence"), Pawn->IsUsingABP() == bExpectABP);
	const EAnimationMode::Type ExpectedMode = bExpectABP ? EAnimationMode::AnimationBlueprint
														 : EAnimationMode::AnimationSingleNode;
	TestTrue(TEXT("mesh mode follows ABP flag"),
			 Pawn->GetMesh()->GetAnimationMode() == ExpectedMode);
	TestTrue(TEXT("full-body slot name"), ADragonCharacter::AnimSlotFullBody == FName(TEXT("FullBody")));
	TestTrue(TEXT("upper-body slot name"),
			 ADragonCharacter::AnimSlotUpperBody == FName(TEXT("UpperBody")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
