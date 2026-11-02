// Copyright Under the Mountain contributors.
// Purpose: T-016 game-loop rule tests - notoriety buckets and
// gains (AT-Notoriety), hoard outcome boundaries
// (AT-HoardThresholds), and the wave composition table. Pure
// statics and subsystem objects, no world spawned.

#include "Misc/AutomationTest.h"
#include "Hoard/HoardSubsystem.h"
#include "Notoriety/NotorietySubsystem.h"
#include "Waves/WaveSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

// [2026-11-01 T-016] Engine filter, runs headless in Scripts/test-fast.sh.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameLoopRulesTest, "UnderTheMountain.GameLoop.Rules",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameLoopRulesTest::RunTest(const FString& Parameters)
{
	// --- AT-Notoriety: points -> stars bucket math (section 4) ---
	TestEqual(TEXT("0 points is 0 stars"), UNotorietySubsystem::StarsForPoints(0), 0);
	TestEqual(TEXT("9 points is 0 stars"), UNotorietySubsystem::StarsForPoints(9), 0);
	TestEqual(TEXT("10 points is 1 star"), UNotorietySubsystem::StarsForPoints(10), 1);
	TestEqual(TEXT("29 points is 1 star"), UNotorietySubsystem::StarsForPoints(29), 1);
	TestEqual(TEXT("30 points is 2 stars"), UNotorietySubsystem::StarsForPoints(30), 2);
	TestEqual(TEXT("60 points is 3 stars"), UNotorietySubsystem::StarsForPoints(60), 3);
	TestEqual(TEXT("100 points is 4 stars"), UNotorietySubsystem::StarsForPoints(100), 4);
	TestEqual(TEXT("150 points is 5 stars"), UNotorietySubsystem::StarsForPoints(150), 5);
	TestEqual(TEXT("huge points stays 5 stars"), UNotorietySubsystem::StarsForPoints(9999), 5);

	// --- Inside-lair zero-gain invariant ---
	TestTrue(TEXT("hoard room is inside"),
			 UNotorietySubsystem::IsInsideLair(FVector(19000.0f, 0.0f, 300.0f)));
	TestTrue(TEXT("entry chamber is inside"),
			 UNotorietySubsystem::IsInsideLair(FVector(0.0f, 0.0f, 150.0f)));
	TestFalse(TEXT("valley village is outside"),
			  UNotorietySubsystem::IsInsideLair(FVector(38000.0f, 0.0f, 100.0f)));
	TestEqual(TEXT("inside kill gains 0"),
			  UNotorietySubsystem::PointsForKillAt(FVector(6000.0f, 0.0f, 150.0f)), 0);
	TestEqual(TEXT("outside kill gains 10"),
			  UNotorietySubsystem::PointsForKillAt(FVector(38000.0f, 0.0f, 100.0f)), 10);

	// --- Point gains accumulate through the verbs ---
	UNotorietySubsystem* Notoriety = NewObject<UNotorietySubsystem>();
	Notoriety->AddEscapePoints(false);
	TestEqual(TEXT("escape is +5"), Notoriety->GetPoints(), 5);
	Notoriety->AddEscapePoints(true);
	TestEqual(TEXT("escape with loot adds +8 (13 total)"), Notoriety->GetPoints(), 13);
	TestEqual(TEXT("13 points is 1 star"), Notoriety->GetStars(), 1);
	Notoriety->AddPublicKillPoints(FVector(6000.0f, 0.0f, 150.0f));
	TestEqual(TEXT("inside kill through the verb gains 0"), Notoriety->GetPoints(), 13);
	Notoriety->AddPublicKillPoints(FVector(38000.0f, 0.0f, 100.0f));
	TestEqual(TEXT("outside kill through the verb adds 10"), Notoriety->GetPoints(), 23);

	// --- AT-HoardThresholds: boundaries at the spec's 8500 start,
	// fractions read from the classifier's parameters (0.5/0.2) ---
	auto Classify = [](int64 Current, bool bFinal, bool bDown)
	{
		return UHoardSubsystem::ClassifyOutcome(Current, 8500, 0.5f, 0.2f, bFinal, bDown);
	};
	TestTrue(TEXT("4249 at final scoring is pyrrhic"),
			 Classify(4249, true, false) == EHoardOutcome::PyrrhicWin);
	TestTrue(TEXT("4250 at final scoring wins"), Classify(4250, true, false) == EHoardOutcome::Win);
	TestTrue(TEXT("1700 at final scoring is pyrrhic"),
			 Classify(1700, true, false) == EHoardOutcome::PyrrhicWin);
	TestTrue(TEXT("1699 at final scoring is plundered"),
			 Classify(1699, true, false) == EHoardOutcome::LosePlundered);
	TestTrue(TEXT("thin hoard mid-session is not yet a loss"),
			 Classify(100, false, false) == EHoardOutcome::InProgress);
	TestTrue(TEXT("dragon down loses on a full hoard"),
			 Classify(8500, true, true) == EHoardOutcome::LoseDrivenOut);
	TestTrue(TEXT("dragon down loses mid-session too"),
			 Classify(8500, false, true) == EHoardOutcome::LoseDrivenOut);

	// --- Hoard ledger: session start latches, thefts reduce ---
	UHoardSubsystem* Hoard = NewObject<UHoardSubsystem>();
	Hoard->RecordSessionStart(3000);
	Hoard->RecordSessionStart(9999);
	TestEqual(TEXT("session start latches the first total"), Hoard->GetStartingHoard(), (int64)3000);
	Hoard->RecordTheft(250, TEXT("TestThief"));
	TestEqual(TEXT("current hoard is registry minus stolen"), Hoard->GetCurrentHoard(3000), (int64)2750);

	// --- Wave table (section 4): totals 2, 3, 7, 8, 12, 13 ---
	const int32 ExpectedTotals[] = {2, 3, 7, 8, 12, 13};
	for (int32 Wave = 1; Wave <= 6; ++Wave)
	{
		TestEqual(FString::Printf(TEXT("wave %d total actors"), Wave),
				  UWaveSubsystem::TotalActorsForWave(Wave), ExpectedTotals[Wave - 1]);
	}
	int32 B = 0, D = 0, A = 0;
	UWaveSubsystem::CompositionForWave(3, B, D, A);
	TestTrue(TEXT("wave 3 is 3 burglars, 1 party, 1 adventurer"), B == 3 && D == 1 && A == 1);
	UWaveSubsystem::CompositionForWave(99, B, D, A);
	TestTrue(TEXT("out-of-range wave clamps to wave 6"), B == 4 && D == 2 && A == 3);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
