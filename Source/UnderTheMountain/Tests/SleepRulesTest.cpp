// Copyright Under the Mountain contributors.
// Purpose: T-084 sleep-rule tests (AT-Sleep) — trigger refusals,
// S9 star decay through the notoriety single writer's statics,
// per-stack bulk risk, the deep-plunder roll, and wake vitals,
// all against pure code with injected randomness and no world.

#include "Misc/AutomationTest.h"
#include "Notoriety/NotorietySubsystem.h"
#include "World/SleepRules.h"
#include "World/WorldClock.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-04 T-084] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSleepRulesTest, "UnderTheMountain.World.Sleep",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSleepRulesTest::RunTest(const FString& Parameters)
{
	// --- Trigger gate (S9: hoard mound, between waves, no live thief) ---
	TestTrue(TEXT("all conditions met allows sleep"),
			 FSleepRules::CanSleep(true, false, false) == ESleepRefusal::None);
	TestTrue(TEXT("away from the mound refuses"),
			 FSleepRules::CanSleep(false, false, false) == ESleepRefusal::NotAtHoardMound);
	TestTrue(TEXT("a wave in progress refuses"),
			 FSleepRules::CanSleep(true, true, false) == ESleepRefusal::WaveInProgress);
	TestTrue(TEXT("a live thief in the lair refuses"),
			 FSleepRules::CanSleep(true, false, true) == ESleepRefusal::ThiefInLair);
	TestTrue(TEXT("place is checked before wave state"),
			 FSleepRules::CanSleep(false, true, true) == ESleepRefusal::NotAtHoardMound);

	// [2026-11-04 T-084] Pawn glue seam: near means within radius in
	// 2D (perched on top of the pile still counts), never beyond it.
	TestTrue(TEXT("at the mound can sleep there"),
			 FSleepRules::IsNearHoardMound(FVector(0, 0, 900), FVector::ZeroVector, FSleepRules::HoardMoundSleepRadiusCm));
	TestTrue(TEXT("at the radius edge can sleep there"),
			 FSleepRules::IsNearHoardMound(FVector(FSleepRules::HoardMoundSleepRadiusCm, 0, 0), FVector::ZeroVector, FSleepRules::HoardMoundSleepRadiusCm));
	TestFalse(TEXT("beyond the mound refuses"),
			  FSleepRules::IsNearHoardMound(FVector(2000, 0, 0), FVector::ZeroVector, FSleepRules::HoardMoundSleepRadiusCm));
	TestFalse(TEXT("a non-positive radius never sleeps"),
			  FSleepRules::IsNearHoardMound(FVector::ZeroVector, FVector::ZeroVector, 0.0f));

	// --- Star decay (S9: -2 stars, points reset to the band floor) ---
	// From each starting star, checked at band floors and interiors.
	TestEqual(TEXT("0 stars stays 0 points"), UNotorietySubsystem::PointsAfterSleepDecay(0), 0);
	TestEqual(TEXT("1 star (10) decays to 0 points"), UNotorietySubsystem::PointsAfterSleepDecay(10),
			  0);
	TestEqual(TEXT("1-star interior (29) decays to 0 points"),
			  UNotorietySubsystem::PointsAfterSleepDecay(29), 0);
	TestEqual(TEXT("2 stars (30) decays to 0 points"), UNotorietySubsystem::PointsAfterSleepDecay(30),
			  0);
	TestEqual(TEXT("3 stars (60) decays to the 1-star floor (10)"),
			  UNotorietySubsystem::PointsAfterSleepDecay(60), 10);
	TestEqual(TEXT("3-star interior (99) also lands on 10, partial progress forgotten"),
			  UNotorietySubsystem::PointsAfterSleepDecay(99), 10);
	TestEqual(TEXT("4 stars (100) decays to the 2-star floor (30)"),
			  UNotorietySubsystem::PointsAfterSleepDecay(100), 30);
	TestEqual(TEXT("5 stars (150) decays to the 3-star floor (60)"),
			  UNotorietySubsystem::PointsAfterSleepDecay(150), 60);
	TestEqual(TEXT("above-cap points still decay by stars, not points"),
			  UNotorietySubsystem::PointsAfterSleepDecay(9999), 60);
	TestEqual(TEXT("negative points decay to 0"), UNotorietySubsystem::PointsAfterSleepDecay(-50), 0);

	// Band floors are the inverse of StarsForPoints.
	TestEqual(TEXT("floor of 0 stars is 0"), UNotorietySubsystem::PointsFloorForStars(0), 0);
	TestEqual(TEXT("floor of 3 stars is 60"), UNotorietySubsystem::PointsFloorForStars(3), 60);
	TestEqual(TEXT("floor of 5 stars is 150"), UNotorietySubsystem::PointsFloorForStars(5), 150);
	TestEqual(TEXT("out-of-range stars clamp high"), UNotorietySubsystem::PointsFloorForStars(99), 150);
	TestEqual(TEXT("out-of-range stars clamp low"), UNotorietySubsystem::PointsFloorForStars(-1), 0);

	// The single-writer instance applies the same math to itself.
	UNotorietySubsystem* Notoriety = NewObject<UNotorietySubsystem>();
	Notoriety->AddPoints(100, TEXT("test setup"));
	Notoriety->ApplySleepDecay();
	TestEqual(TEXT("ApplySleepDecay moves 100 -> 30"), Notoriety->GetPoints(), 30);
	TestEqual(TEXT("ApplySleepDecay reads 2 stars after"), Notoriety->GetStars(), 2);
	Notoriety->ApplySleepDecay();
	TestEqual(TEXT("repeat sleep decays again to 0"), Notoriety->GetPoints(), 0);

	// --- Hoard risk: 15% of each bulk stack, bulk before artifacts ---
	TestEqual(TEXT("15% of a 1000 stack is 150"), FSleepRules::BulkGoldLostForStack(1000), (int64)150);
	TestEqual(TEXT("15% of a 100 stack is 15"), FSleepRules::BulkGoldLostForStack(100), (int64)15);
	TestEqual(TEXT("15% of a 10 stack rounds to 2"), FSleepRules::BulkGoldLostForStack(10), (int64)2);
	TestEqual(TEXT("an empty stack loses nothing"), FSleepRules::BulkGoldLostForStack(0), (int64)0);
	TestEqual(TEXT("a negative stack loses nothing"), FSleepRules::BulkGoldLostForStack(-100), (int64)0);

	// --- Deep plunder: 5% roll, unequipped only, equipped never ---
	TestTrue(TEXT("a 0.0 roll takes an unequipped artifact"),
			 FSleepRules::DeepPlunderTakesArtifact(0.0f, true));
	TestTrue(TEXT("a 0.049 roll takes an unequipped artifact"),
			 FSleepRules::DeepPlunderTakesArtifact(0.049f, true));
	TestFalse(TEXT("exactly 0.05 does not take"),
			  FSleepRules::DeepPlunderTakesArtifact(0.05f, true));
	TestFalse(TEXT("a 0.99 roll does not take"), FSleepRules::DeepPlunderTakesArtifact(0.99f, true));
	TestFalse(TEXT("no unequipped artifact means nothing to take"),
			  FSleepRules::DeepPlunderTakesArtifact(0.0f, false));
	TestFalse(TEXT("an out-of-range roll clamps instead of widening the chance"),
			  FSleepRules::DeepPlunderTakesArtifact(5.0f, true));

	// --- Wake vitals (S9: stamina full, HP +50% of missing) ---
	TestEqual(TEXT("full HP stays full"), FSleepRules::HPOnWake(100.0f, 100.0f), 100.0f);
	TestEqual(TEXT("50 HP wakes at 75"), FSleepRules::HPOnWake(50.0f, 100.0f), 75.0f);
	TestEqual(TEXT("0 HP wakes at 50"), FSleepRules::HPOnWake(0.0f, 100.0f), 50.0f);
	TestEqual(TEXT("a pool-less dragon wakes at 0"), FSleepRules::HPOnWake(50.0f, 0.0f), 0.0f);

	// --- Whole outcome: one ResolveSleep composes every rule ---
	FSleepRequest Request;
	Request.NotorietyPoints = 150;
	Request.BulkGold = 2000;
	Request.bHasUnequippedArtifact = true;
	Request.CurrentHP = 40.0f;
	Request.MaxHP = 100.0f;
	Request.MaxStamina = 100.0f;
	Request.Random01 = 0.01f;
	const FSleepOutcome Outcome = FSleepRules::ResolveSleep(Request);
	TestEqual(TEXT("5 stars decays to 60 points in the outcome"), Outcome.NotorietyPoints, 60);
	TestEqual(TEXT("outcome reads 3 stars"), Outcome.Stars, 3);
	TestEqual(TEXT("outcome loses 15% of 2000 bulk"), Outcome.BulkGoldLost, (int64)300);
	TestTrue(TEXT("outcome's low roll takes the artifact"), Outcome.bArtifactTaken);
	TestEqual(TEXT("outcome wakes at 70 HP"), Outcome.HPOnWake, 70.0f);
	TestEqual(TEXT("outcome wakes at full stamina"), Outcome.StaminaOnWake, 100.0f);

	Request.Random01 = 0.5f;
	const FSleepOutcome SafeOutcome = FSleepRules::ResolveSleep(Request);
	TestFalse(TEXT("a high roll leaves artifacts alone"), SafeOutcome.bArtifactTaken);

	// --- Cannot soft-lock: the clock skip is always positive ---
	// (HoursUntilNextDawn is pinned in the clock test; here the
	// wake-guarantee at the exact dawn boundary sleep lands on.)
	TestTrue(TEXT("sleep at dawn still advances a full day"),
			 FWorldClock::HoursUntilNextDawn(6.0f) > 0.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
